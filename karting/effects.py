"""Particules et traces de pneus, système facultatif indépendant du rendu."""
from collections import deque
from dataclasses import dataclass
import math
import random
from .state import Telemetry, VehicleState


@dataclass(slots=True)
class Particle:
    x: float
    y: float
    z: float
    vx: float
    vy: float
    vz: float
    lifetime: float
    initial_lifetime: float
    kind: str
    size: float


class Effects:
    def __init__(self, seed: int = 7):
        self.rng = random.Random(seed)
        self.particles: list[Particle] = []
        self.marks = deque(maxlen=1600)
        self.previous_wheels: list[tuple[float, float] | None] = [None]*4
        self.frame = 0

    def clear(self) -> None:
        self.particles.clear()
        self.marks.clear()
        self.previous_wheels = [None]*4
        self.frame = 0

    def emit(self, car: VehicleState, kind: str, lx: float, ly: float, lz: float,
             vx: float, vy: float, vz: float, lifetime: float, size: float) -> None:
        if len(self.particles) > 450:
            return
        wx, wy, wz = car.local_to_world(lx, ly, lz)
        ca, sa = math.cos(car.angle), math.sin(car.angle)
        inertia = {"fumee": 0.2, "flamme": 0.5, "etincelle": 0.7}[kind]*car.v
        self.particles.append(Particle(wx, wy, wz, ca*vx-sa*vy+ca*inertia,
                                       sa*vx+ca*vy+sa*inertia, vz, lifetime, lifetime, kind, size))

    def step(self, car: VehicleState, info: Telemetry, dt: float) -> None:
        self.frame += 1
        r = self.rng
        v = car.v
        if car.brake > 0.3 and car.heat > 0.3 and v > 120:
            for side in (-1, 1):
                for _ in range(2):
                    self.emit(car, "etincelle", 24, side*20, 8,
                              -r.uniform(40, 160), side*r.uniform(10, 90),
                              r.uniform(40, 140), r.uniform(0.25, 0.6), 1)
        if (not car.gas and info.previous_gas > 0.35 and v > 250) or (car.brake > 0.5 and v > 350 and r.random() < 0.12):
            for side in (-1, 1):
                for _ in range(3):
                    self.emit(car, "flamme", -40, side*9, 7,
                              -r.uniform(60, 180), r.uniform(-20, 20), r.uniform(-10, 30),
                              r.uniform(0.12, 0.28), r.uniform(5, 9))
        if (info.rear_slip or info.front_slip or info.hard_braking or info.drifting) and not info.on_grass and self.frame % 2 == 0:
            axles = ([-22] if (info.rear_slip or info.drifting) else []) + ([22] if (info.front_slip or info.hard_braking) else [])
            for lx in axles:
                for side in (-1, 1):
                    self.emit(car, "fumee", lx, side*21, 3, -r.uniform(0, 30), r.uniform(-20, 20),
                              r.uniform(15, 40), r.uniform(0.6, 1.2), r.uniform(7, 11))
        slipping = (info.rear_slip or info.front_slip or info.hard_braking or info.drifting) and not info.on_grass
        for k, (lx, ly) in enumerate(((-22,-21), (-22,21), (22,-21), (22,21))):
            wx, wy, _ = car.local_to_world(lx, ly)
            if slipping:
                previous = self.previous_wheels[k]
                if previous:
                    self.marks.append((previous[0], previous[1], wx, wy))
                self.previous_wheels[k] = (wx, wy)
            else:
                self.previous_wheels[k] = None
        self.update_particles(dt)

    def update_particles(self, dt: float) -> None:
        for p in self.particles:
            p.x += p.vx*dt
            p.y += p.vy*dt
            p.z += p.vz*dt
            p.lifetime -= dt
            if p.kind == "etincelle":
                p.vz -= 500*dt
                if p.z < 0:
                    p.z, p.vz = 0, -p.vz*0.3
            elif p.kind == "fumee":
                p.size += 18*dt
        self.particles = [p for p in self.particles if p.lifetime > 0]
