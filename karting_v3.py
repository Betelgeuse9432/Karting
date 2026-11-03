# 🏎️ MINI KARTING V3 - voiture 3D, stands, fumée, étincelles, flammes...
# Lancer :  python karting_v3.py
# Commandes : ⬆️ ou Z/W = gaz | ⬇️ ou S = frein / marche arrière | ⬅️ ➡️ ou Q/A D = tourner
#             R = recommencer | Échap = quitter
#
# 🎓 À APPRENDRE :
#  - GAZ : la pédale s'enfonce doucement (maintiens la touche) mais les gaz sont TRÈS puissants.
#    Trop de gaz en virage = l'ARRIÈRE patine (fumée + traces) et la voiture pivote trop.
#  - FREIN : il agit sur l'AVANT et s'enfonce progressivement. Trop de frein en virage =
#    l'AVANT glisse et la voiture refuse de tourner. Freinage fort = disques chauds = ÉTINCELLES !
#  - APPUI AÉRO : plus tu vas vite, plus l'aileron plaque la voiture au sol (plus de grip).
#    Au freinage, le volet de l'aileron se relève pour t'aider à ralentir.
#  - STANDS : roule dans la voie des stands (à droite de la ligne de départ) pour voir tes scores.

import pygame
import math
import random
from collections import deque

# ===== 1. RÉGLAGES (change-les pour t'amuser !) =====
W, H = 900, 650
VITESSE_MAX = 720          # vitesse max (pixels par seconde)
PUISSANCE = 1500           # puissance des gaz (très forte !)
PEDALE_GAZ = 1.0           # enfoncement de la pédale de gaz (petit = plus doux)
PEDALE_FREIN = 1.6         # enfoncement de la pédale de frein (petit = plus progressif)
FREINAGE = 1200            # force max du frein (sur l'AVANT)
GRIP_AV = 1000             # adhérence des pneus AVANT (frein + virage)
GRIP_AR = 1100             # adhérence des pneus ARRIÈRE (gaz + virage)
APPUI = 0.5                # appui aérodynamique à pleine vitesse (0.5 = +50% de grip)
EMPATTEMENT = 44           # distance entre essieux
DEMI_LARGEUR = 95          # demi-largeur de la piste (plus large qu'avant !)
ECHELLE_CIRCUIT = 0.8      # 1.0 = piste plus longue, 0.7 = plus courte
COULEUR_VOITURE = (220, 30, 30)

BRUT = [(1800, 500), (2800, 500), (3500, 600), (3900, 1000), (3800, 1500), (3300, 1800),
        (3600, 2300), (3200, 2650), (2600, 2600), (2200, 2200), (1700, 2000), (1300, 2300),
        (800, 2600), (400, 2200), (500, 1600), (1000, 1300), (800, 900), (1100, 600)]
POINTS = [(x * ECHELLE_CIRCUIT, y * ECHELLE_CIRCUIT) for x, y in BRUT]
MONDE = (int(4500 * ECHELLE_CIRCUIT), int(3100 * ECHELLE_CIRCUIT))

HORIZON, FOCAL, CAMH, CAMDIST = int(H * 0.40), 520, 65, 150
NEAR, FAR = 60, 1500
FICHIER_RECORD = "record_karting.txt"

CIEL, CIEL2 = (110, 175, 255), (170, 215, 255)
HERBE, HERBE2 = (40, 150, 40), (34, 130, 34)
ASPHALTE, ASPHALTE2 = (80, 80, 85), (72, 72, 77)
ROUGE, BLANC, NOIR = (220, 30, 30), (240, 240, 240), (25, 25, 25)

# ===== 2. CRÉATION DE LA PISTE =====
def lisser(pts, pas=45):
    n, res = len(pts), []
    for i in range(n):
        p0, p1, p2, p3 = pts[i - 1], pts[i], pts[(i + 1) % n], pts[(i + 2) % n]
        for k in range(pas):
            t = k / pas
            x = 0.5 * (2*p1[0] + (-p0[0]+p2[0])*t + (2*p0[0]-5*p1[0]+4*p2[0]-p3[0])*t**2 + (-p0[0]+3*p1[0]-3*p2[0]+p3[0])*t**3)
            y = 0.5 * (2*p1[1] + (-p0[1]+p2[1])*t + (2*p0[1]-5*p1[1]+4*p2[1]-p3[1])*t**2 + (-p0[1]+3*p1[1]-3*p2[1]+p3[1])*t**3)
            res.append((x, y))
    return res

centre = lisser(POINTS)
N = len(centre)

