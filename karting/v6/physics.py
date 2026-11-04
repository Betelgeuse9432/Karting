"""Physique V6 indépendante de Pygame, pilotée par les profils de voitures."""
from dataclasses import dataclass
import math

from ..state import Controls, Telemetry, VehicleState
from .track import V6Track, height


@dataclass(frozen=True, slots=True)
class MotionResult:
    telemetry: Telemetry
    grass_wheels: int
    grip_factor: float
    slope: float
    collision_impact: float


class V6Physics:
    def __init__(self, track: V6Track):
        self.track = track

    def step(self, car: VehicleState, controls: Controls, vehicle: dict,
             wheel_positions: list[tuple[float, float]], rain: float, dt: float) -> MotionResult:
        if not math.isfinite(dt) or not 0 < dt <= self.track.config.max_delta + 1e-9:
            raise ValueError(f"Pas physique invalide : {dt}")
        if not math.isfinite(rain) or not 0 <= rain <= 1:
            raise ValueError("Intensité de pluie invalide")
        car.validate()
        cfg = self.track.config
        info = self.track.info(car.x, car.y)
        outside = self.track.wheels_on_grass(car, wheel_positions, info.index)
        prev_gas = car.gas
        car.gas = min(1, car.gas + vehicle["pedale_gaz"]*dt) if controls.gas else max(0, car.gas-5*dt)
        car.brake = min(1, car.brake + vehicle["pedale_frein"]*dt) if controls.brake else max(0, car.brake-3*dt)
        v = car.v
        vmax = vehicle["vmax"]
        downforce = 1 + vehicle["appui"]*(abs(v)/vmax)**2
        grip = downforce*(1-0.3*rain)*(1-vehicle["herbe_grip"]*outside/4)
        front_grip = max(1, vehicle["grip_av"]*grip)
        rear_grip = max(1, vehicle["grip_ar"]*grip*(0.35 if controls.handbrake else 1.0))
        max_steer = vehicle["braquage"]/(1+abs(v)/vehicle["vsens"])
        car.steering += (controls.steering*max_steer-car.steering)*min(1, 8*dt)
        rotation = v*math.tan(car.steering)/vehicle["emp"]
        lateral = abs(v*rotation)
        brake_acc = vehicle["freinage"]*car.brake**1.5 if v > 0.5 else 0
        front_use = math.hypot(brake_acc, lateral)
        front_slip = front_use > front_grip
        if front_slip:
            rotation *= front_grip/front_use
        motor_acc = vehicle["puissance"]*car.gas*(1-max(v, 0)/vmax)
        rear_use = math.hypot(motor_acc, lateral)
        rear_slip = rear_use > rear_grip
        if rear_slip:
            excess = rear_use/rear_grip-1
            rotation *= 1+(min(2*excess, 1.5) if controls.handbrake else 2*excess)
            motor_acc *= rear_grip/rear_use
        ca, sa = math.cos(car.angle), math.sin(car.angle)
        slope = (height(car.x+6*ca,car.y+6*sa)-height(car.x,car.y))/6
        v += motor_acc*dt
        if v > 0.5:
            v -= brake_acc*dt
        elif controls.brake:
            v -= 250*dt
        # Résistance à l’avancement plus faible sous accélération : conserve
        # un bon rythme même sur les longues lignes droites de la V6.
        v -= v*(0.18 if controls.gas else 0.5)*dt
        if controls.handbrake:
            v -= math.copysign(min(abs(v), 370*dt),v)
        v -= v*vehicle["herbe_frein"]*outside*dt
        v -= slope*1200*dt
        car.v = v = max(-150, min(vmax, v))

        delta = rotation*dt
        if controls.handbrake or abs(car.lateral_speed)>0.01:
            side = car.lateral_speed*math.cos(delta)-v*math.sin(delta)
            recovery = 950*(0.055 if controls.handbrake else 1.0)
            side = math.copysign(max(0, abs(side)-recovery*dt),side)
            car.lateral_speed = side if abs(side)>=0.01 else 0.0
        else:
            car.lateral_speed = 0.0
        car.angle += delta
        ca, sa = math.cos(car.angle), math.sin(car.angle)
        nx, ny = car.x+(ca*v-sa*car.lateral_speed)*dt, car.y+(sa*v+ca*car.lateral_speed)*dt
        w,h = cfg.world_size
        car.x, car.y = max(0,min(w,nx)), max(0,min(h,ny))
        if car.x != nx or car.y != ny:
            car.lateral_speed = 0.0
        impact = self.track.barrier(car,dt).impact
        if car.brake>0.05 and car.v>0:
            car.heat += car.brake*(abs(car.v)/vmax)*1.6*dt
        else:
            car.heat -= 0.4*dt
        if info.in_pits:
            car.heat = 0.0
        car.heat = max(0,min(1,car.heat))
        smoothing = min(1,6*dt)
        target_pitch = ((car.brake*0.05 - car.gas*0.03*(1-abs(car.v)/vmax))
                        *vehicle["tangage_k"]-slope)
        car.pitch += (target_pitch-car.pitch)*smoothing
        target_roll = max(-vehicle["roulis_max"],min(vehicle["roulis_max"],
                                                      vehicle["roulis_k"]*car.v*rotation))
        car.roll += (target_roll-car.roll)*smoothing
        target_wing = 0.1+0.25*(abs(car.v)/vmax)+0.9*car.brake
        car.wing += (target_wing-car.wing)*min(1,10*dt)
        car.wheel_rotation += car.v*dt/10*0.35
        car.validate()
        base=Telemetry(info.index, info.distance, info.offset,info.in_pits,
                       outside>=2,front_slip,rear_slip,car.brake>0.9 and v>350,
                       downforce,prev_gas,abs(car.lateral_speed)>12)
        return MotionResult(base,outside,grip,slope,impact)
