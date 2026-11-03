"""Caméra 3D et projection perspective, indépendantes de Pygame."""
import math
from .config import GameConfig
from .state import VehicleState


class Camera:
    def __init__(self, config: GameConfig, angle: float = 0):
        self.cfg = config
        self.angle = angle
        self.x = self.y = 0.0
        self.cs, self.sn = math.cos(angle), math.sin(angle)

    def follow(self, car: VehicleState, dt: float) -> None:
        delta = (car.angle-self.angle+math.pi) % (2*math.pi)-math.pi
        self.angle += delta*min(1, 3.5*dt)
        self.cs, self.sn = math.cos(self.angle), math.sin(self.angle)
        self.x = car.x-self.cs*self.cfg.camera_distance
        self.y = car.y-self.sn*self.cfg.camera_distance

    def position(self, x: float, y: float) -> tuple[float, float]:
        dx, dy = x-self.x, y-self.y
        return dx*self.cs+dy*self.sn, -dx*self.sn+dy*self.cs

    def space(self, x: float, y: float, z: float) -> tuple[float, float, float]:
        f, r = self.position(x, y)
        return f, r, z

    def ground(self, x: float, y: float) -> tuple[float, float, float]:
        return self.space(x, y, 0)

    def project(self, points: list[tuple[float, float, float]]) -> list[tuple[float, float]] | None:
        result = []
        n = len(points)
        for i in range(n):
            a, b = points[i], points[(i+1) % n]
            a_ok, b_ok = a[0] >= self.cfg.near, b[0] >= self.cfg.near
            if a_ok:
                result.append(a)
            if a_ok != b_ok:
                t = (self.cfg.near-a[0])/(b[0]-a[0])
                result.append((self.cfg.near, a[1]+(b[1]-a[1])*t, a[2]+(b[2]-a[2])*t))
        if len(result) < 3:
            return None
        cfg = self.cfg
        return [(max(-20000, min(20000, cfg.width/2+r/f*cfg.focal)),
                 max(-20000, min(20000, cfg.horizon+(cfg.camera_height-z)/f*cfg.focal)))
                for f, r, z in result]
