"""Transitions météo déterministes, sans dépendance à Pygame."""


def step_weather(meteo: dict, dt: float) -> tuple[float, float]:
    """Interpoler progressivement la nuit et la pluie, sans saut d'image."""
    if not 0.0 <= dt <= 0.05:
        raise ValueError("Pas de simulation météo invalide")
    phase = int(meteo["phase"])
    if phase not in (0, 1, 2):
        raise ValueError("Phase de ciel invalide")
    meteo["nuit"] += ((0.0, 0.55, 1.0)[phase] - meteo["nuit"]) * min(1, 0.8 * dt)
    meteo["pluie"] += ((1.0 if meteo["pluie_on"] else 0.0) - meteo["pluie"]) * min(1, 0.6 * dt)
    return meteo["nuit"], meteo["pluie"]