def plus_proche(x, y):
    i = min(range(N), key=lambda k: (centre[k][0]-x)**2 + (centre[k][1]-y)**2)
    return i, math.hypot(centre[i][0]-x, centre[i][1]-y)

KERB = 16
DECALAGES = [DEMI_LARGEUR + KERB, DEMI_LARGEUR, -DEMI_LARGEUR, -(DEMI_LARGEUR + KERB), 2, -2]
LIGNES = [[] for _ in DECALAGES]
NORMALES = []
for i in range(N):
    x0, y0 = centre[i - 1]
    x1, y1 = centre[(i + 1) % N]
    l = math.hypot(x1 - x0, y1 - y0) or 1
    nx, ny = -(y1 - y0) / l, (x1 - x0) / l
    NORMALES.append((nx, ny))
    for k, d in enumerate(DECALAGES):
        LIGNES[k].append((centre[i][0] + nx * d, centre[i][1] + ny * d))

# --- Les STANDS : voie des stands + garages, à droite de la ligne de départ ---
PIT_D1 = DEMI_LARGEUR + KERB + 4          # début de la voie des stands
PIT_D2 = PIT_D1 + 110                     # fin de la voie
PIT_FIN = PIT_D2 + 6
PIT0, PITLEN = N - 45, 95
PITS = [(PIT0 + m) % N for m in range(PITLEN)]
PITSET = set(PITS)
def pos(i, d):
    return (centre[i][0] + NORMALES[i][0] * d, centre[i][1] + NORMALES[i][1] * d)
LIGNES_PIT = [[pos(i, d) for i in range(N)] for d in (PIT_D1, PIT_D2 - 3, PIT_D2)]
COULEURS_PORTES = [(220, 40, 40), (40, 90, 220), (240, 200, 30), (40, 170, 90), (230, 120, 20), (150, 60, 200)]
GARAGES = []
for g, m in enumerate(range(2, PITLEN - 12, 14)):
    i0, i1 = PITS[m], PITS[m + 10]
    GARAGES.append((pos(i0, PIT_D2 + 30), pos(i1, PIT_D2 + 30), pos(i0, PIT_D2 + 130), pos(i1, PIT_D2 + 130),
                    COULEURS_PORTES[g % 6], PITS[m + 5]))

def info_piste(x, y):
    n, d = plus_proche(x, y)
    dec = (x - centre[n][0]) * NORMALES[n][0] + (y - centre[n][1]) * NORMALES[n][1]
    return n, d, dec

# --- Les arbres ---
random.seed(7)
ARBRES = []
for i in range(0, N, 5):
    for cote in (1, -1):
        d = DEMI_LARGEUR + KERB + 60 + random.random() * 300
        x, y = centre[i][0] + NORMALES[i][0] * d * cote, centre[i][1] + NORMALES[i][1] * d * cote
        if 0 < x < MONDE[0] and 0 < y < MONDE[1]:
            n, dist, dec = info_piste(x, y)
            if dist > DEMI_LARGEUR + KERB + 50 and not (n in PITSET and 0 < dec < 450):
                ARBRES.append((x, y, random.randint(0, 1), 0.8 + random.random() * 0.6))
for _ in range(220):
    x, y = random.random() * MONDE[0], random.random() * MONDE[1]
    n, dist, dec = info_piste(x, y)
    if dist > DEMI_LARGEUR + KERB + 80 and not (n in PITSET and 0 < dec < 450):
        ARBRES.append((x, y, random.randint(0, 1), 0.8 + random.random() * 0.6))

# ===== 3. MISE EN PLACE =====
pygame.init()
ecran = pygame.display.set_mode((W, H))
pygame.display.set_caption("Mini Karting V3")
horloge = pygame.time.Clock()
police = pygame.font.SysFont("monospace", 22, bold=True)
petite = pygame.font.SysFont("monospace", 16, bold=True)
grande = pygame.font.SysFont("monospace", 34, bold=True)

ECH = 0.05
mini = pygame.Surface((int(MONDE[0]*ECH)+10, int(MONDE[1]*ECH)+10), pygame.SRCALPHA)
mini.fill((0, 0, 0, 120))
pygame.draw.lines(mini, (255, 255, 255), True, [(x*ECH+5, y*ECH+5) for x, y in centre], 4)

CERCLE = pygame.Surface((64, 64), pygame.SRCALPHA)
pygame.draw.circle(CERCLE, (225, 225, 225), (32, 32), 32)

def lire_record():
    try:
        return float(open(FICHIER_RECORD).read())
    except Exception:
        return None

