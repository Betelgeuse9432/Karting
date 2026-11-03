"""Dessin du kart : géométrie V3, aucun calcul physique ici."""
import math
import pygame
from ..camera import Camera
from ..config import GameConfig, NOIR
from ..state import VehicleState
from .common import blend, polygon


class VehicleRenderer:
    def __init__(self, config: GameConfig):
        self.config = config

    def draw(self, screen: pygame.Surface, camera: Camera, car: VehicleState) -> None:
        faces = []
        tangage, roulis = car.pitch, car.roll

        def face(pts, couleur, susp=True, biais=0):
            if susp:   # la carrosserie plonge au freinage et penche en virage
                pts = [(x, y, z - tangage * x + roulis * y) for x, y, z in pts]
            mondes = [camera.space(*car.local_to_world(*p)) for p in pts]
            faces.append((sum(m[0] for m in mondes) / len(mondes) - biais, couleur, mondes))

        def teinte(c, k):
            return (int(c[0] * k), int(c[1] * k), int(c[2] * k))

        def box(x0, x1, y0, y1, z0, z1, c, rot=None):
            def R(p):
                x, y, z = p
                if rot:
                    px, pz, th = rot
                    dx, dz = x - px, z - pz
                    x, z = px + dx * math.cos(th) + dz * math.sin(th), pz + dz * math.cos(th) - dx * math.sin(th)
                return (x, y, z)
            for pts, k in [([(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)], 1.0),
                           ([(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0)], 0.45),
                           ([(x1,y0,z0),(x1,y1,z0),(x1,y1,z1),(x1,y0,z1)], 0.85),
                           ([(x0,y0,z0),(x0,y1,z0),(x0,y1,z1),(x0,y0,z1)], 0.95),
                           ([(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)], 0.6),
                           ([(x0,y1,z0),(x1,y1,z0),(x1,y1,z1),(x0,y1,z1)], 0.7)]:
                face([R(p) for p in pts], teinte(c, k))

        def roue(cx, cy, braque, chaud):
            n, rayon, demi = 10, 9, 6
            cb, sb = math.cos(braque), math.sin(braque)
            def pt(theta, y, r=1.0):
                dx, dz = rayon * r * math.cos(theta), rayon * r * math.sin(theta)
                return (cx + dx * cb - y * sb, cy + dx * sb + y * cb, 9 + dz)
            for k in range(n):
                t0 = car.wheel_rotation + k * 2 * math.pi / n
                t1 = t0 + 2 * math.pi / n
                base = (38, 38, 42) if k % 2 == 0 else (62, 62, 68)
                face([pt(t0, -demi), pt(t1, -demi), pt(t1, demi), pt(t0, demi)],
                     teinte(base, 0.6 + 0.4 * max(0, math.sin((t0 + t1) / 2))), susp=False)
            jante = blend((150, 150, 158), (255, 90, 20), chaud)       # le disque rougit quand il chauffe
            rayon_c = blend((225, 225, 230), (255, 200, 60), chaud)
            for y in (-demi, demi):
                face([pt(car.wheel_rotation + k * 2 * math.pi / n, y) for k in range(n)], jante, susp=False)
                for m in range(4):
                    a = car.wheel_rotation + m * math.pi / 2
                    face([pt(a, y, 0.9), pt(a + 0.25, y, 0.2), pt(a - 0.25, y, 0.2)], rayon_c, susp=False, biais=0.3)

        c = self.config.color_car
        sombre = teinte(c, 0.65)
        # roues (les roues avant tournent avec le volant, toutes tournent avec la vitesse)
        for cx, cy, avant in [(-22, -21, False), (-22, 21, False), (22, -21, True), (22, 21, True)]:
            roue(cx, cy, car.steering if avant else 0, car.heat if avant else 0)
        box(-34, 38, -17, 17, 5, 15, c)                    # carrosserie
        box(26, 46, -9, 9, 5, 11, sombre)                  # nez
        box(42, 48, -26, 26, 4, 7, NOIR)                   # aileron avant
        box(-8, 16, -11, 11, 15, 28, sombre)               # cabine
        face([(-8.3, -9, 17), (-8.3, 9, 17), (-8.3, 9, 26), (-8.3, -9, 26)], (120, 190, 240))   # lunette arrière
        box(-37, -30, -15, 15, 3, 7, NOIR)                 # diffuseur
        feux = (255, 60, 60) if car.brake > 0.1 else (130, 0, 0)
        for s in (-1, 1):
            box(-38, -34, s * 9 - 2, s * 9 + 2, 6, 10, (90, 90, 95))      # échappements
            box(-35.5, -34, s * 13 - 3, s * 13 + 3, 10, 13, feux)         # feux de frein
            box(-33, -30, s * 12 - 1.5, s * 12 + 1.5, 15, 25, NOIR)       # supports de l'aileron
        box(-36, -28, -20, 20, 25, 27, NOIR)                              # profil principal
        box(-44, -36, -20, 20, 25, 26.5, (255, 210, 0), rot=(-36, 25.5, car.wing))   # LE VOLET MOBILE 😎
        # ombre sous la voiture
        polygon(screen, camera, (25, 25, 28), [camera.ground(*car.local_to_world(x, y, 0)[:2]) for x, y in [(-36, -20), (40, -20), (40, 20), (-36, 20)]])
        faces.sort(key=lambda f: f[0], reverse=True)
        for _, couleur, pts in faces:
            polygon(screen, camera, couleur, pts)

