"""Données de simulation sans dépendance graphique ni état global mutable."""
from dataclasses import dataclass, field
import math


@dataclass(frozen=True, slots=True)
class Controls:
    gas: bool = False
    brake: bool = False
    steering: int = 0  # -1 gauche, 0 neutre, +1 droite


@dataclass(slots=True)
class VehicleState:
    x: float
    y: float
    angle: float
    v: float = 0.0
    steering: float = 0.0
    gas: float = 0.0
    brake: float = 0.0
    heat: float = 0.0
    pitch: float = 0.0
    roll: float = 0.0
    wing: float = 0.1
    wheel_rotation: float = 0.0

    def local_to_world(self, lx: float, ly: float, z: float = 0) -> tuple[float, float, float]:
        ca, sa = math.cos(self.angle), math.sin(self.angle)
        return self.x + ca * lx - sa * ly, self.y + sa * lx + ca * ly, z

    def validate(self) -> None:
        for name in self.__dataclass_fields__:
            v = getattr(self, name)
            if not math.isfinite(v):
                raise FloatingPointError(f"État physique non fini : {name}={v}")


@dataclass(slots=True)
class RaceState:
    next_sector: int = 1
    laps: int = 0
    elapsed: float = 0.0
    best: float | None = None
    last: float | None = None
    recent: list[float] = field(default_factory=list)
    max_speed: float = 0.0


@dataclass(frozen=True, slots=True)
class Telemetry:
    track_index: int
    track_distance: float
    track_offset: float
    in_pits: bool
    on_grass: bool
    front_slip: bool
    rear_slip: bool
    hard_braking: bool
    downforce_factor: float
    previous_gas: float
