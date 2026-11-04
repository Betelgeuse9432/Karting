"""Circuit V6, pente, stands, rails et limites de roues. Aucune dépendance à Pygame."""
import math
from dataclasses import dataclass
from ..track import Garage, Track
from .track_layout import BRUT
from .config import V6Config


def height(x: float, y: float) -> float:
    return 70 * (0.6 * math.sin(x / 1100) + 0.4 * math.sin(y / 900 + 1)
                 + 0.3 * math.sin((x + y) / 1400 + 2))


@dataclass(frozen=True, slots=True)
class BarrierCollision:
    hit: bool = False
    impact: float = 0.0


class V6Track(Track):
    def __init__(self, config: V6Config):
        super().__init__(config, tuple(BRUT))
        self.mur = config.half_track_width + config.kerb + 70
        self.offsets = (*self.offsets, self.mur, -self.mur)
        for offset in (self.mur, -self.mur):
            self.lines.append([self.position(i, offset) for i in range(self.count)])
        self.distances = [0.0]
        for i in range(1, self.count):
            a, b = self.center[i-1], self.center[i]
            self.distances.append(self.distances[-1] + math.dist(a, b))
        self.length = self.distances[-1] + math.dist(self.center[-1], self.center[0])
        self.spacing = self.length / self.count
        pit_length = max(20, int(1900 / self.spacing))
        pit_start = self.count - max(3, int(500 / self.spacing))
        self.pit_indices = [(pit_start + m) % self.count for m in range(pit_length)]
        self.pit_set = set(self.pit_indices)
        self.pit_lines = [[self.position(i, d) for i in range(self.count)]
                          for d in (self.pit_d1, self.pit_d2 - 3, self.pit_d2)]
        self.garages = []
        step = max(1, int(340 / self.spacing))
        depth = max(2, int(250 / self.spacing))
        for g, m in enumerate(range(max(1, int(150 / self.spacing)), pit_length-depth-2, step)):
            i0, i1 = self.pit_indices[m], self.pit_indices[m+depth]
            self.garages.append(Garage(
                self.position(i0, self.pit_d2+30), self.position(i1, self.pit_d2+30),
                self.position(i0, self.pit_d2+150), self.position(i1, self.pit_d2+150),
                self.DOOR_COLORS[g % len(self.DOOR_COLORS)], self.pit_indices[m+depth//2]))

        # Les hauteurs du relief sur la piste sont fixes : les calculer une fois
        # évite des milliers d'appels à sin() lors de chaque image.
        self.line_heights = [[height(x, y) for x, y in line] for line in self.lines]
        self.pit_line_heights = [[height(x, y) for x, y in line] for line in self.pit_lines]

        # Recherche spatiale exacte de la portion de piste la plus proche.
        # Le balayage annulaire s'arrête seulement quand aucune cellule non
        # explorée ne peut contenir un point plus proche.
        self._cell_size = 320.0
        self._cells = {}
        for index, (x, y) in enumerate(self.center):
            cell = (math.floor(x/self._cell_size), math.floor(y/self._cell_size))
            self._cells.setdefault(cell, []).append(index)

    def nearest(self, x: float, y: float) -> tuple[int, float]:
        if not math.isfinite(x) or not math.isfinite(y):
            raise ValueError("Coordonnées de piste non finies")
        side = self._cell_size
        gx, gy = math.floor(x/side), math.floor(y/side)
        best_index, best_squared = -1, math.inf
        ring = 0
        while True:
            for ix in range(gx-ring, gx+ring+1):
                for iy in range(gy-ring, gy+ring+1):
                    if max(abs(ix-gx), abs(iy-gy)) != ring:
                        continue
                    for index in self._cells.get((ix, iy), ()):
                        px, py = self.center[index]
                        squared = (px-x)**2 + (py-y)**2
                        if squared < best_squared or (squared == best_squared and index < best_index):
                            best_index, best_squared = index, squared
            if best_index >= 0:
                border = min(x-(gx-ring)*side, (gx+ring+1)*side-x,
                             y-(gy-ring)*side, (gy+ring+1)*side-y)
                if best_squared <= border*border:
                    return best_index, math.sqrt(best_squared)
            ring += 1

    def wheel_is_outside(self, x: float, y: float, center_index: int) -> bool:
        i = min(((center_index + delta) % self.count for delta in range(-30, 31)),
                key=lambda k: (self.center[k][0]-x)**2 + (self.center[k][1]-y)**2)
        distance = math.dist(self.center[i], (x, y))
        half = self.config.half_track_width + self.config.kerb
        if distance <= half:
            return False
        off = (x-self.center[i][0])*self.normals[i][0] + (y-self.center[i][1])*self.normals[i][1]
        return not (i in self.pit_set and half < off < self.pit_end)

    def wheels_on_grass(self, car, wheel_positions, center_index=None) -> int:
        if center_index is None:
            center_index, _ = self.nearest(car.x, car.y)
        return sum(self.wheel_is_outside(*car.local_to_world(lx, ly)[:2], center_index)
                   for lx, ly in wheel_positions)

    def barrier(self, car, dt: float) -> BarrierCollision:
        info = self.info(car.x, car.y)
        pit_side = info.index in self.pit_set and info.offset > 0
        limit = (self.pit_d2 + 25) if pit_side else self.mur - 18
        if info.distance <= limit:
            return BarrierCollision()
        cx, cy = self.center[info.index]
        nx, ny = ((car.x - cx)/info.distance, (car.y - cy)/info.distance)
        car.x, car.y = cx+nx*limit, cy+ny*limit
        towards = max(0.0, math.cos(car.angle)*nx + math.sin(car.angle)*ny)
        impact = 0.0
        if towards > 0.05 and car.v > 30:
            impact = towards * car.v
            car.v *= 1 - 0.6*towards
            tx, ty = -self.normals[info.index][0], self.normals[info.index][1]
            tangent = math.atan2(ty, tx)
            if math.cos(car.angle-tangent) < 0:
                tangent += math.pi
            delta = (tangent-car.angle+math.pi) % (2*math.pi)-math.pi
            car.angle += delta*min(1, 6*dt)
            car.lateral_speed *= 0.3
        return BarrierCollision(True, impact)
