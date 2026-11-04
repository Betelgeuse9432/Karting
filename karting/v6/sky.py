"""Ciel V6 : nuages, montagnes, soleil, lune, étoiles et dégradés météo."""
import math
import random
import pygame
from .mesh import melange


def sky_palette(weather):
    n, pluie=weather["nuit"],weather["pluie"]
    phases=[((84,150,235),(190,224,255)),
            ((88,66,128),(255,156,88)),
            ((5,8,28),(22,28,66))]
    if n<0.55:
        a,b,t=phases[0],phases[1],n/0.55
    else:
        a,b,t=phases[1],phases[2],(n-0.55)/0.45
    haut,bas=melange(a[0],b[0],t),melange(a[1],b[1],t)
    grey=int(135*(1-0.85*n))
    c=(grey-5,grey,grey+10)
    return melange(haut,c,0.65*pluie),melange(bas,c,0.52*pluie)


def ecart_angle(a,b):
    return (a-b+math.pi)%(2*math.pi)-math.pi


def glow(colour):
    surface=pygame.Surface((256,256))
    surface.fill((0,0,0))
    for radius in range(128,0,-3):
        intensity=(1-radius/128)**2.2
        pygame.draw.circle(surface,tuple(int(c*intensity) for c in colour),(128,128),radius)
    return surface


class SkyRenderer:
    def __init__(self,seed=7):
        rng=random.Random(seed)
        self.stars=[(rng.uniform(0,2*math.pi),rng.uniform(0.05,0.95)) for _ in range(120)]
        self.clouds=[(rng.uniform(0,2*math.pi),rng.uniform(0,0.85),
                      rng.uniform(70,150),rng.random()) for _ in range(18)]
        self.sun=glow((255,215,140))
        self.moon=glow((150,170,230))

    def draw(self,screen,camera,weather,quality,temps):
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        QUALITE=quality
        ETOILES,NUAGES=self.stars,self.clouds
        SOLEIL_B,LUNE_B=0.9,3.9
        LUEUR_SOLEIL,LUEUR_LUNE=self.sun,self.moon
        ecran=screen
        etat={"cam":camera.angle}
        haut,bas=sky_palette(weather)
        nuit,pluie=weather["nuit"],weather["pluie"]
    for y in range(0, HORIZON, 4):                  # le dégradé du ciel
        pygame.draw.rect(ecran, melange(haut, bas, (y / HORIZON) ** 1.5), (0, y, W, 4))
    if nuit > 0.5 and pluie < 0.6:                  # les étoiles
        lum = int(255 * min(1, (nuit - 0.5) * 2) * (1 - pluie))
        for gis, elev in ETOILES:
            d = ecart_angle(gis, etat["cam"])
            if abs(d) < 1.2:
                pygame.draw.circle(ecran, (lum, lum, lum), (W / 2 + math.tan(d) * FOCAL, HORIZON - 15 - elev * (HORIZON - 40)), 1)
    vis_soleil = (1 - pluie) * max(0.0, 1 - nuit / 0.75)     # le soleil (il descend le soir)
    if vis_soleil > 0.05:
        d = ecart_angle(SOLEIL_B, etat["cam"])
        if abs(d) < 1.3:
            sx, sy = W / 2 + math.tan(d) * FOCAL, HORIZON - 30 - 170 * max(0.0, 1 - nuit / 0.6)
            ecran.blit(LUEUR_SOLEIL, (sx - 128, sy - 128), special_flags=pygame.BLEND_RGB_ADD)
            pygame.draw.circle(ecran, melange((255, 252, 225), (255, 150, 70), nuit / 0.6), (sx, sy), 26)
    vis_lune = (1 - pluie) * max(0.0, (nuit - 0.6) / 0.4)    # la lune
    if vis_lune > 0.05:
        d = ecart_angle(LUNE_B, etat["cam"])
        if abs(d) < 1.3:
            sx, sy = W / 2 + math.tan(d) * FOCAL, HORIZON - 170
            ecran.blit(LUEUR_LUNE, (sx - 128, sy - 128), special_flags=pygame.BLEND_RGB_ADD)
            pygame.draw.circle(ecran, (236, 240, 252), (sx, sy), 17)
    if QUALITE >= 1:                                # les nuages
        clair = melange(melange((255, 255, 255), haut, 0.12 + nuit * 0.85), (112, 118, 130), pluie * 0.75)
        ombre = melange(clair, (90, 100, 128), 0.38 + 0.4 * pluie)
        for gis, elev, taille, graine in NUAGES:
            d = ecart_angle(gis + temps * 0.004, etat["cam"])
            if abs(d) > 1.35:
                continue
            cx, cy = W / 2 + math.tan(d) * FOCAL, HORIZON - 60 - elev * (HORIZON - 150)
            for ox, oy, rr in [(-1.0, 0.2, 0.8), (-0.4, -0.2, 1.0), (0.3, -0.3, 1.1), (1.0, 0.1, 0.8), (0.0, 0.25, 0.9)]:
                pygame.draw.circle(ecran, ombre, (cx + ox * taille, cy + oy * taille * 0.6 + 7), rr * taille * 0.46)
            for ox, oy, rr in [(-1.0, 0.2, 0.8), (-0.4, -0.2, 1.0), (0.3, -0.3, 1.1), (1.0, 0.1, 0.8), (0.0, 0.25, 0.9)]:
                pygame.draw.circle(ecran, clair, (cx + ox * taille, cy + oy * taille * 0.6), rr * taille * 0.42)
    couches = [(120, 40, (1.3, 3.1, 7.0), (112, 132, 172), 0.60), (82, 18, (2.1, 4.7, 9.5), (80, 104, 132), 0.40),
               (48, 0, (3.0, 6.1, 13.0), (50, 92, 70), 0.20)]
    for (amp, base, fr, coul, brume) in couches[3 - (QUALITE + 1):]:
        c = melange(melange(coul, bas, brume + 0.2 * pluie), (6, 8, 22), nuit * 0.55)
        pts = [(W, HORIZON), (0, HORIZON)]
        for x in range(0, W + 9, 8):
            b = etat["cam"] + math.atan((x - W / 2) / FOCAL)
            pts.append((x, HORIZON - base - amp * (0.55 + 0.3 * math.sin(b * fr[0] + 1.3) + 0.2 * math.sin(b * fr[1] + 0.4) + 0.12 * math.sin(b * fr[2]))))
        pygame.draw.polygon(ecran, c, pts)
        return haut,bas
