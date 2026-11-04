"""Construction exacte des quatre modèles 3D de la V6, sans contexte global ni Pygame."""
import math
from ..config import NOIR


def teinte(c, k):
    return (int(c[0]*k), int(c[1]*k), int(c[2]*k))


def melange(c1, c2, t):
    t = max(0.0, min(1.0, t))
    return tuple(int(c1[k] + (c2[k]-c1[k])*t) for k in range(3))


def anneau(x, demi_larg, z_bas, z_haut, n, dy=0):
    zc, hh = (z_bas + z_haut) / 2, (z_haut - z_bas) / 2
    return [(x, dy + demi_larg * math.cos(2 * math.pi * k / n), zc + hh * math.sin(2 * math.pi * k / n)) for k in range(n)]

def forme(sections, couleur, n, cap_arriere=None, cap_avant=None, dy=0):
    # on relie des anneaux successifs pour faire une forme ronde
    rings = [anneau(*s, n, dy) for s in sections]
    faces = []
    for a, b in zip(rings[:-1], rings[1:]):
        for k in range(n):
            th = 2 * math.pi * (k + 0.5) / n
            lum = 0.62 + 0.38 * max(0.0, 0.8 * math.sin(th) + 0.35 * math.cos(th))
            faces.append(([a[k], a[(k + 1) % n], b[(k + 1) % n], b[k]], teinte(couleur, lum)))
    if cap_arriere:
        faces.append((rings[0], cap_arriere))
    if cap_avant:
        faces.append((rings[-1], cap_avant))
    return faces

def boite(x0, x1, y0, y1, z0, z1, c, rot=None):
    x0, x1, y0, y1, z0, z1 = min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1), min(z0, z1), max(z0, z1)
    def R(p):
        x, y, z = p
        if rot:
            px, pz, th = rot
            dx, dz = x - px, z - pz
            x, z = px + dx * math.cos(th) + dz * math.sin(th), pz + dz * math.cos(th) - dx * math.sin(th)
        return (x, y, z)
    out = []
    for pts, k in [([(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)], 1.0),
                   ([(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0)], 0.45),
                   ([(x1,y0,z0),(x1,y1,z0),(x1,y1,z1),(x1,y0,z1)], 0.85),
                   ([(x0,y0,z0),(x0,y1,z0),(x0,y1,z1),(x0,y0,z1)], 0.95),
                   ([(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)], 0.6),
                   ([(x0,y1,z0),(x1,y1,z0),(x1,y1,z1),(x0,y1,z1)], 0.7)]:
        out.append(([R(p) for p in pts], teinte(c, k)))
    return out