def sauver_record(t):
    try:
        open(FICHIER_RECORD, "w").write(str(t))
    except Exception:
        pass

def reset():
    angle = math.atan2(centre[1][1]-centre[0][1], centre[1][0]-centre[0][0])
    return dict(x=centre[0][0], y=centre[0][1], angle=angle, cam=angle, v=0.0, volant=0.0,
                gaz=0.0, frein=0.0, chaleur=0.0, tangage=0.0, roulis=0.0, aile=0.1, roue=0.0,
                suivant=1, tours=0, chrono=0.0, meilleur=lire_record(), dernier=None, liste=[], vmax=0.0)

etat = reset()
particules = []                    # fumée, flammes, étincelles
marques = deque(maxlen=1600)       # traces de pneus sur le sol
derniere = [None] * 4

# ===== 4. CAMÉRA 3D =====
camx = camy = cs = sn = 0.0

def cam(x, y):
    dx, dy = x - camx, y - camy
    return dx * cs + dy * sn, -dx * sn + dy * cs

def c3(x, y, z):
    f, r = cam(x, y)
    return (f, r, z)

def sol(x, y):
    return c3(x, y, 0)

def clamp(v):
    return max(-20000, min(20000, v))

def projeter(pts):
    res = []
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        a_ok, b_ok = a[0] >= NEAR, b[0] >= NEAR
        if a_ok:
            res.append(a)
        if a_ok != b_ok:
            t = (NEAR - a[0]) / (b[0] - a[0])
            res.append((NEAR, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t))
    if len(res) < 3:
        return None
    return [(clamp(W / 2 + r / f * FOCAL), clamp(HORIZON + (CAMH - z) / f * FOCAL)) for f, r, z in res]

def poly(couleur, pts):
    p = projeter(pts)
    if p:
        pygame.draw.polygon(ecran, couleur, p)

def melange(c1, c2, t):
    t = max(0, min(1, t))
    return tuple(int(c1[k] + (c2[k] - c1[k]) * t) for k in range(3))

# ===== 5. LA VOITURE EN 3D (faite de petits blocs) =====
def loc2mon(lx, ly, lz):
    """Position dans la voiture (avant, droite, haut) -> position dans le monde"""
    ca, sa = math.cos(etat["angle"]), math.sin(etat["angle"])
    return etat["x"] + ca * lx - sa * ly, etat["y"] + sa * lx + ca * ly, lz

def dessiner_voiture():
    faces = []
    tangage, roulis = etat["tangage"], etat["roulis"]

    def face(pts, couleur, susp=True, biais=0):
        if susp:   # la carrosserie plonge au freinage et penche en virage
            pts = [(x, y, z - tangage * x + roulis * y) for x, y, z in pts]
        mondes = [c3(*loc2mon(*p)) for p in pts]
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
            t0 = etat["roue"] + k * 2 * math.pi / n
            t1 = t0 + 2 * math.pi / n
            base = (38, 38, 42) if k % 2 == 0 else (62, 62, 68)
            face([pt(t0, -demi), pt(t1, -demi), pt(t1, demi), pt(t0, demi)],
                 teinte(base, 0.6 + 0.4 * max(0, math.sin((t0 + t1) / 2))), susp=False)
        jante = melange((150, 150, 158), (255, 90, 20), chaud)       # le disque rougit quand il chauffe
        rayon_c = melange((225, 225, 230), (255, 200, 60), chaud)
        for y in (-demi, demi):
            face([pt(etat["roue"] + k * 2 * math.pi / n, y) for k in range(n)], jante, susp=False)
            for m in range(4):
                a = etat["roue"] + m * math.pi / 2
                face([pt(a, y, 0.9), pt(a + 0.25, y, 0.2), pt(a - 0.25, y, 0.2)], rayon_c, susp=False, biais=0.3)

    c = COULEUR_VOITURE
    sombre = teinte(c, 0.65)
    # roues (les roues avant tournent avec le volant, toutes tournent avec la vitesse)
    for cx, cy, avant in [(-22, -21, False), (-22, 21, False), (22, -21, True), (22, 21, True)]:
        roue(cx, cy, etat["volant"] if avant else 0, etat["chaleur"] if avant else 0)
    box(-34, 38, -17, 17, 5, 15, c)                    # carrosserie
    box(26, 46, -9, 9, 5, 11, sombre)                  # nez
    box(42, 48, -26, 26, 4, 7, NOIR)                   # aileron avant
    box(-8, 16, -11, 11, 15, 28, sombre)               # cabine
    face([(-8.3, -9, 17), (-8.3, 9, 17), (-8.3, 9, 26), (-8.3, -9, 26)], (120, 190, 240))   # lunette arrière
    box(-37, -30, -15, 15, 3, 7, NOIR)                 # diffuseur
    feux = (255, 60, 60) if etat["frein"] > 0.1 else (130, 0, 0)
    for s in (-1, 1):
        box(-38, -34, s * 9 - 2, s * 9 + 2, 6, 10, (90, 90, 95))      # échappements
        box(-35.5, -34, s * 13 - 3, s * 13 + 3, 10, 13, feux)         # feux de frein
        box(-33, -30, s * 12 - 1.5, s * 12 + 1.5, 15, 25, NOIR)       # supports de l'aileron
    box(-36, -28, -20, 20, 25, 27, NOIR)                              # profil principal
    box(-44, -36, -20, 20, 25, 26.5, (255, 210, 0), rot=(-36, 25.5, etat["aile"]))   # LE VOLET MOBILE 😎
    # ombre sous la voiture
    poly((25, 25, 28), [sol(*loc2mon(x, y, 0)[:2]) for x, y in [(-36, -20), (40, -20), (40, 20), (-36, 20)]])
    faces.sort(key=lambda f: f[0], reverse=True)
    for _, couleur, pts in faces:
        poly(couleur, pts)

