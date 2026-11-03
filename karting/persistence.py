"""Persistance isolée, erreurs propagées au superviseur plutôt qu'ignorées."""
import math
import os
from pathlib import Path
import tempfile


class RecordStore:
    def __init__(self, path: str | Path):
        self.path = Path(path)

    def load(self) -> float | None:
        if not self.path.exists():
            return None
        try:
            best = float(self.path.read_text(encoding="utf-8").strip())
        except ValueError as exc:
            raise ValueError(f"Record illisible : {self.path}") from exc
        if not math.isfinite(best) or best <= 0:
            raise ValueError(f"Record invalide : {best!r}")
        return best

    def save(self, value: float) -> None:
        if not math.isfinite(value) or value <= 0:
            raise ValueError(f"Temps à sauvegarder invalide : {value}")
        # Remplacement atomique afin d'éviter un record partiellement écrit.
        target = self.path
        parent = target.parent
        parent.mkdir(parents=True, exist_ok=True)
        temporary = None
        try:
            with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=parent,
                                             prefix=".karting_record_", delete=False) as f:
                temporary = f.name
                f.write(str(value))
            os.replace(temporary, target)
        finally:
            if temporary is not None and os.path.exists(temporary):
                os.unlink(temporary)
