"""Paramètres de jeu immuables. Aucun module n'initialise Pygame à l'import."""
from dataclasses import dataclass
import math

RGB = tuple[int, int, int]

# Points d'origine de la V3, exprimés dans les unités du monde.
DEFAULT_POINTS: tuple[tuple[int, int], ...] = (
    (1800, 500), (2800, 500), (3500, 600), (3900, 1000),
    (3800, 1500), (3300, 1800), (3600, 2300), (3200, 2650),
    (2600, 2600), (2200, 2200), (1700, 2000), (1300, 2300),
    (800, 2600), (400, 2200), (500, 1600), (1000, 1300),
    (800, 900), (1100, 600),
)


@dataclass(frozen=True, slots=True)
class GameConfig:
    width: int = 900
    height: int = 650
    max_speed: float = 720.0
    power: float = 1500.0
    gas_pedal_rate: float = 1.0
    brake_pedal_rate: float = 1.6
    braking: float = 1200.0
    front_grip: float = 1000.0
    rear_grip: float = 1100.0
    downforce: float = 0.5
    wheelbase: float = 44.0
    handbrake_drag: float = 370.0  # ralentissement longitudinal (unités/s²)
    handbrake_rear_grip: float = 0.35
    lateral_recovery: float = 950.0  # adhérence latérale lors de la reprise
    handbrake_lateral_factor: float = 0.055
    half_track_width: float = 95.0
    circuit_scale: float = 0.8
    kerb: float = 16.0
    smoothing_steps: int = 45
    color_car: RGB = (220, 30, 30)
    focal: float = 520.0
    camera_height: float = 65.0
    camera_distance: float = 150.0
    near: float = 60.0
    far: float = 1500.0
    frame_rate: int = 60
    max_delta: float = 0.05
    scenery_seed: int = 7
    record_path: str = "record_karting.txt"

    @property
    def horizon(self) -> int:
        return int(self.height * 0.4)

    @property
    def world_size(self) -> tuple[int, int]:
        return int(4500 * self.circuit_scale), int(3100 * self.circuit_scale)

    def validate(self) -> None:
        positive = ("width", "height", "max_speed", "power", "braking", "front_grip",
                    "rear_grip", "wheelbase", "half_track_width", "circuit_scale",
                    "smoothing_steps", "focal", "camera_height", "camera_distance",
                    "near", "far", "frame_rate", "max_delta")
        for name in positive:
            value = getattr(self, name)
            if not isinstance(value, (float, int)) or not math.isfinite(value) or value <= 0:
                raise ValueError(f"Configuration invalide : {name}={value!r}")
        if self.handbrake_drag <= 0 or self.lateral_recovery <= 0:
            raise ValueError("Paramètres de frein à main invalides")
        if not 0 < self.handbrake_rear_grip <= 1 or not 0 < self.handbrake_lateral_factor <= 1:
            raise ValueError("Facteurs d’adhérence du frein à main invalides")
        if self.near >= self.far:
            raise ValueError("La distance near doit être inférieure à far")
        if self.kerb < 0 or self.downforce < 0 or self.gas_pedal_rate <= 0 or self.brake_pedal_rate <= 0:
            raise ValueError("Largeur de bordure, appui et pédales invalides")
        if not self.record_path:
            raise ValueError("Le chemin du record ne peut pas être vide")


CIEL: RGB = (110, 175, 255)
CIEL2: RGB = (170, 215, 255)
HERBE: RGB = (40, 150, 40)
HERBE2: RGB = (34, 130, 34)
ASPHALTE: RGB = (80, 80, 85)
ASPHALTE2: RGB = (72, 72, 77)
ROUGE: RGB = (220, 30, 30)
BLANC: RGB = (240, 240, 240)
NOIR: RGB = (25, 25, 25)