def construire(idx, c):
    # fabrique le modèle 3D de la voiture avec la couleur choisie
    sombre = teinte(c, 0.65)
    S = []
    if idx == 0:   # ===== KART =====
        S += forme([(14, 10, 5, 10), (24, 9, 5, 9), (32, 4, 5, 8)], c, 6, cap_arriere=teinte(c, 0.8))
        for s in (-1, 1):
            S += forme([(-8, 3, 5, 12), (2, 5, 5, 13), (14, 3, 5, 10)], c, 6, dy=s * 15)
        S += boite(-22, 28, -14, 14, 3, 6, (40, 42, 48))
        S += boite(-13, -8, -8, 8, 6, 24, (43, 47, 54))
        S += forme([(-8, 6.5, 10, 26), (-2, 7, 10, 28), (4, 5.5, 10, 22)], (31, 79, 179), 6, cap_arriere=(31, 79, 179))
        S += forme([(-5, 3, 25, 31), (-1, 5.5, 23, 33), (3, 3, 25, 31)], (242, 194, 0), 8,
                   cap_arriere=(242, 194, 0), cap_avant=(242, 194, 0))
        S += boite(8, 9.5, -5, 5, 16, 19, NOIR)
        S += boite(-22, -14, -6, 6, 5, 15, (139, 145, 155))
        S += boite(-26, -22, 2, 5, 6, 10, (95, 100, 108))
        S += boite(-28, -25, -18, 18, 4, 8, c)
        S += boite(29, 32, -16, 16, 4, 8, c)
        S += boite(-16, -13, -17, 17, 6, 8, (107, 114, 128))
        S += boite(-28.6, -28.2, -8, 8, 10, 18, (245, 220, 60))
        return dict(statique=S, rj=0.6, pz=8, nez=33, phare_y=10, menu_dist=170, ombre=(-30, 34, 20), aile=None,
                    roues=[(-15, -18, False, 7.5, 6), (-15, 18, False, 7.5, 6), (17, -16, True, 6.5, 4), (17, 16, True, 6.5, 4)],
                    echap=[(-27, 3.5, 8)], feux_pos=[(-23, -3.5, 12.5), (-23, 3.5, 12.5)],
                    feux=[[(-22.3, s * 2, 11), (-22.3, s * 5, 11), (-22.3, s * 5, 14), (-22.3, s * 2, 14)] for s in (-1, 1)],
                    phares=[[(32.1, s * 10, 5), (32.1, s * 14, 5), (32.1, s * 14, 8), (32.1, s * 10, 8)] for s in (-1, 1)])
    if idx == 1:   # ===== 4x4 =====
        S += forme([(-44, 20, 14, 38), (-30, 22, 13, 42), (0, 22, 13, 44), (30, 22, 13, 42), (46, 18, 14, 34)], c, 8,
                   cap_arriere=teinte(c, 0.8))
        S += forme([(-26, 18, 42, 44), (-24, 19, 42, 60), (-2, 18, 42, 64), (16, 16, 42, 60), (26, 13, 42, 48)],
                   teinte(c, 0.85), 8, cap_arriere=(150, 190, 220), cap_avant=sombre)
        S += boite(-22, 14, -17, 17, 64, 66, (40, 43, 50))
        S += boite(12, 14, -13, 13, 66, 70, (60, 63, 70))
        S += forme([(-51, 12, 30, 54), (-45, 12, 30, 54)], (28, 28, 30), 10, cap_arriere=(95, 100, 108), cap_avant=(28, 28, 30))
        S += boite(-52, -45, -24, 24, 13, 24, (80, 85, 93))
        S += boite(-30, 30, -16, 16, 9, 13, (60, 64, 72))
        S += boite(-32, -28, -27, 27, 10, 14, (75, 80, 88))
        for s in (-1, 1):
            S += boite(-46, -45, s * 18, s * 29, 6, 20, NOIR)
            for cx in (-30, 30):
                S += boite(cx - 12, cx + 12, s * 21, s * 31, 25, 28, NOIR)
        S += boite(10, 12, 22, 25, 40, 64, NOIR)
        return dict(statique=S, rj=0.55, pz=26, nez=47, phare_y=12, menu_dist=250, ombre=(-54, 52, 28), aile=None,
                    roues=[(-30, -26, False, 13, 8), (-30, 26, False, 13, 8), (30, -26, True, 13, 8), (30, 26, True, 13, 8)],
                    echap=[(-46, 14, 17)], feux_pos=[(-45, -18, 40), (-45, 18, 40)],
                    feux=[[(-44.4, s * 15, 34), (-44.4, s * 21, 34), (-44.4, s * 21, 46), (-44.4, s * 15, 46)] for s in (-1, 1)],
                    phares=[[(46.3, s * 9, 24), (46.3, s * 16, 24), (46.3, s * 16, 30), (46.3, s * 9, 30)] for s in (-1, 1)]
                           + [[(14.2, -12, 66.5), (14.2, 12, 66.5), (14.2, 12, 69.5), (14.2, -12, 69.5)]])
    if idx == 2:   # ===== GT3 =====
        S += forme([(-36, 12, 6, 14), (-28, 16, 5, 19), (-14, 18, 5, 21), (4, 17, 5, 19), (22, 14, 5, 14),
                    (38, 9, 5, 10), (46, 4, 6, 8)], c, 8, cap_arriere=teinte(c, 0.8))
        S += forme([(-10, 8, 19, 21), (-6, 10, 18, 27), (4, 9, 18, 29), (12, 6, 17, 24), (16, 3, 17, 20)],
                   sombre, 6, cap_arriere=(110, 185, 235), cap_avant=sombre)
        S += boite(-39, -33, -15, 15, 3, 7, NOIR)
        S += boite(42, 50, -26, 26, 4, 7, NOIR)
        S += boite(-39, -30, -21, 21, 26, 28.5, NOIR)
        for s in (-1, 1):
            S += boite(-35, -32, s * 12 - 1.5, s * 12 + 1.5, 19, 26, NOIR)
            S += boite(-42, -28, s * 21, s * 21 + s * 1.5, 22, 33, NOIR)
            S += boite(-41, -37, s * 9 - 2, s * 9 + 2, 6, 10, (90, 90, 95))
        return dict(statique=S, rj=0.8, pz=10, nez=44, phare_y=12, menu_dist=210, ombre=(-40, 46, 23),
                    aile=dict(x0=-48, x1=-39, y0=-21, y1=21, z0=26, z1=27.5, px=-39, pz=26.5, couleur=(255, 210, 0)),
                    roues=[(-22, -21, False, 10, 7), (-22, 21, False, 10, 7), (22, -21, True, 10, 7), (22, 21, True, 10, 7)],
                    echap=[(-41, -9, 8), (-41, 9, 8)], feux_pos=[(-38, -11, 11), (-38, 11, 11)],
                    feux=[[(-36.4, s * 7, 10), (-36.4, s * 15, 10), (-36.4, s * 15, 13), (-36.4, s * 7, 13)] for s in (-1, 1)],
                    phares=[[(37.5, s * 13, 7), (37.5, s * 8, 7), (37.5, s * 8, 10), (37.5, s * 13, 10)] for s in (-1, 1)])
    # ===== WEC (hypercar) =====
    S += forme([(-56, 10, 5, 12), (-46, 18, 4, 17), (-30, 22, 4, 20), (-6, 21, 4, 21), (18, 19, 4, 16),
                (40, 15, 4, 11), (58, 6, 4, 7)], c, 8, cap_arriere=teinte(c, 0.8))
    for s in (-1, 1):
        S += forme([(-44, 6, 12, 24), (-30, 11, 10, 29), (-16, 6, 12, 24)], teinte(c, 0.95), 8, dy=s * 22)
        S += forme([(16, 6, 12, 24), (30, 11, 10, 28), (44, 6, 12, 23)], teinte(c, 0.95), 8, dy=s * 22)
    S += forme([(-14, 9, 19, 22), (-8, 12, 19, 31), (6, 11, 19, 33), (20, 7, 18, 26), (28, 3, 18, 21)],
               (30, 45, 70), 6, cap_arriere=(20, 30, 50), cap_avant=(30, 45, 70))
    aileron = [(-10, 30), (-56, 17), (-56, 50), (-34, 50)]
    S.append(([(x, 1.5, z) for x, z in aileron], teinte(c, 0.9)))
    S.append(([(x, -1.5, z) for x, z in aileron], teinte(c, 0.7)))
    S.append(([(-56, -1.5, 17), (-56, 1.5, 17), (-56, 1.5, 50), (-56, -1.5, 50)], teinte(c, 0.85)))
    S += boite(-62, -50, -22, 22, 48, 51, NOIR)
    S += boite(-60, -48, -18, 18, 4, 9, NOIR)
    S += boite(50, 62, -20, 20, 3, 5, NOIR)
    for k in (-12, 0, 12):
        S += boite(-61, -48, k - 0.8, k + 0.8, 4, 11, (60, 60, 66))
    for s in (-1, 1):
        S += boite(-67, -48, s * 22, s * 23.5, 38, 56, (20, 20, 24))
    return dict(statique=S, rj=0.8, pz=11, nez=60, phare_y=11, menu_dist=260, ombre=(-68, 64, 26),
                aile=dict(x0=-66, x1=-58, y0=-22, y1=22, z0=49, z1=51, px=-58, pz=50, couleur=(255, 212, 0)),
                roues=[(-30, -22, False, 11, 9), (-30, 22, False, 11, 9), (30, -22, True, 11, 9), (30, 22, True, 11, 9)],
                echap=[(-58, -4, 11), (-58, 4, 11)], feux_pos=[(-58, -13, 14), (-58, 13, 14)],
                feux=[[(-56.7, s * 9, 13), (-56.7, s * 17, 13), (-56.7, s * 17, 15.5), (-56.7, s * 9, 15.5)] for s in (-1, 1)],
                phares=[[(45, s * 9, 6), (45, s * 13, 6), (45, s * 13, 9), (45, s * 9, 9)] for s in (-1, 1)])

