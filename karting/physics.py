"""Simulation du véhicule : aucune dépendance à Pygame ou au rendu."""
import math
from .config import GameConfig
from .state import Controls, Telemetry, VehicleState
from .track import Track


class VehiclePhysics:
    def __init__(self, config: GameConfig):
        self.config = config

    def step(self, car: VehicleState, controls: Controls, track: Track, dt: float) -> Telemetry:
        if not math.isfinite(dt) or dt <= 0 or dt > self.config.max_delta + 1e-9:
            raise ValueError(f"Pas de simulation invalide : {dt}")
        car.validate()
        cfg = self.config
        info = track.info(car.x, car.y)
        previous_gas = car.gas
        car.gas = min(1, car.gas + cfg.gas_pedal_rate*dt) if controls.gas else max(0, car.gas-5*dt)
        car.brake = min(1, car.brake + cfg.brake_pedal_rate*dt) if controls.brake else max(0, car.brake-3*dt)

        v = car.v
        downforce = 1 + cfg.downforce*(abs(v)/cfg.max_speed)**2
        front_grip, rear_grip = cfg.front_grip*downforce, cfg.rear_grip*downforce
        if controls.handbrake:
            rear_grip *= cfg.handbrake_rear_grip
        max_turn = 0.55 / (1 + abs(v)/150)
        car.steering += (controls.steering*max_turn-car.steering)*min(1, 8*dt)
        rotation = v*math.tan(car.steering)/cfg.wheelbase
        lateral = abs(v*rotation)

        brake_acc = cfg.braking * car.brake**1.5 if v > 0.5 else 0
        front_use = math.hypot(brake_acc, lateral)
        front_slip = front_use > front_grip
        if front_slip:
            rotation *= front_grip/front_use

        motor_acc = cfg.power*car.gas*(1-max(v, 0)/cfg.max_speed)
        rear_use = math.hypot(motor_acc, lateral)
        rear_slip = rear_use > rear_grip
        if rear_slip:
            excess = rear_use/rear_grip-1
            rotation *= 1 + (min(2*excess, 1.5) if controls.handbrake else 2*excess)
            motor_acc *= rear_grip/rear_use

        v += motor_acc*dt
        if v > 0.5:
            v -= brake_acc*dt
        elif controls.brake:
            v -= 250*dt
        v -= v*(0.3 if controls.gas else 0.6)*dt
        if controls.handbrake:
            v -= math.copysign(min(abs(v), cfg.handbrake_drag*dt), v)
        if info.on_grass:
            v -= v*2.5*dt
            v = min(v, 220)
        car.v = v = max(-150, min(cfg.max_speed, v))

        # Le frein à main libère la dérive : la trajectoire n'est plus obligée de
        # suivre immédiatement l'orientation du châssis. En sortie de dérapage,
        # le grip fait revenir progressivement la vitesse latérale vers zéro.
        delta_angle = rotation*dt
        if controls.handbrake or abs(car.lateral_speed) > 0.01:
            side = car.lateral_speed*math.cos(delta_angle) - v*math.sin(delta_angle)
            grip = cfg.lateral_recovery * (cfg.handbrake_lateral_factor if controls.handbrake else 1.0)
            side = math.copysign(max(0.0, abs(side)-grip*dt), side)
            car.lateral_speed = side if abs(side) >= 0.01 else 0.0
        else:
            car.lateral_speed = 0.0

        car.angle += delta_angle
        ca, sa = math.cos(car.angle), math.sin(car.angle)
        dx = (ca*v-sa*car.lateral_speed)*dt
        dy = (sa*v+ca*car.lateral_speed)*dt
        world_w, world_h = cfg.world_size
        next_x, next_y = car.x+dx, car.y+dy
        car.x = max(0, min(world_w, next_x))
        car.y = max(0, min(world_h, next_y))
        if car.x != next_x or car.y != next_y:
            car.lateral_speed = 0.0

        if car.brake > 0.05 and v > 0:
            car.heat += car.brake*(abs(v)/cfg.max_speed)*1.6*dt
        else:
            car.heat -= 0.4*dt
        if info.in_pits:
            car.heat = 0
        car.heat = max(0, min(1, car.heat))

        smoothing = min(1, 6*dt)
        car.pitch += ((car.brake*0.9-car.gas*0.5*(1-abs(v)/cfg.max_speed))*0.12-car.pitch)*smoothing
        car.roll += (0.00025*v*rotation-car.roll)*smoothing
        target_wing = 0.1+0.25*(abs(v)/cfg.max_speed)+0.9*car.brake
        car.wing += (target_wing-car.wing)*min(1, 10*dt)
        car.wheel_rotation += v*dt/9*0.35
        car.validate()
        hard_braking = car.brake > 0.9 and v > 350
        return Telemetry(info.index, info.distance, info.offset, info.in_pits,
                         info.on_grass, front_slip, rear_slip, hard_braking,
                         downforce, previous_gas, abs(car.lateral_speed) > 12)
