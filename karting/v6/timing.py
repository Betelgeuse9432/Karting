"""Horloge de simulation indépendante du rendu et de Pygame."""
from dataclasses import dataclass, field
import math


@dataclass(slots=True)
class FixedStepClock:
    """Accumule le temps réel pour rejouer les pas physiques oubliés.

    La simulation ne ralentit plus proportionnellement au taux d'images :
    chaque image peut déclencher plusieurs pas d'une durée déterministe.
    """
    hz: int = 60
    max_steps: int = 8
    max_frame_seconds: float = 0.15
    accumulator: float = field(default=0.0, init=False)

    @property
    def dt(self) -> float:
        return 1.0 / self.hz

    def reset(self) -> None:
        self.accumulator = 0.0

    def consume(self, frame_seconds: float) -> int:
        if not math.isfinite(frame_seconds) or frame_seconds < 0:
            raise ValueError("Temps de frame invalide")
        self.accumulator += min(frame_seconds, self.max_frame_seconds)
        steps = min(self.max_steps, int((self.accumulator + 1e-10) / self.dt))
        self.accumulator -= steps * self.dt
        if steps == self.max_steps:
            # En forte surcharge, éviter une dette de calcul qui augmenterait
            # continuellement la charge CPU.
            self.accumulator = min(self.accumulator, self.dt)
        self.accumulator = max(0.0, self.accumulator)
        return steps
