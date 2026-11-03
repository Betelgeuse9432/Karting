"""Géométrie de piste et stands. Aucun import Pygame."""
from __future__ import annotations
from dataclasses import dataclass
import math
from .config import DEFAULT_POINTS, GameConfig, RGB

Point = tuple[float, float]


@dataclass(frozen=True, slots=True)
class Garage:
    a0: Point
    a1: Point
    b0: Point
    b1: Point
    door_color: RGB
    track_index: int


@dataclass(frozen=True, slots=True)
class TrackInfo:
    index: int
    distance: float
    offset: float
    in_pits: bool
    on_grass: bool


def smooth(points: list[Point], steps: int) -> list[Point]:
    """Interpolation de Catmull-Rom fermée, identique à la version V3."""
    if len(points) < 4 or steps < 1:
        raise ValueError("La piste nécessite au moins quatre points et un pas positif")
    result = []
    n = len(points)
    for i in range(n):
        p0, p1, p2, p3 = points[i - 1], points[i], points[(i + 1) % n], points[(i + 2) % n]
        for k in range(steps):
            t = k / steps
            x = 0.5 * (2*p1[0] + (-p0[0]+p2[0])*t + (2*p0[0]-5*p1[0]+4*p2[0]-p3[0])*t**2 + (-p0[0]+3*p1[0]-3*p2[0]+p3[0])*t**3)
            y = 0.5 * (2*p1[1] + (-p0[1]+p2[1])*t + (2*p0[1]-5*p1[1]+4*p2[1]-p3[1])*t**2 + (-p0[1]+3*p1[1]-3*p2[1]+p3[1])*t**3)
            result.append((x, y))
    return result


class Track:
    """Une instance possède sa piste, ses normales et ses zones de stands."""
    DOOR_COLORS = ((220, 40, 40), (40, 90, 220), (240, 200, 30),
                   (40, 170, 90), (230, 120, 20), (150, 60, 200))

    def __init__(self, config: GameConfig, raw_points: tuple[Point, ...] = DEFAULT_POINTS):
        self.config = config
        points = [(x * config.circuit_scale, y * config.circuit_scale) for x, y in raw_points]
        self.center = smooth(points, config.smoothing_steps)
        self.count = len(self.center)
        half, kerb = config.half_track_width, config.kerb
        self.offsets = (half + kerb, half, -half, -(half + kerb), 2, -2)
        self.lines: list[list[Point]] = [[] for _ in self.offsets]
        self.normals: list[Point] = []
        for i in range(self.count):
            x0, y0 = self.center[i - 1]
            x1, y1 = self.center[(i + 1) % self.count]
            length = math.hypot(x1-x0, y1-y0) or 1
            nx, ny = -(y1-y0) / length, (x1-x0) / length
            self.normals.append((nx, ny))
            for k, distance in enumerate(self.offsets):
                self.lines[k].append((self.center[i][0] + nx*distance, self.center[i][1] + ny*distance))

        self.pit_d1 = half + kerb + 4
        self.pit_d2 = self.pit_d1 + 110
        self.pit_end = self.pit_d2 + 6
        pit_start, pit_length = self.count - 45, 95
        self.pit_indices = [(pit_start + m) % self.count for m in range(pit_length)]
        self.pit_set = set(self.pit_indices)
        self.pit_lines = [[self.position(i, d) for i in range(self.count)]
                          for d in (self.pit_d1, self.pit_d2-3, self.pit_d2)]
        self.garages = []
        for g, m in enumerate(range(2, pit_length-12, 14)):
            i0, i1 = self.pit_indices[m], self.pit_indices[m + 10]
            self.garages.append(Garage(self.position(i0, self.pit_d2+30),
                                      self.position(i1, self.pit_d2+30),
                                      self.position(i0, self.pit_d2+130),
                                      self.position(i1, self.pit_d2+130),
                                      self.DOOR_COLORS[g % len(self.DOOR_COLORS)], self.pit_indices[m+5]))

    def position(self, i: int, d: float) -> Point:
        return (self.center[i][0]+self.normals[i][0]*d,
                self.center[i][1]+self.normals[i][1]*d)

    def nearest(self, x: float, y: float) -> tuple[int, float]:
        if not math.isfinite(x) or not math.isfinite(y):
            raise ValueError("Coordonnées de piste non finies")
        i = min(range(self.count), key=lambda k: (self.center[k][0]-x)**2 + (self.center[k][1]-y)**2)
        return i, math.hypot(self.center[i][0]-x, self.center[i][1]-y)

    def info(self, x: float, y: float) -> TrackInfo:
        i, distance = self.nearest(x, y)
        offset = (x-self.center[i][0])*self.normals[i][0] + (y-self.center[i][1])*self.normals[i][1]
        in_pits = i in self.pit_set and self.config.half_track_width+self.config.kerb < offset < self.pit_end
        grass = distance > self.config.half_track_width+self.config.kerb and not in_pits
        return TrackInfo(i, distance, offset, in_pits, grass)

    def start_position(self) -> tuple[float, float, float]:
        angle = math.atan2(self.center[1][1]-self.center[0][1],
                           self.center[1][0]-self.center[0][0])
        return *self.center[0], angle