# ===== 6. EFFETS : fumée, flammes, étincelles =====
def emettre(genre, lx, ly, lz, vx, vy, vz, vie, taille):
    if len(particules) > 450:
        return
    wx, wy, wz = loc2mon(lx, ly, lz)
    ca, sa = math.cos(etat["angle"]), math.sin(etat["angle"])
    inertie = {"fumee": 0.2, "flamme": 0.5, "etincelle": 0.7}[genre] * etat["v"]
    particules.append([wx, wy, wz, ca * vx - sa * vy + ca * inertie, sa * vx + ca * vy + sa * inertie, vz, vie, vie, genre, taille])

def mettre_a_jour_particules(dt):
    for p in particules:
        p[0] += p[3] * dt
        p[1] += p[4] * dt
        p[2] += p[5] * dt
        p[6] -= dt
        if p[8] == "etincelle":
            p[5] -= 500 * dt
            if p[2] < 0:
                p[2], p[5] = 0, -p[5] * 0.3
        elif p[8] == "fumee":
            p[9] += 18 * dt
    particules[:] = [p for p in particules if p[6] > 0]

def dessiner_particule(f, p):
    s = FOCAL / f
    sx, sy = W / 2 + cam(p[0], p[1])[1] * s, HORIZON + (CAMH - p[2]) * s
    vie = p[6] / p[7]
    if p[8] == "etincelle":
        pygame.draw.circle(ecran, (255, int(120 + 135 * vie), int(40 * vie)), (sx, sy), max(1, 2.5 * s))
    elif p[8] == "flamme":
        pygame.draw.circle(ecran, (255, int(70 + 170 * vie), int(20 + 100 * vie * vie)), (sx, sy), max(1, p[9] * vie * s))
    else:
        d = int(max(2, min(300, 2 * p[9] * s)))
        image = pygame.transform.scale(CERCLE, (d, d))
        image.set_alpha(int(110 * vie))
        ecran.blit(image, (sx - d / 2, sy - d / 2))

# ===== 7. DÉCORS =====
def dessiner_arbre(f, r, genre, taille):
    s = FOCAL / f * taille
    sx, sy = W / 2 + r / f * FOCAL, HORIZON + CAMH / f * FOCAL
    if sx < -150 or sx > W + 150:
        return
    pygame.draw.rect(ecran, (95, 60, 30), (sx - 5 * s, sy - 35 * s, 10 * s, 35 * s))
    if genre == 0:
        pygame.draw.polygon(ecran, (20, 90, 40), [(sx - 40*s, sy - 25*s), (sx + 40*s, sy - 25*s), (sx, sy - 110*s)])
        pygame.draw.polygon(ecran, (30, 110, 50), [(sx - 30*s, sy - 70*s), (sx + 30*s, sy - 70*s), (sx, sy - 150*s)])
    else:
        pygame.draw.circle(ecran, (50, 140, 50), (sx, sy - 85 * s), 45 * s)
        pygame.draw.circle(ecran, (70, 165, 60), (sx - 12*s, sy - 95*s), 25 * s)

def milieu(a, b, t):
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)

