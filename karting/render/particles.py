"""Traces de pneus et particules, projection sans modification des effets."""
import pygame
from ..camera import Camera
from ..effects import Effects, Particle


class EffectsRenderer:
    def __init__(self):
        self.circle = pygame.Surface((64, 64), pygame.SRCALPHA)
        pygame.draw.circle(self.circle, (225, 225, 225), (32, 32), 32)

    def draw_marks(self, screen: pygame.Surface, camera: Camera, effects: Effects) -> None:
        cfg = camera.cfg
        for x1, y1, x2, y2 in effects.marks:
            f1, r1 = camera.position(x1, y1)
            f2, r2 = camera.position(x2, y2)
            if cfg.near < f1 < cfg.far and f2 > cfg.near:
                pygame.draw.line(screen, (28, 28, 30),
                                 (cfg.width/2 + r1/f1*cfg.focal, cfg.horizon+cfg.camera_height/f1*cfg.focal),
                                 (cfg.width/2 + r2/f2*cfg.focal, cfg.horizon+cfg.camera_height/f2*cfg.focal),
                                 max(1, int(5*cfg.focal/f1)))

    def draw_particle(self, screen: pygame.Surface, camera: Camera, forward: float, p: Particle) -> None:
        cfg = camera.cfg
        scale = cfg.focal/forward
        sx = cfg.width/2+camera.position(p.x, p.y)[1]*scale
        sy = cfg.horizon+(cfg.camera_height-p.z)*scale
        age = max(0, min(1, p.lifetime/p.initial_lifetime))
        if p.kind == "etincelle":
            pygame.draw.circle(screen, (255, int(120+135*age), int(40*age)), (sx,sy), max(1, 2.5*scale))
        elif p.kind == "flamme":
            pygame.draw.circle(screen, (255, int(70+170*age), int(20+100*age*age)),
                               (sx,sy), max(1, p.size*age*scale))
        else:
            diameter = int(max(2, min(300, 2*p.size*scale)))
            picture = pygame.transform.scale(self.circle, (diameter, diameter))
            picture.set_alpha(int(110*age))
            screen.blit(picture, (sx-diameter/2, sy-diameter/2))
