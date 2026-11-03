"""Supervision des composants et réessais réels, sans attente bloquante.

Une panne n'est résolue qu'après exécution réussie de l'opération en défaut.
Les délais exponentiels évitent une boucle d'exceptions à 60 images/s.
"""
from __future__ import annotations

from dataclasses import dataclass
import logging
import math
from collections.abc import Callable
from typing import TypeVar

T = TypeVar("T")
logger = logging.getLogger("karting")


@dataclass(frozen=True)
class Fault:
    component: str
    message: str
    essential: bool


@dataclass
class RetryState:
    attempts: int
    next_retry: float
    essential: bool
    last_error: str


class FaultManager:
    """Circuit de réessai par composant, piloté par le temps de la boucle.

    L'échec déclenche un délai : 0,5 s, 1 s, 2 s, 4 s, puis 8 s
    entre les tentatives. Aucune limite de tentatives n'est imposée.
    """

    def __init__(self, before_call: Callable[[str], None] | None = None,
                 initial_delay: float = 0.5, max_delay: float = 8.0):
        if not (math.isfinite(initial_delay) and math.isfinite(max_delay)) or not (0 < initial_delay <= max_delay):
            raise ValueError("Délais de réessai invalides")
        self.before_call = before_call
        self.initial_delay = initial_delay
        self.max_delay = max_delay
        self.time = 0.0
        self.disabled: set[str] = set()
        self.pending: dict[str, RetryState] = {}
        self.faults: list[Fault] = []
        self.recoveries: list[str] = []
        self.total_recoveries = 0

    def advance(self, dt: float) -> None:
        if not math.isfinite(dt) or dt < 0:
            raise ValueError("Temps de supervision invalide")
        self.time += dt

    def can_retry(self, component: str) -> bool:
        return component not in self.disabled or self.time >= self.pending[component].next_retry

    def record(self, component: str, exc: Exception, essential: bool = False) -> None:
        previous = self.pending.get(component)
        attempts = 1 if previous is None else previous.attempts + 1
        delay = min(self.initial_delay * 2 ** min(attempts - 1, 20), self.max_delay)
        message = f"{type(exc).__name__}: {exc}"
        self.pending[component] = RetryState(attempts, self.time + delay,
                                              essential or bool(previous and previous.essential), message)
        self.disabled.add(component)
        logger.error("Défaillance de %s (tentative %d, nouvel essai dans %.2fs) : %s",
                     component, attempts, delay, exc, exc_info=exc)
        self.faults.append(Fault(component, message, essential))
        self.faults = self.faults[-8:]

    def success(self, component: str) -> None:
        previous = self.pending.pop(component, None)
        self.disabled.discard(component)
        if previous is not None:
            message = f"{component} réparé après {previous.attempts} échec(s)"
            logger.info("REPRISE : %s", message)
            self.total_recoveries += 1
            self.recoveries.append(message)
            self.recoveries = self.recoveries[-8:]

    def retry(self, component: str, action: Callable[[], T], *, essential: bool = False,
              prepare: Callable[[], None] | None = None) -> tuple[bool, T | None]:
        """Réexécute l'opération ; vrai uniquement si elle réussit effectivement."""
        if not self.can_retry(component):
            return False, None
        try:
            if component in self.disabled and prepare is not None:
                prepare()
            if self.before_call is not None:
                self.before_call(component)
            result = action()
        except Exception as exc:
            self.record(component, exc, essential)
            return False, None
        self.success(component)
        return True, result

    def optional(self, component: str, action: Callable[[], T], default: T = None,
                 on_retry: Callable[[], None] | None = None) -> T:  # type: ignore[assignment]
        ok, result = self.retry(component, action, prepare=on_retry)
        return result if ok else default  # type: ignore[return-value]

    def recover(self) -> None:
        """Réinitialisation MANUELLE (R), facultative pour fonctionner."""
        self.disabled.clear()
        self.pending.clear()
        self.faults.clear()
        self.recoveries.clear()
        # Total depuis le lancement conservé pour le bilan de la session.