def dessiner_garage(g):
    A0, A1, B0, B1, porte, _ = g
    h = 55
    poly((60, 60, 70), [c3(*A0, h), c3(*A1, h), c3(*B1, h), c3(*B0, h)])           # toit
    poly((205, 205, 210), [c3(*A0, 0), c3(*A1, 0), c3(*A1, h), c3(*A0, h)])        # mur
    d0, d1 = milieu(A0, A1, 0.15), milieu(A0, A1, 0.85)
    poly(porte, [c3(*d0, 0), c3(*d1, 0), c3(*d1, 38), c3(*d0, 38)])                # porte du garage

def dessiner_portique():
    pa, pb = LIGNES[0][0], LIGNES[3][0]
    (fa, ra), (fb, rb) = cam(*pa), cam(*pb)
    if min(fa, fb) < NEAR:
        return
    xa, xb = W / 2 + ra / fa * FOCAL, W / 2 + rb / fb * FOCAL
    sa, sb = HORIZON + CAMH / fa * FOCAL, HORIZON + CAMH / fb * FOCAL
    ha, hb = HORIZON + (CAMH - 170) / fa * FOCAL, HORIZON + (CAMH - 170) / fb * FOCAL
    ea, eb = 45 * FOCAL / fa, 45 * FOCAL / fb
    pygame.draw.line(ecran, (200, 200, 200), (xa, sa), (xa, ha), max(2, int(9 * FOCAL / fa)))
    pygame.draw.line(ecran, (200, 200, 200), (xb, sb), (xb, hb), max(2, int(9 * FOCAL / fb)))
    for k in range(14):
        t0, t1 = k / 14, (k + 1) / 14
        x0, x1 = xa + (xb - xa) * t0, xa + (xb - xa) * t1
        y0, y1 = ha + (hb - ha) * t0, ha + (hb - ha) * t1
        e0, e1 = ea + (eb - ea) * t0, ea + (eb - ea) * t1
        pygame.draw.polygon(ecran, BLANC if k % 2 == 0 else NOIR, [(x0, y0), (x1, y1), (x1, y1 + e1), (x0, y0 + e0)])

# ===== 8. TABLEAU DES SCORES DES STANDS =====
def fmt(t):
    return "--" if t is None else f"{t:.2f}s"

