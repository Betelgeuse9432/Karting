"""Paramètres V6. Les préréglages du jeu sont des objets immuables."""
from dataclasses import dataclass
from .track_layout import BRUT
from ..config import GameConfig


@dataclass(frozen=True, slots=True)
class V6Config(GameConfig):
    width: int = 1100
    height: int = 720
    max_speed: float = 700.0
    power: float = 800.0
    braking: float = 760.0
    front_grip: float = 740.0
    rear_grip: float = 780.0
    downforce: float = 0.55
    wheelbase: float = 46.0
    half_track_width: float = 150.0
    circuit_scale: float = 1.0
    kerb: float = 22.0
    focal: float = 638.0
    near: float = 50.0
    far: float = 2100.0
    record_path: str = "record_karting_v6.txt"
    scenery_seed: int = 7

    @property
    def world_size(self) -> tuple[int, int]:
        return (int(max(p[0] for p in BRUT) * self.circuit_scale) + 1800,
                int(max(p[1] for p in BRUT) * self.circuit_scale) + 1800)
