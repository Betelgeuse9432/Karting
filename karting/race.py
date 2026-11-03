"""Chronométrage, progression en quatre secteurs et événements de tours."""
from .state import RaceState, VehicleState
from .track import Track


class RaceTracker:
    def __init__(self, track: Track, best: float | None = None):
        self.track = track
        self.state = RaceState(best=best)

    def reset(self) -> None:
        best = self.state.best
        self.state = RaceState(best=best)

    def step(self, car: VehicleState, dt: float) -> float | None:
        """Renvoie le temps d'un tour terminé, sinon None."""
        state = self.state
        state.max_speed = max(state.max_speed, abs(car.v))
        state.elapsed += dt
        index, _ = self.track.nearest(car.x, car.y)
        sector = index*4//self.track.count
        if sector != state.next_sector:
            return None
        lap = None
        if state.next_sector == 0:
            lap = state.elapsed
            state.laps += 1
            state.last = lap
            state.recent = (state.recent+[lap])[-5:]
            if state.best is None or lap < state.best:
                state.best = lap
            state.elapsed = 0.0
        state.next_sector = (state.next_sector+1) % 4
        return lap