def afficher_stands():
    panneau = pygame.Surface((580, 400), pygame.SRCALPHA)
    panneau.fill((10, 10, 25, 220))
    pygame.draw.rect(panneau, (255, 210, 0), (0, 0, 580, 400), 4)
    ecran.blit(panneau, (W // 2 - 290, H // 2 - 200))
    lignes = [("STANDS - TABLEAU DES SCORES", (255, 210, 0), grande),
              (f"Tour en cours   : {etat['tours'] + 1}", (255, 255, 255), police),
              (f"Dernier tour    : {fmt(etat['dernier'])}", (255, 255, 255), police),
              (f"Meilleur tour   : {fmt(etat['meilleur'])}", (255, 220, 0), police),
              (f"Vitesse max     : {int(etat['vmax'] * 0.3)} km/h", (255, 255, 255), police),
              (f"Tours termines  : {etat['tours']}", (255, 255, 255), police)]
    y = H // 2 - 180
    for texte, couleur, f in lignes:
        ecran.blit(f.render(texte, True, couleur), (W // 2 - 270, y))
        y += 48 if f is grande else 34
    derniers = "   ".join(f"T{etat['tours'] - len(etat['liste']) + k + 1}: {t:.1f}s" for k, t in enumerate(etat["liste"][-4:]))
    ecran.blit(petite.render(derniers or "Termine un tour pour voir tes temps !", True, (180, 200, 255)), (W // 2 - 270, y + 6))
    ecran.blit(petite.render("Disques et pneus refroidis !", True, (120, 255, 160)), (W // 2 - 270, y + 34))

# ===== 9. BOUCLE DE JEU =====
en_marche = True
img = 0
while en_marche:
    dt = min(horloge.tick(60) / 1000, 0.05)
    img += 1
    for ev in pygame.event.get():
        if ev.type == pygame.QUIT:
            en_marche = False
        if ev.type == pygame.KEYDOWN:
            if ev.key == pygame.K_ESCAPE:
                en_marche = False
            if ev.key == pygame.K_r:
                etat = reset()
                particules.clear()
                marques.clear()

    t = pygame.key.get_pressed()
    gaz = t[pygame.K_UP] or t[pygame.K_z] or t[pygame.K_w]
    frein = t[pygame.K_DOWN] or t[pygame.K_s]
    droite = (1 if (t[pygame.K_RIGHT] or t[pygame.K_d]) else 0) - (1 if (t[pygame.K_LEFT] or t[pygame.K_q] or t[pygame.K_a]) else 0)

    numero, distance, dec = info_piste(etat["x"], etat["y"])
    en_stand = numero in PITSET and DEMI_LARGEUR + KERB < dec < PIT_FIN
    herbe = distance > DEMI_LARGEUR + KERB and not en_stand

    # --- Pédales : elles s'enfoncent progressivement ---
    gaz_avant = etat["gaz"]
    etat["gaz"] = min(1, etat["gaz"] + PEDALE_GAZ * dt) if gaz else max(0, etat["gaz"] - 5 * dt)
    etat["frein"] = min(1, etat["frein"] + PEDALE_FREIN * dt) if frein else max(0, etat["frein"] - 3 * dt)

    # --- Appui aéro : plus on va vite, plus on colle au sol ---
    v = etat["v"]
    appui = 1 + APPUI * (abs(v) / VITESSE_MAX) ** 2
    grip_av, grip_ar = GRIP_AV * appui, GRIP_AR * appui

    # --- Direction ---
    braquage_max = 0.55 / (1 + abs(v) / 150)
    etat["volant"] += (droite * braquage_max - etat["volant"]) * min(1, 8 * dt)
    rotation = v * math.tan(etat["volant"]) / EMPATTEMENT
    a_lat = abs(v * rotation)

    a_frein = FREINAGE * etat["frein"] ** 1.5 if v > 0.5 else 0
    usage_av = math.hypot(a_frein, a_lat)
    avant_glisse = usage_av > grip_av
    if avant_glisse:
        rotation *= grip_av / usage_av                 # sous-virage

    a_moteur = PUISSANCE * etat["gaz"] * (1 - max(v, 0) / VITESSE_MAX)
    usage_ar = math.hypot(a_moteur, a_lat)
    arriere_patine = usage_ar > grip_ar
    if arriere_patine:
        exces = usage_ar / grip_ar - 1
        rotation *= 1 + 2 * exces                      # survirage
        a_moteur *= grip_ar / usage_ar

    # --- Vitesse ---
    v += a_moteur * dt
    if v > 0.5:
        v -= a_frein * dt
    elif frein:
        v -= 250 * dt                                  # marche arrière
    v -= v * (0.3 if gaz else 0.6) * dt
    if herbe:
        v -= v * 2.5 * dt
        v = min(v, 220)
    etat["v"] = v = max(-150, min(VITESSE_MAX, v))
    etat["vmax"] = max(etat["vmax"], abs(v))

    etat["angle"] += rotation * dt
    etat["x"] = max(0, min(MONDE[0], etat["x"] + math.cos(etat["angle"]) * v * dt))
    etat["y"] = max(0, min(MONDE[1], etat["y"] + math.sin(etat["angle"]) * v * dt))

    # --- Disques de frein : ils chauffent quand on freine fort ---
    if etat["frein"] > 0.05 and v > 0:
        etat["chaleur"] += etat["frein"] * (abs(v) / VITESSE_MAX) * 1.6 * dt
    else:
        etat["chaleur"] -= 0.4 * dt
    if en_stand:
        etat["chaleur"] = 0
    etat["chaleur"] = max(0, min(1, etat["chaleur"]))

    # --- Mouvements de la carrosserie et de l'aileron ---
    lisse = min(1, 6 * dt)
    etat["tangage"] += ((etat["frein"] * 0.9 - etat["gaz"] * 0.5 * (1 - abs(v) / VITESSE_MAX)) * 0.12 - etat["tangage"]) * lisse
    etat["roulis"] += (0.00025 * v * rotation - etat["roulis"]) * lisse
    cible_aile = 0.1 + 0.25 * (abs(v) / VITESSE_MAX) + 0.9 * etat["frein"]
    etat["aile"] += (cible_aile - etat["aile"]) * min(1, 10 * dt)
    etat["roue"] += v * dt / 9 * 0.35

    # --- Effets ---
    if etat["frein"] > 0.3 and etat["chaleur"] > 0.3 and v > 120:
        for cote in (-1, 1):                           # ÉTINCELLES aux disques avant
            for _ in range(2):
                emettre("etincelle", 24, cote * 20, 8, -random.uniform(40, 160), cote * random.uniform(10, 90),
                        random.uniform(40, 140), random.uniform(0.25, 0.6), 1)
    if (not gaz and gaz_avant > 0.35 and v > 250) or (etat["frein"] > 0.5 and v > 350 and random.random() < 0.12):
        for cote in (-1, 1):                           # FLAMMES de l'échappement
            for _ in range(3):
                emettre("flamme", -40, cote * 9, 7, -random.uniform(60, 180), random.uniform(-20, 20),
                        random.uniform(-10, 30), random.uniform(0.12, 0.28), random.uniform(5, 9))
    gros_freinage = etat["frein"] > 0.9 and v > 350
    if (arriere_patine or avant_glisse or gros_freinage) and not herbe and img % 2 == 0:
        roues = ([-22] if arriere_patine else []) + ([22] if (avant_glisse or gros_freinage) else [])
        for lx in roues:                               # FUMÉE des pneus
            for cote in (-1, 1):
                emettre("fumee", lx, cote * 21, 3, -random.uniform(0, 30), random.uniform(-20, 20),
                        random.uniform(15, 40), random.uniform(0.6, 1.2), random.uniform(7, 11))
    glisse = (arriere_patine or avant_glisse or gros_freinage) and not herbe
    for k, (lx, ly) in enumerate([(-22, -21), (-22, 21), (22, -21), (22, 21)]):
        wx, wy, _ = loc2mon(lx, ly, 0)
        if glisse:                                     # TRACES de pneus sur le sol
            if derniere[k]:
                marques.append((derniere[k][0], derniere[k][1], wx, wy))
            derniere[k] = (wx, wy)
        else:
            derniere[k] = None
    mettre_a_jour_particules(dt)

    # --- Tours ---
    etat["chrono"] += dt
    if numero * 4 // N == etat["suivant"]:
        if etat["suivant"] == 0:
            etat["tours"] += 1
            etat["dernier"] = etat["chrono"]
            etat["liste"] = (etat["liste"] + [etat["chrono"]])[-5:]
            if etat["meilleur"] is None or etat["chrono"] < etat["meilleur"]:
                etat["meilleur"] = etat["chrono"]
                sauver_record(etat["meilleur"])
            etat["chrono"] = 0.0
        etat["suivant"] = (etat["suivant"] + 1) % 4

    # --- Caméra derrière la voiture (avec un peu de retard : on voit la voiture tourner) ---
    ecart = (etat["angle"] - etat["cam"] + math.pi) % (2 * math.pi) - math.pi
    etat["cam"] += ecart * min(1, 3.5 * dt)
    cs, sn = math.cos(etat["cam"]), math.sin(etat["cam"])
    camx, camy = etat["x"] - cs * CAMDIST, etat["y"] - sn * CAMDIST

    # --- Dessin : ciel, collines, herbe ---
    ecran.fill(CIEL)
    pygame.draw.rect(ecran, CIEL2, (0, HORIZON - 70, W, 70))
    for x in range(0, W, 6):
        gisement = etat["cam"] + math.atan((x - W / 2) / FOCAL)
        h = int(35 + 25 * math.sin(gisement * 4) + 12 * math.sin(gisement * 9 + 1))
        pygame.draw.rect(ecran, (80, 125, 115), (x, HORIZON - h, 6, h))
    pygame.draw.rect(ecran, HERBE, (0, HORIZON, W, H - HORIZON))

    T = 300
    gi, gj = int(etat["x"] // T), int(etat["y"] // T)
    carreaux = []
    for i in range(gi - 6, gi + 7):
        for j in range(gj - 6, gj + 7):
            if (i + j) % 2 == 0:
                pts = [sol(i*T, j*T), sol(i*T+T, j*T), sol(i*T+T, j*T+T), sol(i*T, j*T+T)]
                carreaux.append((sum(p[0] for p in pts), pts))
    carreaux.sort(key=lambda c: c[0], reverse=True)
    for _, pts in carreaux:
        poly(HERBE2, pts)

    # --- La piste et la voie des stands, du plus loin au plus proche ---
    visibles = {}
    for i in range(N):
        f = (centre[i][0] - camx) * cs + (centre[i][1] - camy) * sn
        if -250 < f < FAR:
            visibles[i] = f
    cache, cache_pit = {}, {}
    for i in sorted(visibles, key=visibles.get, reverse=True):
        j = (i + 1) % N
        if j not in visibles:
            continue
        if i not in cache:
            cache[i] = [sol(*ligne[i]) for ligne in LIGNES]
        if j not in cache:
            cache[j] = [sol(*ligne[j]) for ligne in LIGNES]
        a, b = cache[i], cache[j]
        vibreur = ROUGE if (i // 3) % 2 == 0 else BLANC
        poly(vibreur, [a[0], a[1], b[1], b[0]])
        poly(vibreur, [a[2], a[3], b[3], b[2]])
        poly(ASPHALTE if (i // 6) % 2 == 0 else ASPHALTE2, [a[1], a[2], b[2], b[1]])
        if (i // 3) % 2 == 0:
            poly(BLANC, [a[4], a[5], b[5], b[4]])
        if i in PITSET and j in PITSET:
            for q in (i, j):
                if q not in cache_pit:
                    cache_pit[q] = [sol(*ligne[q]) for ligne in LIGNES_PIT]
            pa, pb = cache_pit[i], cache_pit[j]
            poly((100, 100, 108), [pa[0], pa[2], pb[2], pb[0]])
            poly((255, 210, 0) if (i // 4) % 2 == 0 else NOIR, [pa[1], pa[2], pb[2], pb[1]])

    # --- Traces de pneus ---
    for (x1, y1, x2, y2) in marques:
        f1, r1 = cam(x1, y1)
        f2, r2 = cam(x2, y2)
        if NEAR < f1 < FAR and f2 > NEAR:
            pygame.draw.line(ecran, (28, 28, 30), (W / 2 + r1 / f1 * FOCAL, HORIZON + CAMH / f1 * FOCAL),
                             (W / 2 + r2 / f2 * FOCAL, HORIZON + CAMH / f2 * FOCAL), max(1, int(5 * FOCAL / f1)))

    # --- Décors, effets et voiture, du plus loin au plus proche ---
    objets = [(cam(etat["x"], etat["y"])[0], "voiture", None)]
    for (ax, ay, genre, taille) in ARBRES:
        f, r = cam(ax, ay)
        if NEAR < f < FAR:
            objets.append((f, "arbre", (r, genre, taille)))
    for g in GARAGES:
        f = cam(*centre[g[5]])[0] + 200
        if NEAR < f < FAR + 200:
            objets.append((f, "garage", g))
    for p in particules:
        f = cam(p[0], p[1])[0]
        if f > NEAR:
            objets.append((f, "particule", p))
    objets.append((cam(*centre[0])[0], "portique", None))
    objets.sort(key=lambda o: o[0], reverse=True)
    for f, genre, donnees in objets:
        if genre == "arbre":
            dessiner_arbre(f, *donnees)
        elif genre == "garage":
            dessiner_garage(donnees)
        elif genre == "particule":
            dessiner_particule(f, donnees)
        elif genre == "portique":
            dessiner_portique()
        else:
            dessiner_voiture()

    # --- HUD ---
    blanc = (255, 255, 255)
    ecran.blit(police.render(f"{int(abs(v) * 0.3)} km/h", True, blanc), (20, 20))
    ecran.blit(police.render(f"Tour {etat['tours'] + 1}  {etat['chrono']:.1f}s", True, blanc), (20, 48))
    ecran.blit(police.render(f"Meilleur : {fmt(etat['meilleur'])}", True, (255, 220, 0)), (20, 76))
    for k, (nom, valeur, couleur) in enumerate([("GAZ", etat["gaz"], (60, 220, 60)), ("FREIN", etat["frein"], (240, 60, 60)),
                                                ("DISQ", etat["chaleur"], (255, 150, 30))]):
        bx = 20 + k * 58
        pygame.draw.rect(ecran, (0, 0, 0), (bx, H - 130, 40, 100))
        pygame.draw.rect(ecran, couleur, (bx, H - 30 - 100 * valeur, 40, 100 * valeur))
        pygame.draw.rect(ecran, blanc, (bx, H - 130, 40, 100), 2)
        ecran.blit(petite.render(nom, True, blanc), (bx - 2, H - 28))
    ecran.blit(petite.render(f"APPUI AERO : +{int((appui - 1) * 100)}%", True, (160, 220, 255)), (W // 2 - 80, H - 28))
    if herbe:
        ecran.blit(police.render("HORS PISTE !", True, (255, 80, 80)), (W // 2 - 80, 20))
    if avant_glisse:
        ecran.blit(police.render("AVANT QUI GLISSE : trop de frein en virage !", True, (255, 160, 40)), (W // 2 - 230, 50))
    if arriere_patine:
        ecran.blit(police.render("ARRIERE QUI PATINE : trop de gaz !", True, (255, 160, 40)), (W // 2 - 190, 80))
    ecran.blit(mini, (W - mini.get_width() - 10, 10))
    pygame.draw.circle(ecran, (255, 60, 60), (W - mini.get_width() - 10 + etat["x"] * ECH + 5, 10 + etat["y"] * ECH + 5), 5)
    if en_stand:
        afficher_stands()

    pygame.display.flip()

pygame.quit()
