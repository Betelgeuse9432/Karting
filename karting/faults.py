"""Isolation des fonctionnalités facultatives et diagnostics exploitables."""
from dataclasses import dataclass
import logging
from collections.abc import Callable
from typing import TypeVar

T = TypeVar("T")
logger = logging.getLogger("karting")


@dataclass(frozen=True)
class Fault:
    component: str
    message: str
    essential: bool


class FaultManager:
    def __init__(self):
        self.disabled: set[str] = set()
        self.faults: list[Fault] = []

    def record(self, component: str, exc: Exception, essential: bool = False) -> None:
        # Exception déjà capturée sur l'appelant ; logger.exception conserve le traceback.
        if component not in self.disabled:
            logger.error("Défaillance de %s : %s", component, exc, exc_info=exc)
            self.faults.append(Fault(component, f"{type(exc).__name__}: {exc}", essential))
            self.faults = self.faults[-8:]
        self.disabled.add(component)

    def optional(self, component: str, action: Callable[[], T], default: T = None) -> T:  # type: ignore[assignment]
        if component in self.disabled:
            return default
        try:
            return action()
        except Exception as exc:
            self.record(component, exc)
            return default

    def recover(self) -> None:
        """Réactive les composants lors d'un reset explicitement demandé."""
        self.disabled.clear()
        self.faults.clear()
