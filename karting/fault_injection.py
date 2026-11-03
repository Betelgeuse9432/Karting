"""Sabotage volontaire et reproductible, réservé à la branche de test.

Une seule panne est armée à la fois. L'exception est levée à l'entrée du
composant, avant qu'il ait eu le temps de modifier son état : cela teste
l'isolation des exceptions sans simuler de corruption silencieuse.
"""
from __future__ import annotations

import logging
import math
import random
import secrets
from dataclasses import dataclass
from collections.abc import Collection

logger = logging.getLogger("karting")

# Ces fonctions sont appelées à chaque image en fonctionnement normal.
# Les éléments rares (sauvegarde, fumée spécifique, etc.) sont exclus pour
# éviter d'armer un défaut qui ne se déclencherait jamais.
OPTIONAL_TARGETS = (
    "input", "camera", "effects.sim", "render.background", "render.track",
    "render.marks", "render.vehicle", "render.gantry", "render.hud",
)
ESSENTIAL_TARGETS = ("physics", "race")


class InjectedFault(RuntimeError):
    """Exception intentionnelle, différente d'un vrai défaut logiciel."""


@dataclass(frozen=True)
class InjectionSettings:
    min_interval: float = 2.0
    max_interval: float = 5.0
    seed: int | None = None
    include_essential: bool = True

    def validate(self) -> None:
        if not (math.isfinite(self.min_interval) and math.isfinite(self.max_interval)):
            raise ValueError("Les intervalles doivent être finis")
        if self.min_interval <= 0 or self.max_interval < self.min_interval:
            raise ValueError("Intervalle invalide : min > 0 et max >= min requis")


class FaultInjector:
    """Choisit un composant à des instants pseudo-aléatoires.

    Le planificateur avance avec le temps du jeu et non avec une horloge
    externe. Avec une graine et des pas de temps identiques, la suite des
    injections est reproductible pour les tests.
    """

    def __init__(self, settings: InjectionSettings | None = None):
        self.settings = settings or InjectionSettings()
        self.settings.validate()
        self.seed = self.settings.seed if self.settings.seed is not None else secrets.randbits(32)
        self.rng = random.Random(self.seed)
        self.time = 0.0
        self.enabled = True
        self.pending: str | None = None
        self.count = 0
        self.last: str | None = None
        self.next_failure = self._next_time()
        logger.warning("TEST SABOTAGE ACTIVÉ | seed=%s | intervalle=%.1f-%.1fs | physique=%s",
                       self.seed, self.settings.min_interval, self.settings.max_interval,
                       self.settings.include_essential)

    def _next_time(self) -> float:
        return self.time + self.rng.uniform(self.settings.min_interval, self.settings.max_interval)

    def _choose(self, disabled: Collection[str]) -> str | None:
        available = [name for name in OPTIONAL_TARGETS if name not in disabled]
        weights = [1.0] * len(available)
        if self.settings.include_essential:
            available.extend(ESSENTIAL_TARGETS)
            weights.extend([0.35] * len(ESSENTIAL_TARGETS))
        if not available:
            return None
        return self.rng.choices(available, weights=weights, k=1)[0]

    def advance(self, dt: float, disabled: Collection[str]) -> None:
        """Prépare la prochaine injection, sans interrompre l'image courante."""
        if not self.enabled:
            return
        if not math.isfinite(dt) or dt < 0:
            raise ValueError("Pas de temps invalide pour le saboteur")
        self.time += dt
        if self.pending is not None or self.time < self.next_failure:
            return
        self.pending = self._choose(disabled)
        if self.pending is None:
            self.next_failure = self._next_time()
        else:
            logger.warning("TEST SABOTAGE : panne armée de %s (t=%.2fs)", self.pending, self.time)

    def force_random(self, disabled: Collection[str]) -> str | None:
        """F10 : armer immédiatement une panne aléatoire parmi les composants disponibles."""
        if not self.enabled:
            return None
        self.pending = self._choose(disabled)
        return self.pending

    def before_call(self, component: str) -> None:
        """Appelé avant une opération ; déclenche la panne seulement sur sa cible."""
        if not self.enabled or component != self.pending:
            return
        self.count += 1
        self.last = component
        self.pending = None
        self.next_failure = self._next_time()
        message = (f"PANNE INJECTEE #{self.count} : {component} "
                   f"a t={self.time:.2f}s (seed={self.seed})")
        logger.warning(message)
        raise InjectedFault(message)

    def toggle(self) -> bool:
        self.enabled = not self.enabled
        self.pending = None
        if self.enabled:
            self.next_failure = self._next_time()
        logger.warning("TEST SABOTAGE %s", "ACTIVÉ" if self.enabled else "DÉSACTIVÉ")
        return self.enabled

    def reset(self) -> None:
        """R : effacer la cible éventuelle et repartir sur un délai aléatoire."""
        self.pending = None
        self.next_failure = self._next_time()
