"""Primitives de rendu utilisées par les autres modules graphiques."""
import pygame
from ..camera import Camera
from ..config import RGB


def polygon(screen: pygame.Surface, camera: Camera, color: RGB,
            points: list[tuple[float, float, float]]) -> None:
    projected = camera.project(points)
    if projected:
        pygame.draw.polygon(screen, color, projected)


def blend(a: RGB, b: RGB, t: float) -> RGB:
    t = max(0, min(1, t))
    return tuple(int(a[k]+(b[k]-a[k])*t) for k in range(3))
