# 🏎️ MINI KARTING V6 - nouveau circuit, graphismes améliorés, rails, tribunes, brume, ciel, compteur
# Lancer :  python karting_v6.py
# Commandes : ⬆️ ou Z/W = gaz | ⬇️ ou S = frein / marche arrière | ⬅️ ➡️ ou Q/A D = tourner
#             N = jour / crépuscule / nuit | P = pluie ON/OFF | R = recommencer | M = retour au menu | G = qualité graphique | Échap = quitter
#
# 🎓 À APPRENDRE :
#  - GAZ progressif mais puissants : trop de gaz en virage = l'ARRIÈRE patine (fumée + traces).
#  - FREIN sur l'AVANT, progressif : trop de frein en virage = l'AVANT glisse (sous-virage).
#    Freinage fort = disques chauds = ÉTINCELLES.
#  - APPUI AÉRO : plus tu vas vite, plus l'aileron plaque la voiture au sol.
#  - HERBE : 1 roue dehors = presque rien, 4 roues dehors = gros ralentissement ET +1 seconde !
#  - PLUIE : la piste est glissante (moins de grip). NUIT : tes phares éclairent la route.
#  - RELIEF : ça monte (ça ralentit) et ça descend (ça accélère).
#  - STANDS : roule dans la voie des stands (à droite de la ligne de départ) pour voir tes scores.

import pygame
import math
import random
from collections import deque

# ===== 1. RÉGLAGES (change-les pour t'amuser !) =====
W, H = 1100, 720
QUALITE = 1                # 0 = éco (PC lent) | 1 = élevé | 2 = ultra  (touche G en jeu)
VITESSE_MAX = 720          # (ces réglages sont remplacés par ceux de la voiture choisie)
PUISSANCE = 800
PEDALE_GAZ = 1.0
PEDALE_FREIN = 1.6
FREINAGE = 700
GRIP_AV = 700
GRIP_AR = 750
APPUI = 0.5
EMPATTEMENT = 44
DEMI_LARGEUR = 150         # demi-largeur de la piste : 300 px de large, comme un vrai circuit !
KERB = 22                  # largeur des vibreurs
MUR_DIST = 70              # bande d'herbe entre les vibreurs et les rails
ECHELLE_CIRCUIT = 1.0      # 1.0 = ~21 000 px de long. Monte à 1.3 pour un circuit encore plus long
RELIEF = 70                # hauteur des collines (0 = piste plate)
PENTE_FORCE = 1200
HERBE_FREIN = 0.18
COULEUR_VOITURE = (220, 30, 30)

# Le circuit : départ sur la grande ligne droite du haut, puis virage 1, enchaînement rapide,
# virage serré, longue ligne droite, chicane, virage en épingle, esses, et dernier virage.
BRUT = [(3200, 1700), (5000, 1700), (6700, 1720), (7500, 1950), (8000, 2550), (8000, 3250), (7650, 3900),
        (8100, 4550), (7950, 5300), (7700, 6000), (7100, 6550), (6200, 6750), (5200, 6750), (4500, 6700),
        (3950, 6480), (3400, 6930), (2850, 6700), (2200, 6600), (1750, 6150), (1700, 5500), (1950, 4850),
        (1600, 4200), (1950, 3550), (1650, 2900), (1850, 2300), (2350, 1850)]
POINTS = [(x * ECHELLE_CIRCUIT, y * ECHELLE_CIRCUIT) for x, y in BRUT]
MONDE = (int(max(p[0] for p in POINTS)) + 1800, int(max(p[1] for p in POINTS)) + 1800)

HORIZON, FOCAL_BASE, CAMH, CAMDIST = int(H * 0.40), int(W * 0.58), 65, 150
FOCAL = FOCAL_BASE
NEAR = 50
FAR_Q = [1500, 2100, 2600]
FAR = FAR_Q[QUALITE]
FICHIER_RECORD = "record_karting.txt"

HERBE, HERBE2, HERBE3 = (44, 156, 46), (36, 136, 40), (88, 160, 52)
ASPHALTE, ASPHALTE2 = (80, 80, 85), (72, 72, 77)
ROUGE, BLANC, NOIR = (220, 30, 30), (240, 240, 240), (25, 25, 25)

def hauteur(x, y):
    """Le relief du terrain (grandes collines douces)"""
    return RELIEF * (0.6 * math.sin(x / 1100) + 0.4 * math.sin(y / 900 + 1) + 0.3 * math.sin((x + y) / 1400 + 2))

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
DIST = [0.0]
for i in range(1, N):
    DIST.append(DIST[-1] + math.hypot(centre[i][0] - centre[i-1][0], centre[i][1] - centre[i-1][1]))
LONGUEUR = DIST[-1] + math.hypot(centre[0][0] - centre[-1][0], centre[0][1] - centre[-1][1])
ESP = LONGUEUR / N         # distance entre deux points de la piste (~18 px)

def plus_proche(x, y):
    i = min(range(N), key=lambda k: (centre[k][0]-x)**2 + (centre[k][1]-y)**2)
    return i, math.hypot(centre[i][0]-x, centre[i][1]-y)

MUR = DEMI_LARGEUR + KERB + MUR_DIST
DECALAGES = [DEMI_LARGEUR + KERB, DEMI_LARGEUR, -DEMI_LARGEUR, -(DEMI_LARGEUR + KERB), 38, -38, MUR, -MUR]
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

def pos(i, d):
    i %= N
    return (centre[i][0] + NORMALES[i][0] * d, centre[i][1] + NORMALES[i][1] * d)

# --- Les STANDS : voie des stands + garages, à droite de la ligne de départ ---
PIT_D1 = DEMI_LARGEUR + KERB + 4
PIT_D2 = PIT_D1 + 110
PIT_FIN = PIT_D2 + 6
PITLEN = int(1900 / ESP)
PIT0 = N - int(500 / ESP)
PITS = [(PIT0 + m) % N for m in range(PITLEN)]
PITSET = set(PITS)
LIGNES_PIT = [[pos(i, d) for i in range(N)] for d in (PIT_D1, PIT_D2 - 3, PIT_D2)]
COULEURS_PORTES = [(220, 40, 40), (40, 90, 220), (240, 200, 30), (40, 170, 90), (230, 120, 20), (150, 60, 200)]
GARAGES = []
pas_g, long_g = int(340 / ESP), int(250 / ESP)
for g, m in enumerate(range(int(150 / ESP), PITLEN - long_g - 2, pas_g)):
    i0, i1 = PITS[m], PITS[m + long_g]
    GARAGES.append((pos(i0, PIT_D2 + 30), pos(i1, PIT_D2 + 30), pos(i0, PIT_D2 + 150), pos(i1, PIT_D2 + 150),
                    COULEURS_PORTES[g % 6], PITS[m + long_g // 2]))

def info_piste(x, y):
    n, d = plus_proche(x, y)
    dec = (x - centre[n][0]) * NORMALES[n][0] + (y - centre[n][1]) * NORMALES[n][1]
    return n, d, dec

def roue_dehors(x, y, n0):
    """Cette roue est-elle hors de la piste ? (on cherche seulement près de la voiture)"""
    i = min(((n0 + d) % N for d in range(-30, 31)), key=lambda k: (centre[k][0]-x)**2 + (centre[k][1]-y)**2)
    if math.hypot(centre[i][0]-x, centre[i][1]-y) <= DEMI_LARGEUR + KERB:
        return False
    dec = (x - centre[i][0]) * NORMALES[i][0] + (y - centre[i][1]) * NORMALES[i][1]
    return not (i in PITSET and DEMI_LARGEUR + KERB < dec < PIT_FIN)

# --- Les décors fixes (calculés une seule fois) ---
random.seed(7)
def hors_stands(n, dec):
    return not (n in PITSET and 0 < dec < 500)

ARBRES = []
for i in range(0, N, 4):
    for cote in (1, -1):
        d = MUR + 40 + random.random() * 420
        x, y = centre[i][0] + NORMALES[i][0] * d * cote, centre[i][1] + NORMALES[i][1] * d * cote
        if 0 < x < MONDE[0] and 0 < y < MONDE[1]:
            n, dist, dec = info_piste(x, y)
            if dist > MUR + 30 and hors_stands(n, dec):
                ARBRES.append((x, y, hauteur(x, y), random.randint(0, 1), 0.8 + random.random() * 0.7))
for _ in range(900):                                  # la forêt dans le circuit et autour
    x, y = random.random() * MONDE[0], random.random() * MONDE[1]
    n, dist, dec = info_piste(x, y)
    if dist > MUR + 120 and hors_stands(n, dec):
        ARBRES.append((x, y, hauteur(x, y), random.randint(0, 1), 0.8 + random.random() * 0.8))

# Tribunes en face des stands (côté gauche de la ligne droite de départ)
FOULE = [(220, 60, 60), (60, 90, 220), (240, 200, 50), (240, 240, 240), (60, 170, 90), (230, 130, 40), (170, 70, 200)]
TRIBUNES = []
pas_t = max(2, int(170 / ESP))
for k, i0 in enumerate(range(int(40 / ESP), int(2000 / ESP), pas_t)):
    TRIBUNES.append((i0, i0 + pas_t - 1, [[random.choice(FOULE) for _ in range(3)] for _ in range(4)]))

# Panneaux publicitaires le long des rails
PANNEAUX = []
PUBS = [((220, 40, 40), (255, 255, 255)), ((30, 80, 200), (255, 220, 0)), ((20, 20, 24), (255, 60, 60)),
        ((250, 250, 250), (30, 80, 200)), ((255, 200, 0), (20, 20, 24)), ((30, 150, 80), (255, 255, 255))]
long_p = max(3, int(260 / ESP))
for k, i0 in enumerate(range(int(1500 / ESP), N - long_p, int(700 / ESP))):
    cote = 1 if k % 2 == 0 else -1
    if cote == 1 and (i0 in PITSET or (i0 + long_p) in PITSET):
        cote = -1
    PANNEAUX.append((i0, i0 + long_p, cote, PUBS[k % len(PUBS)]))

# Lampadaires (s'allument la nuit) le long de la ligne droite de départ
LAMPES = []
for i0 in range(N - int(700 / ESP), N + int(3400 / ESP), int(380 / ESP)):
    for cote in (1, -1):
        if cote == 1 and (i0 % N) in PITSET:
            continue
        LAMPES.append(pos(i0, cote * (MUR + 24)))
ETOILES = [(random.uniform(0, 2 * math.pi), random.uniform(0.05, 0.95)) for _ in range(120)]
NUAGES = [(random.uniform(0, 2 * math.pi), random.uniform(0.0, 0.85), random.uniform(70, 150), random.random()) for _ in range(18)]
SOLEIL_B, LUNE_B = 0.9, 3.9

# ===== 3. MISE EN PLACE =====
pygame.init()
ecran = pygame.display.set_mode((W, H))
pygame.display.set_caption("Mini Karting V6")
horloge = pygame.time.Clock()
police = pygame.font.SysFont("monospace", 22, bold=True)
petite = pygame.font.SysFont("monospace", 16, bold=True)
grande = pygame.font.SysFont("monospace", 34, bold=True)
enorme = pygame.font.SysFont("monospace", 46, bold=True)

# Minimap (la carte du circuit en haut à droite)
ECH = 250 / MONDE[0]
mini = pygame.Surface((int(MONDE[0] * ECH) + 20, int(MONDE[1] * ECH) + 20), pygame.SRCALPHA)
pygame.draw.rect(mini, (8, 10, 18, 170), (0, 0, mini.get_width(), mini.get_height()), border_radius=10)
mini_pts = [(x * ECH + 10, y * ECH + 10) for x, y in centre]
pygame.draw.lines(mini, (40, 44, 56, 255), True, mini_pts, 8)
pygame.draw.lines(mini, (225, 228, 235, 255), True, mini_pts, 4)
pygame.draw.circle(mini, (255, 210, 0, 255), (int(mini_pts[0][0]), int(mini_pts[0][1])), 5)

CERCLE = pygame.Surface((64, 64), pygame.SRCALPHA)
pygame.draw.circle(CERCLE, (225, 225, 225), (32, 32), 32)
OMBRE = pygame.Surface((W, H), pygame.SRCALPHA)     # voile sombre de la nuit
LUMIERE = pygame.Surface((W, H))                    # phares, feux, lampadaires (ajoutés par-dessus)

def fabriquer_lueur(couleur):
    s = pygame.Surface((256, 256))
    s.fill((0, 0, 0))
    for r in range(128, 0, -3):
        k = (1 - r / 128) ** 2.2
        pygame.draw.circle(s, tuple(int(c * k) for c in couleur), (128, 128), r)
    return s
LUEUR_SOLEIL = fabriquer_lueur((255, 215, 140))
LUEUR_LUNE = fabriquer_lueur((150, 170, 230))

VIGNETTE = pygame.Surface((W, H), pygame.SRCALPHA)  # coins sombres (effet caméra)
VIGNETTE.fill((0, 0, 0, 120))
_rmax = math.hypot(W, H) / 2
for k in range(34):
    pygame.draw.circle(VIGNETTE, (0, 0, 0, int(120 * (1 - (k + 1) / 34) ** 1.7)), (W // 2, H // 2), int(_rmax * (1 - k / 34)))

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
    z0 = hauteur(*centre[0])
    return dict(x=centre[0][0], y=centre[0][1], z=z0, cz=z0, angle=angle, cam=angle, v=0.0, volant=0.0,
                gaz=0.0, frein=0.0, chaleur=0.0, tangage=0.0, roulis=0.0, aile=0.1, roue=0.0,
                suivant=1, tours=0, chrono=0.0, meilleur=lire_record(), dernier=None, liste=[], vmax=0.0,
                dehors4=False, msg=0.0, secousse=0.0)

etat = reset()
meteo = dict(phase=0, nuit=0.0, pluie_on=False, pluie=0.0)
gouttes = [[random.random() * W, random.random() * H, random.uniform(700, 1100)] for _ in range(170)]
lentille = []                                       # gouttes d'eau sur la caméra
particules = []
marques = deque(maxlen=1600)
derniere = [None] * 4
ROUES = [(-22, -21), (-22, 21), (22, -21), (22, 21)]

# ===== 4. CAMÉRA 3D (avec brume : plus c'est loin, plus ça se fond dans l'horizon) =====
camx = camy = cs = sn = 0.0
CAMZ = CAMH
BRUME = (170, 215, 255)
BRUME_DIST = FAR

def cam(x, y):
    dx, dy = x - camx, y - camy
    return dx * cs + dy * sn, -dx * sn + dy * cs

def c3(x, y, z):
    f, r = cam(x, y)
    return (f, r, z)

def sol(x, y):
    return c3(x, y, hauteur(x, y))

def clamp(v):
    return max(-20000, min(20000, v))

def projeter(pts):
    if min(p[0] for p in pts) >= NEAR:          # cas rapide : tout est devant la caméra
        return [(W / 2 + r / f * FOCAL, HORIZON + (CAMZ - z) / f * FOCAL) for f, r, z in pts]
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
    return [(clamp(W / 2 + r / f * FOCAL), clamp(HORIZON + (CAMZ - z) / f * FOCAL)) for f, r, z in res]

def poly(couleur, pts, cible=None):
    p = projeter(pts)
    if p:
        pygame.draw.polygon(ecran if cible is None else cible, couleur, p)

def melange(c1, c2, t):
    t = max(0, min(1, t))
    return tuple(int(c1[k] + (c2[k] - c1[k]) * t) for k in range(3))

def teinte(c, k):
    return (int(c[0] * k), int(c[1] * k), int(c[2] * k))

def fondu(couleur, f):
    """Applique la brume selon la distance f"""
    t = f / BRUME_DIST
    if t <= 0.1:
        return couleur
    return melange(couleur, BRUME, min(1.0, t) ** 1.6 * 0.92)

def polym(couleur, pts):
    """Polygone du monde (avec brume)"""
    poly(fondu(couleur, sum(p[0] for p in pts) / len(pts)), pts)

# ===== 5. LES 4 VOITURES : modèles 3D + physique propre à chacune =====
PALETTE = [(220, 30, 30), (40, 90, 220), (240, 200, 30), (240, 240, 245), (40, 170, 90), (235, 120, 20)]

# Les réglages de chaque voiture (c'est ici que tu peux les bidouiller !)
VOITURES = [
    dict(nom="KART", defaut=0, vmax=520, puissance=900, pedale_gaz=1.4, pedale_frein=2.0, freinage=800,
         grip_av=950, grip_ar=950, appui=0.0, emp=30, braquage=0.7, vsens=200, roulis_k=0.00005, roulis_max=0.03,
         tangage_k=0.4, herbe_frein=0.2, herbe_grip=0.5, camh=48, camdist=125,
         stats=(0.45, 0.9, 0.75, 0.85, 0.0, 0.4),
         notes=["Ultra léger, tourne sec", "Freine très fort", "Pas d'aileron, pas d'appui", "Se pilote à l'instinct"]),
    dict(nom="4x4", defaut=5, vmax=560, puissance=600, pedale_gaz=0.9, pedale_frein=1.3, freinage=560,
         grip_av=600, grip_ar=650, appui=0.05, emp=52, braquage=0.5, vsens=150, roulis_k=0.0003, roulis_max=0.12,
         tangage_k=1.8, herbe_frein=0.04, herbe_grip=0.1, camh=85, camdist=175,
         stats=(0.55, 0.55, 0.5, 0.5, 0.05, 1.0),
         notes=["Haut : penche beaucoup", "Très doux sur l'herbe", "Peu de grip sur l'asphalte", "Freine moins fort"]),
    dict(nom="GT3", defaut=3, vmax=700, puissance=800, pedale_gaz=1.0, pedale_frein=1.6, freinage=760,
         grip_av=740, grip_ar=780, appui=0.55, emp=46, braquage=0.55, vsens=150, roulis_k=0.00012, roulis_max=0.07,
         tangage_k=1.0, herbe_frein=0.18, herbe_grip=0.45, camh=65, camdist=150,
         stats=(0.8, 0.7, 0.7, 0.7, 0.5, 0.4),
         notes=["L'équilibrée : un peu de tout", "Bon appui aéro", "Freinage puissant", "Facile à prendre en main"]),
    dict(nom="WEC", defaut=1, vmax=820, puissance=950, pedale_gaz=0.9, pedale_frein=1.5, freinage=840,
         grip_av=760, grip_ar=800, appui=1.3, emp=50, braquage=0.5, vsens=140, roulis_k=0.00008, roulis_max=0.05,
         tangage_k=0.6, herbe_frein=0.3, herbe_grip=0.6, camh=58, camdist=160,
         stats=(1.0, 0.8, 0.9, 0.75, 1.0, 0.2),
         notes=["Pointe de vitesse énorme", "Appui aéro colossal", "Peu de grip à basse vitesse", "Déteste l'herbe"]),
]
BRAQUAGE, VSENS, ROULIS_K, ROULIS_MAX, TANGAGE_K, HERBE_GRIP = 0.55, 150, 0.00012, 0.07, 1.0, 0.45
MODELE = None

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

def appliquer_voiture(idx, ci):
    # applique la physique de la voiture choisie et construit son modèle 3D
    global VITESSE_MAX, PUISSANCE, PEDALE_GAZ, PEDALE_FREIN, FREINAGE, GRIP_AV, GRIP_AR, APPUI, EMPATTEMENT
    global BRAQUAGE, VSENS, ROULIS_K, ROULIS_MAX, TANGAGE_K, HERBE_FREIN, HERBE_GRIP, CAMH, CAMDIST, MODELE, ROUES
    s = VOITURES[idx]
    VITESSE_MAX, PUISSANCE, FREINAGE = s["vmax"], s["puissance"], s["freinage"]
    PEDALE_GAZ, PEDALE_FREIN = s["pedale_gaz"], s["pedale_frein"]
    GRIP_AV, GRIP_AR, APPUI, EMPATTEMENT = s["grip_av"], s["grip_ar"], s["appui"], s["emp"]
    BRAQUAGE, VSENS = s["braquage"], s["vsens"]
    ROULIS_K, ROULIS_MAX, TANGAGE_K = s["roulis_k"], s["roulis_max"], s["tangage_k"]
    HERBE_FREIN, HERBE_GRIP, CAMH, CAMDIST = s["herbe_frein"], s["herbe_grip"], s["camh"], s["camdist"]
    MODELE = construire(idx, PALETTE[ci])
    ROUES = [(w[0], w[1]) for w in MODELE["roues"]]

def loc2mon(lx, ly, lz):
    ca, sa = math.cos(etat["angle"]), math.sin(etat["angle"])
    return etat["x"] + ca * lx - sa * ly, etat["y"] + sa * lx + ca * ly, etat["z"] + lz

def dessiner_voiture(nuit):
    M = MODELE
    pz = M["pz"]
    ca, sa = math.cos(etat["angle"]), math.sin(etat["angle"])
    cp, sp = math.cos(etat["tangage"]), math.sin(etat["tangage"])
    cr, sr = math.cos(etat["roulis"]), math.sin(etat["roulis"])
    X, Y, Z = etat["x"], etat["y"], etat["z"]

    def tr(p, corps=True):
        x, y, z = p
        if corps:    # la carrosserie PIVOTE (tangage + roulis) sans se déformer
            dz = z - pz
            x, dz2 = x * cp + dz * sp, dz * cp - x * sp
            y, dz3 = y * cr - dz2 * sr, dz2 * cr + y * sr
            z = pz + dz3
        wx, wy = X + ca * x - sa * y, Y + sa * x + ca * y
        dx, dy = wx - camx, wy - camy
        return (dx * cs + dy * sn, -dx * sn + dy * cs, Z + z)

    faces = []
    def ajouter(pts, couleur, corps=True, biais=0):
        m = [tr(p, corps) for p in pts]
        faces.append((sum(q[0] for q in m) / len(m) - biais, couleur, m))

    # roues : elles tournent avec la vitesse, les avant braquent avec le volant
    n = 12
    for cx, cy, avant, rayon, demi in M["roues"]:
        braque = etat["volant"] if avant else 0
        chaud = etat["chaleur"] if avant else 0
        cb, sb = math.cos(braque), math.sin(braque)
        ext = demi if cy > 0 else -demi
        def pt(theta, y, r=1.0):
            dx, dz = rayon * r * math.cos(theta), rayon * r * math.sin(theta)
            return (cx + dx * cb - y * sb, cy + dx * sb + y * cb, rayon + dz)
        for k in range(n):
            t0 = etat["roue"] + k * 2 * math.pi / n
            t1 = t0 + 2 * math.pi / n
            base = (36, 36, 40) if k % 2 == 0 else (60, 60, 66)
            ajouter([pt(t0, -demi), pt(t1, -demi), pt(t1, demi), pt(t0, demi)],
                    teinte(base, 0.55 + 0.45 * max(0, math.sin((t0 + t1) / 2))), False)
        jante = melange((150, 150, 158), (255, 90, 20), chaud)       # le disque rougit quand il chauffe
        ajouter([pt(etat["roue"] + k * 2 * math.pi / n, ext, M["rj"]) for k in range(n)], jante, False, biais=0.2)
        for m in range(3):
            a = etat["roue"] + m * 2 * math.pi / 3
            ajouter([pt(a, ext, M["rj"] * 0.95), pt(a + 0.3, ext, 0.15), pt(a - 0.3, ext, 0.15)],
                    melange((225, 225, 230), (255, 200, 60), chaud), False, biais=0.4)

    for pts, couleur in M["statique"]:
        ajouter(pts, couleur)
    if M["aile"]:        # le volet mobile de l'aileron : il se relève au freinage
        a = M["aile"]
        for pts, couleur in boite(a["x0"], a["x1"], a["y0"], a["y1"], a["z0"], a["z1"], a["couleur"],
                                  rot=(a["px"], a["pz"], etat["aile"])):
            ajouter(pts, couleur)
    feux = (255, 60, 60) if etat["frein"] > 0.1 else (130, 0, 0)
    phares = (255, 250, 200) if nuit > 0.2 else (225, 220, 170)
    for quad in M["feux"]:
        ajouter(quad, feux)
    for quad in M["phares"]:
        ajouter(quad, phares, biais=0.5)
    x0, x1, yw = M["ombre"]
    ajouter([(x0, -yw, 0.3), (x1, -yw, 0.3), (x1, yw, 0.3), (x0, yw, 0.3)], (22, 22, 25), False, biais=-1000)
    faces.sort(key=lambda f: f[0], reverse=True)
    for _, couleur, pts in faces:
        poly(couleur, pts)

def dessiner_menu(t):
    global camx, camy, cs, sn, CAMZ
    s = VOITURES[sel_car]
    etat["x"] = etat["y"] = etat["z"] = 0.0
    etat["angle"] = t * 0.7
    etat["roue"] += 0.05
    etat["aile"] = 0.1 + 0.45 * (0.5 + 0.5 * math.sin(t * 2))
    etat["frein"] = 1.0 if int(t * 1.2) % 2 else 0.0
    etat["tangage"] = 0.01 * math.sin(t * 2)
    etat["roulis"] = 0.015 * math.sin(t * 1.3)
    cs, sn, camx, camy, CAMZ = 1.0, 0.0, -MODELE["menu_dist"], 0.0, 45
    ecran.fill((18, 20, 30))
    pygame.draw.rect(ecran, (26, 30, 44), (0, 0, W, HORIZON))
    pygame.draw.rect(ecran, (40, 44, 54), (0, HORIZON, W, H - HORIZON))
    poly((72, 78, 92), [c3(95 * math.cos(k * 2 * math.pi / 28), 95 * math.sin(k * 2 * math.pi / 28), 0) for k in range(28)])
    dessiner_voiture(0.0)
    titre = grande.render("CHOISIS TA VOITURE", True, (255, 210, 0))
    ecran.blit(titre, (W // 2 - titre.get_width() // 2, 16))
    for i, v in enumerate(VOITURES):
        x = W // 2 - 400 + i * 200
        actif = i == sel_car
        pygame.draw.rect(ecran, (255, 210, 0) if actif else (50, 56, 72), (x, 70, 180, 40), border_radius=6)
        nom = police.render(v["nom"], True, (20, 20, 20) if actif else (230, 230, 230))
        ecran.blit(nom, (x + 90 - nom.get_width() // 2, 77))
    ecran.blit(grande.render("<", True, (255, 210, 0)), (W // 2 - 340, 330))
    ecran.blit(grande.render(">", True, (255, 210, 0)), (W // 2 + 320, 330))
    ecran.blit(police.render("STATS", True, (255, 255, 255)), (50, 150))
    for k, (nom, val) in enumerate(zip(["Vitesse", "Accélération", "Freinage", "Grip", "Appui aéro", "Herbe"], s["stats"])):
        y = 195 + k * 40
        ecran.blit(petite.render(nom, True, (200, 205, 220)), (50, y))
        pygame.draw.rect(ecran, (50, 56, 72), (200, y + 3, 170, 16), border_radius=4)
        if val > 0:
            pygame.draw.rect(ecran, melange((240, 90, 60), (90, 220, 120), val), (200, y + 3, int(170 * val), 16), border_radius=4)
    ecran.blit(police.render("PHYSIQUE", True, (255, 255, 255)), (W - 400, 150))
    for k, ligne in enumerate(s["notes"]):
        ecran.blit(petite.render("- " + ligne, True, (200, 205, 220)), (W - 400, 195 + k * 34))
    for k, col in enumerate(PALETTE):
        x = W // 2 - 3 * 52 + k * 52 + 6
        pygame.draw.rect(ecran, col, (x, H - 120, 40, 40), border_radius=6)
        if k == sel_col:
            pygame.draw.rect(ecran, (255, 255, 255), (x - 4, H - 124, 48, 48), 3, border_radius=8)
    aide = petite.render("GAUCHE/DROITE : voiture    HAUT/BAS : couleur    ENTREE : jouer    ECHAP : quitter", True, (200, 205, 220))
    ecran.blit(aide, (W // 2 - aide.get_width() // 2, H - 50))

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
            sol_z = hauteur(p[0], p[1])
            if p[2] < sol_z:
                p[2], p[5] = sol_z, -p[5] * 0.3
        elif p[8] == "fumee":
            p[9] += 18 * dt
    particules[:] = [p for p in particules if p[6] > 0]

def dessiner_particule(f, p):
    s = FOCAL / f
    sx, sy = W / 2 + cam(p[0], p[1])[1] * s, HORIZON + (CAMZ - p[2]) * s
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
def dessiner_arbre(f, r, z, genre, taille):
    s = FOCAL / f * taille
    sx, sy = W / 2 + r / f * FOCAL, HORIZON + (CAMZ - z) / f * FOCAL
    if sx < -250 or sx > W + 250:
        return
    pygame.draw.rect(ecran, fondu((92, 58, 30), f), (sx - 5 * s, sy - 35 * s, 10 * s, 35 * s))
    simple = QUALITE == 0 or f > 1100
    if genre == 0:       # sapin
        sombre, moyen, clair = fondu((16, 78, 36), f), fondu((26, 104, 50), f), fondu((46, 135, 70), f)
        if simple:
            pygame.draw.polygon(ecran, moyen, [(sx - 42*s, sy - 25*s), (sx + 42*s, sy - 25*s), (sx, sy - 150*s)])
        else:
            for (w, y0, y1) in [(48, -22, -86), (39, -60, -116), (28, -94, -154)]:
                pygame.draw.polygon(ecran, sombre, [(sx - w*s, sy + y0*s), (sx + w*s, sy + y0*s), (sx, sy + y1*s)])
                pygame.draw.polygon(ecran, moyen if QUALITE == 1 else clair, [(sx - w*s, sy + y0*s), (sx, sy + y0*s), (sx, sy + y1*s)])
    else:                # arbre rond
        sombre, moyen, clair = fondu((34, 108, 40), f), fondu((54, 142, 52), f), fondu((88, 178, 76), f)
        pygame.draw.circle(ecran, sombre, (sx, sy - 85 * s), 48 * s)
        if not simple:
            pygame.draw.circle(ecran, moyen, (sx - 6 * s, sy - 92 * s), 40 * s)
            pygame.draw.circle(ecran, clair, (sx - 16 * s, sy - 102 * s), 22 * s)

def milieu(a, b, t):
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)

def p3(q, dz=0.0):
    return c3(q[0], q[1], hauteur(*q) + dz)

def dessiner_garage(g):
    A0, A1, B0, B1, porte, _ = g
    h = 60
    polym((52, 54, 64), [p3(A0, h), p3(A1, h), p3(B1, h), p3(B0, h)])
    polym((214, 216, 222), [p3(A0, 0), p3(A1, 0), p3(A1, h), p3(A0, h)])
    polym((60, 62, 74), [p3(A0, h - 8), p3(A1, h - 8), p3(A1, h), p3(A0, h)])
    for k in range(3):                              # trois portes de garage par bâtiment
        d0, d1 = milieu(A0, A1, 0.06 + k * 0.31), milieu(A0, A1, 0.31 + k * 0.31)
        polym(porte, [p3(d0, 0), p3(d1, 0), p3(d1, 40), p3(d0, 40)])
        polym((30, 32, 40), [p3(d0, 40), p3(d1, 40), p3(d1, 44), p3(d0, 44)])

def dessiner_portique():
    pa, pb = LIGNES[0][0], LIGNES[3][0]
    (fa, ra), (fb, rb) = cam(*pa), cam(*pb)
    if min(fa, fb) < NEAR:
        return
    za, zb = hauteur(*pa), hauteur(*pb)
    xa, xb = W / 2 + ra / fa * FOCAL, W / 2 + rb / fb * FOCAL
    sa_, sb_ = HORIZON + (CAMZ - za) / fa * FOCAL, HORIZON + (CAMZ - zb) / fb * FOCAL
    ha, hb = HORIZON + (CAMZ - za - 200) / fa * FOCAL, HORIZON + (CAMZ - zb - 200) / fb * FOCAL
    ea, eb = 55 * FOCAL / fa, 55 * FOCAL / fb
    pygame.draw.line(ecran, (205, 205, 210), (xa, sa_), (xa, ha), max(2, int(11 * FOCAL / fa)))
    pygame.draw.line(ecran, (205, 205, 210), (xb, sb_), (xb, hb), max(2, int(11 * FOCAL / fb)))
    for k in range(18):
        t0, t1 = k / 18, (k + 1) / 18
        x0, x1 = xa + (xb - xa) * t0, xa + (xb - xa) * t1
        y0, y1 = ha + (hb - ha) * t0, ha + (hb - ha) * t1
        e0, e1 = ea + (eb - ea) * t0, ea + (eb - ea) * t1
        pygame.draw.polygon(ecran, BLANC if k % 2 == 0 else NOIR, [(x0, y0), (x1, y1), (x1, y1 + e1), (x0, y0 + e0)])

def dessiner_tribune(t):
    """Gradins avec la foule, vus de la piste"""
    i0, i1, rangs = t
    base, marche = MUR + 60, 36
    poly_toit = []
    for r in (3, 2, 1, 0):                          # du fond vers l'avant
        off = base + r * marche
        h1, h0 = 30 + r * 30, r * 30
        a0, a1 = pos(i0, -off), pos(i1, -off)
        for k in range(3):                          # la foule : 3 groupes par rangée
            b0, b1 = milieu(a0, a1, k / 3), milieu(a0, a1, (k + 1) / 3)
            polym(rangs[r][k], [p3(b0, h0), p3(b1, h0), p3(b1, h1), p3(b0, h1)])
        c0, c1 = pos(i0, -(off + marche)), pos(i1, -(off + marche))
        polym((150, 152, 160), [p3(a0, h1), p3(a1, h1), p3(c1, h1), p3(c0, h1)])
    f0, f1 = pos(i0, -(base - 14)), pos(i1, -(base - 14))
    g0, g1 = pos(i0, -(base + 4 * marche + 20)), pos(i1, -(base + 4 * marche + 20))
    polym((170, 40, 44), [p3(f0, 190), p3(f1, 190), p3(g1, 205), p3(g0, 205)])   # le toit

def dessiner_panneau(b):
    i0, i1, cote, (c1, c2) = b
    a0, a1 = pos(i0, cote * (MUR + 14)), pos(i1, cote * (MUR + 14))
    polym(c1, [p3(a0, 28), p3(a1, 28), p3(a1, 110), p3(a0, 110)])
    polym(c2, [p3(a0, 52), p3(a1, 52), p3(a1, 72), p3(a0, 72)])
    polym((60, 60, 66), [p3(a0, 20), p3(a1, 20), p3(a1, 28), p3(a0, 28)])

def dessiner_lampe(f, r, z, nuit):
    s = FOCAL / f
    sx, sy = W / 2 + r / f * FOCAL, HORIZON + (CAMZ - z) / f * FOCAL
    if sx < -100 or sx > W + 100:
        return
    pygame.draw.line(ecran, fondu((70, 72, 80), f), (sx, sy), (sx, sy - 230 * s), max(1, int(7 * s)))
    pygame.draw.rect(ecran, fondu((255, 244, 200) if nuit > 0.3 else (190, 190, 195), f), (sx - 14 * s, sy - 238 * s, 28 * s, 9 * s))

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
    derniers = "   ".join(f"T{etat['tours'] - len(etat['liste'][-4:]) + k + 1}: {t:.1f}s" for k, t in enumerate(etat["liste"][-4:]))
    ecran.blit(petite.render(derniers or "Termine un tour pour voir tes temps !", True, (180, 200, 255)), (W // 2 - 270, y + 6))
    ecran.blit(petite.render("Disques et pneus refroidis !", True, (120, 255, 160)), (W // 2 - 270, y + 34))

# ===== 9. CIEL, JOUR/NUIT, PLUIE, LUMIÈRES, COMPTEUR =====
def palette_ciel():
    n = meteo["nuit"]
    P = [((84, 150, 235), (190, 224, 255)), ((88, 66, 128), (255, 156, 88)), ((5, 8, 28), (22, 28, 66))]
    if n < 0.55:
        a, b, t = P[0], P[1], n / 0.55
    else:
        a, b, t = P[1], P[2], (n - 0.55) / 0.45
    haut, bas = melange(a[0], b[0], t), melange(a[1], b[1], t)
    g = int(135 * (1 - 0.85 * n))
    gris = (g - 5, g, g + 10)
    k = 0.65 * meteo["pluie"]
    return melange(haut, gris, k), melange(bas, gris, k * 0.8)

def ecart_angle(a, b):
    return (a - b + math.pi) % (2 * math.pi) - math.pi

def dessiner_ciel(haut, bas, nuit, pluie, temps):
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

def dessiner_lumieres(nuit):
    """Faisceaux des phares + feux arrière + lampadaires (ajoutés par-dessus l'image sombre)"""
    M = MODELE
    nez, py = M["nez"], M["phare_y"]
    LUMIERE.fill((0, 0, 0))
    for longueur, ouverture, couleur in [(720, 0.62, (20, 19, 13)), (600, 0.5, (32, 30, 20)), (470, 0.38, (46, 43, 29)),
                                         (350, 0.28, (64, 60, 40)), (240, 0.19, (88, 82, 54)), (140, 0.11, (118, 110, 72))]:
        for s in (-1, 1):
            pts = [loc2mon(nez + 40, s * py - 3, 0), loc2mon(nez + longueur, s * py - longueur * ouverture * 0.35, 0),
                   loc2mon(nez + longueur, s * py + longueur * ouverture * 0.35, 0), loc2mon(nez + 40, s * py + 3, 0)]
            poly(tuple(int(c * min(1, nuit * 1.4)) for c in couleur), [sol(p[0], p[1]) for p in pts], LUMIERE)
    for (lx, ly, lz) in M["feux_pos"]:
        wx, wy, wz = loc2mon(lx, ly, lz)
        f, r = cam(wx, wy)
        if f > NEAR:
            sc = FOCAL / f
            force = 0.5 + 0.5 * etat["frein"]
            pygame.draw.circle(LUMIERE, (int(150 * force), 10, 10), (W / 2 + r * sc, HORIZON + (CAMZ - wz) * sc), (5 + 9 * etat["frein"]) * sc)
    for (lx, ly) in LAMPES:                          # lampadaires
        f, r = cam(lx, ly)
        if NEAR < f < FAR:
            sc = FOCAL / f
            z = hauteur(lx, ly) + 234
            pygame.draw.circle(LUMIERE, (110, 98, 62), (W / 2 + r * sc, HORIZON + (CAMZ - z) * sc), max(2, 34 * sc))
            pygame.draw.circle(LUMIERE, (190, 170, 110), (W / 2 + r * sc, HORIZON + (CAMZ - z) * sc), max(1, 10 * sc))
    ecran.blit(LUMIERE, (0, 0), special_flags=pygame.BLEND_RGB_ADD)

def dessiner_compteur(v):
    """Le compteur de vitesse rond"""
    cx, cy, R = W - 150, H - 115, 92
    pygame.draw.circle(ecran, (10, 12, 20), (cx, cy), R + 8)
    pygame.draw.circle(ecran, (60, 66, 84), (cx, cy), R + 8, 3)
    vmax_aff = int(VITESSE_MAX * 0.3 / 50 + 1) * 50
    for k in range(0, 41):
        frac = k / 40
        a = math.radians(150 + 240 * frac)
        long_trait = 14 if k % 4 == 0 else 7
        coul = (235, 70, 60) if frac > 0.82 else (215, 220, 235)
        pygame.draw.line(ecran, coul, (cx + (R - long_trait) * math.cos(a), cy + (R - long_trait) * math.sin(a)),
                         (cx + R * math.cos(a), cy + R * math.sin(a)), 3 if k % 4 == 0 else 1)
    frac = max(0.0, min(1.0, abs(v) * 0.3 / vmax_aff))
    arc = [(cx + (R - 22) * math.cos(math.radians(150 + 240 * k / 30 * frac)), cy + (R - 22) * math.sin(math.radians(150 + 240 * k / 30 * frac))) for k in range(31)]
    if frac > 0.01:
        pygame.draw.lines(ecran, melange((80, 220, 130), (255, 70, 50), frac * 1.1), False, arc, 6)
    a = math.radians(150 + 240 * frac)
    pygame.draw.line(ecran, (255, 255, 255), (cx, cy), (cx + (R - 12) * math.cos(a), cy + (R - 12) * math.sin(a)), 3)
    pygame.draw.circle(ecran, (30, 34, 46), (cx, cy), 9)
    kmh = enorme.render(str(int(abs(v) * 0.3)), True, (255, 255, 255))
    ecran.blit(kmh, (cx - kmh.get_width() // 2, cy + 18))
    ecran.blit(petite.render("km/h", True, (170, 178, 200)), (cx - 18, cy + 64))
    rapport = "R" if v < -5 else "N" if v < 5 else str(min(6, 1 + int(frac * 6)))
    ecran.blit(police.render(rapport, True, (255, 210, 0)), (cx - 7, cy - 52))

PANNEAU_INFO = pygame.Surface((345, 108), pygame.SRCALPHA)
pygame.draw.rect(PANNEAU_INFO, (8, 10, 18, 160), (0, 0, 345, 108), border_radius=12)

# ===== 10. BOUCLE DE JEU =====
en_marche = True
img = 0
mode = "menu"
sel_car = 2
sel_col = VOITURES[sel_car]["defaut"]
MODELE = construire(sel_car, PALETTE[sel_col])
temps_menu = 0.0
while en_marche:
    dt = min(horloge.tick(60) / 1000, 0.05)
    img += 1
    for ev in pygame.event.get():
        if ev.type == pygame.QUIT:
            en_marche = False
        if ev.type == pygame.KEYDOWN:
            if ev.key == pygame.K_ESCAPE:
                en_marche = False
            if mode == "menu":
                if ev.key in (pygame.K_LEFT, pygame.K_q, pygame.K_a, pygame.K_RIGHT, pygame.K_d):
                    sel_car = (sel_car + (-1 if ev.key in (pygame.K_LEFT, pygame.K_q, pygame.K_a) else 1)) % len(VOITURES)
                    sel_col = VOITURES[sel_car]["defaut"]
                    MODELE = construire(sel_car, PALETTE[sel_col])
                if ev.key in (pygame.K_UP, pygame.K_DOWN, pygame.K_c):
                    sel_col = (sel_col + (-1 if ev.key == pygame.K_UP else 1)) % len(PALETTE)
                    MODELE = construire(sel_car, PALETTE[sel_col])
                if ev.key in (pygame.K_RETURN, pygame.K_SPACE):
                    appliquer_voiture(sel_car, sel_col)
                    etat = reset()
                    particules.clear()
                    marques.clear()
                    mode = "jeu"
            else:
                if ev.key == pygame.K_r:
                    etat = reset()
                    particules.clear()
                    marques.clear()
                if ev.key == pygame.K_m:
                    mode = "menu"
                    etat = reset()
                    particules.clear()
                    marques.clear()
                    MODELE = construire(sel_car, PALETTE[sel_col])
            if ev.key == pygame.K_n:
                meteo["phase"] = (meteo["phase"] + 1) % 3
            if ev.key == pygame.K_p:
                meteo["pluie_on"] = not meteo["pluie_on"]
            if ev.key == pygame.K_g:
                QUALITE = (QUALITE + 1) % 3

    if mode == "menu":
        temps_menu += dt
        FOCAL = FOCAL_BASE
        dessiner_menu(temps_menu)
        pygame.display.flip()
        continue

    t = pygame.key.get_pressed()
    gaz = t[pygame.K_UP] or t[pygame.K_z] or t[pygame.K_w]
    frein = t[pygame.K_DOWN] or t[pygame.K_s]
    droite = (1 if (t[pygame.K_RIGHT] or t[pygame.K_d]) else 0) - (1 if (t[pygame.K_LEFT] or t[pygame.K_q] or t[pygame.K_a]) else 0)

    # --- Météo ---
    meteo["nuit"] += ([0.0, 0.55, 1.0][meteo["phase"]] - meteo["nuit"]) * min(1, 0.8 * dt)
    meteo["pluie"] += ((1.0 if meteo["pluie_on"] else 0.0) - meteo["pluie"]) * min(1, 0.6 * dt)
    nuit, pluie = meteo["nuit"], meteo["pluie"]

    # --- Où sont les roues ? (herbe progressive : chaque roue compte) ---
    numero, distance, dec = info_piste(etat["x"], etat["y"])
    en_stand = numero in PITSET and DEMI_LARGEUR + KERB < dec < PIT_FIN
    nb_herbe = sum(1 for lx, ly in ROUES if roue_dehors(*loc2mon(lx, ly, 0)[:2], numero))
    if nb_herbe == 4 and not etat["dehors4"]:
        etat["chrono"] += 1.0                          # LIMITES DE PISTE : +1 seconde !
        etat["msg"] = 2.0
    etat["dehors4"] = nb_herbe == 4
    etat["msg"] = max(0, etat["msg"] - dt)

    # --- Pédales ---
    gaz_avant = etat["gaz"]
    etat["gaz"] = min(1, etat["gaz"] + PEDALE_GAZ * dt) if gaz else max(0, etat["gaz"] - 5 * dt)
    etat["frein"] = min(1, etat["frein"] + PEDALE_FREIN * dt) if frein else max(0, etat["frein"] - 3 * dt)

    # --- Grip : appui aéro, pluie, herbe ---
    v = etat["v"]
    appui = 1 + APPUI * (abs(v) / VITESSE_MAX) ** 2
    adherence = appui * (1 - 0.3 * pluie) * (1 - HERBE_GRIP * nb_herbe / 4)
    grip_av, grip_ar = GRIP_AV * adherence, GRIP_AR * adherence

    # --- Direction ---
    braquage_max = BRAQUAGE / (1 + abs(v) / VSENS)
    etat["volant"] += (droite * braquage_max - etat["volant"]) * min(1, 8 * dt)
    rotation = v * math.tan(etat["volant"]) / EMPATTEMENT
    a_lat = abs(v * rotation)

    a_frein = FREINAGE * etat["frein"] ** 1.5 if v > 0.5 else 0
    usage_av = math.hypot(a_frein, a_lat)
    avant_glisse = usage_av > grip_av
    if avant_glisse:
        rotation *= grip_av / usage_av

    a_moteur = PUISSANCE * etat["gaz"] * (1 - max(v, 0) / VITESSE_MAX)
    usage_ar = math.hypot(a_moteur, a_lat)
    arriere_patine = usage_ar > grip_ar
    if arriere_patine:
        exces = usage_ar / grip_ar - 1
        rotation *= 1 + 2 * exces
        a_moteur *= grip_ar / usage_ar

    # --- Relief : ça monte, ça descend ---
    ca, sa = math.cos(etat["angle"]), math.sin(etat["angle"])
    pente = (hauteur(etat["x"] + ca * 6, etat["y"] + sa * 6) - hauteur(etat["x"], etat["y"])) / 6

    # --- Vitesse ---
    v += a_moteur * dt
    if v > 0.5:
        v -= a_frein * dt
    elif frein:
        v -= 250 * dt
    v -= v * (0.3 if gaz else 0.6) * dt
    v -= v * HERBE_FREIN * nb_herbe * dt               # herbe : progressif, roue par roue
    v -= pente * PENTE_FORCE * dt
    etat["v"] = v = max(-150, min(VITESSE_MAX, v))
    etat["vmax"] = max(etat["vmax"], abs(v))

    etat["angle"] += rotation * dt
    etat["x"] = max(0, min(MONDE[0], etat["x"] + math.cos(etat["angle"]) * v * dt))
    etat["y"] = max(0, min(MONDE[1], etat["y"] + math.sin(etat["angle"]) * v * dt))
    # --- Rails de sécurité : on ne les traverse pas ! ---
    cote_stands = numero in PITSET and dec > 0
    limite = (PIT_D2 + 25) if cote_stands else (MUR - 18)
    if distance > limite:
        cxm, cym = centre[numero]
        nxm, nym = (etat["x"] - cxm) / distance, (etat["y"] - cym) / distance
        etat["x"], etat["y"] = cxm + nxm * limite, cym + nym * limite
        vers_mur = max(0.0, math.cos(etat["angle"]) * nxm + math.sin(etat["angle"]) * nym)
        if vers_mur > 0.05 and v > 30:
            v *= 1 - 0.6 * vers_mur
            etat["v"] = v
            ta = math.atan2(-NORMALES[numero][0], NORMALES[numero][1])
            if math.cos(etat["angle"] - ta) < 0:
                ta += math.pi
            etat["angle"] += ((ta - etat["angle"] + math.pi) % (2 * math.pi) - math.pi) * min(1, 6 * dt)
            if vers_mur > 0.25:
                etat["secousse"] = 3.0
                laterale = nxm * -math.sin(etat["angle"]) + nym * math.cos(etat["angle"])
                for _ in range(4):
                    emettre("etincelle", 0, 20 if laterale > 0 else -20, 8, -random.uniform(20, 120), 0.0,
                            random.uniform(30, 120), random.uniform(0.2, 0.5), 1)
    etat["z"] = hauteur(etat["x"], etat["y"])
    etat["cz"] += (etat["z"] - etat["cz"]) * min(1, 4 * dt)

    # --- Disques de frein ---
    if etat["frein"] > 0.05 and v > 0:
        etat["chaleur"] += etat["frein"] * (abs(v) / VITESSE_MAX) * 1.6 * dt
    else:
        etat["chaleur"] -= 0.4 * dt
    if en_stand:
        etat["chaleur"] = 0
    etat["chaleur"] = max(0, min(1, etat["chaleur"]))

    # --- Suspensions : la carrosserie PIVOTE (en radians) ---
    lisse = min(1, 6 * dt)
    cible_t = (etat["frein"] * 0.05 - etat["gaz"] * 0.03 * (1 - abs(v) / VITESSE_MAX)) * TANGAGE_K - pente
    etat["tangage"] += (cible_t - etat["tangage"]) * lisse
    etat["roulis"] += (max(-ROULIS_MAX, min(ROULIS_MAX, ROULIS_K * v * rotation)) - etat["roulis"]) * lisse
    cible_aile = 0.1 + 0.25 * (abs(v) / VITESSE_MAX) + 0.9 * etat["frein"]
    etat["aile"] += (cible_aile - etat["aile"]) * min(1, 10 * dt)
    etat["roue"] += v * dt / 10 * 0.35

    # --- Effets ---
    avants = [w for w in MODELE["roues"] if w[2]]
    if etat["frein"] > 0.3 and etat["chaleur"] > 0.3 and v > 120:
        for (cx, cy, _, rayon, _) in avants:           # ÉTINCELLES aux disques avant
            cote = 1 if cy > 0 else -1
            for _ in range(2):
                emettre("etincelle", cx, cy, rayon * 0.8, -random.uniform(40, 160), cote * random.uniform(10, 90),
                        random.uniform(40, 140), random.uniform(0.25, 0.6), 1)
    if (not gaz and gaz_avant > 0.35 and v > 250) or (etat["frein"] > 0.5 and v > 350 and random.random() < 0.12):
        for (ex, ey, ez) in MODELE["echap"]:           # FLAMMES de l'échappement
            for _ in range(3):
                emettre("flamme", ex - 2, ey, ez, -random.uniform(60, 180), random.uniform(-20, 20),
                        random.uniform(-10, 30), random.uniform(0.12, 0.28), random.uniform(5, 9))
    gros_freinage = etat["frein"] > 0.9 and v > 350
    glisse = (arriere_patine or avant_glisse or gros_freinage) and nb_herbe < 2
    if glisse and img % 2 == 0:
        for (cx, cy, avant, rayon, _) in MODELE["roues"]:      # FUMÉE des pneus qui glissent
            if (avant and (avant_glisse or gros_freinage)) or (not avant and arriere_patine):
                emettre("fumee", cx, cy, 3, -random.uniform(0, 30), random.uniform(-20, 20),
                        random.uniform(15, 40), random.uniform(0.6, 1.2), random.uniform(7, 11))
    for k, (lx, ly) in enumerate(ROUES):
        wx, wy, wz = loc2mon(lx, ly, 0)
        if glisse:
            if derniere[k]:
                marques.append((derniere[k][0], derniere[k][1], derniere[k][2], wx, wy, wz))
            derniere[k] = (wx, wy, wz)
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

    # --- Caméra : derrière la voiture avec un peu de retard ; le champ de vision s'élargit avec la vitesse ---
    ecart = (etat["angle"] - etat["cam"] + math.pi) % (2 * math.pi) - math.pi
    etat["cam"] += ecart * min(1, 3.5 * dt)
    FOCAL = FOCAL_BASE * (1 - 0.10 * (abs(v) / VITESSE_MAX) ** 1.5)
    FAR = FAR_Q[QUALITE]
    BRUME_DIST = FAR * (1 - 0.45 * pluie)
    sur_vibreur = DEMI_LARGEUR < distance <= DEMI_LARGEUR + KERB and not en_stand
    cible_sec = 0.25 * (abs(v) / VITESSE_MAX) ** 2 + (0.9 if sur_vibreur and abs(v) > 60 else 0.0)
    etat["secousse"] += (cible_sec - etat["secousse"]) * min(1, 10 * dt)
    angle_cam = etat["cam"] + random.uniform(-1, 1) * 0.0012 * etat["secousse"]
    cs, sn = math.cos(angle_cam), math.sin(angle_cam)
    camx, camy = etat["x"] - cs * CAMDIST, etat["y"] - sn * CAMDIST
    CAMZ = etat["cz"] + CAMH + random.uniform(-1, 1) * 0.6 * etat["secousse"]

    # --- Ciel, soleil, nuages, montagnes, sol ---
    haut, bas = palette_ciel()
    BRUME = bas
    dessiner_ciel(haut, bas, nuit, pluie, img / 60)
    for y in range(HORIZON, H, 5):
        pygame.draw.rect(ecran, melange(bas, HERBE, ((y - HORIZON) / (H - HORIZON)) ** 0.5), (0, y, W, 5))

    T = 450
    gi, gj = int(etat["x"] // T), int(etat["y"] // T)
    carreaux = []
    for i in range(gi - 5, gi + 6):
        for j in range(gj - 5, gj + 6):
            if (i + j) % 2 == 0:
                coul = HERBE2
            elif (i * 5 + j * 3) % 7 == 0:
                coul = HERBE3
            else:
                continue
            pts = [sol(i*T, j*T), sol(i*T+T, j*T), sol(i*T+T, j*T+T), sol(i*T, j*T+T)]
            carreaux.append((sum(p[0] for p in pts), coul, pts))
    carreaux.sort(key=lambda c: c[0], reverse=True)
    for _, coul, pts in carreaux:
        polym(coul, pts)

    # --- La piste : plus c'est loin, moins on dessine de petits morceaux (pour garder un jeu fluide) ---
    asph1, asph2 = melange(ASPHALTE, (46, 50, 62), pluie), melange(ASPHALTE2, (40, 44, 56), pluie)
    trace = melange(asph1, (14, 14, 16), 0.2)
    visibles = {}
    for i in range(N):
        f = (centre[i][0] - camx) * cs + (centre[i][1] - camy) * sn
        if -250 < f < FAR:
            visibles[i] = f
    base = (numero + N // 2) % N
    segments = []
    p = 0
    while p < N:
        i = (base + p) % N
        f = visibles.get(i)
        if f is None:
            p += 1
            continue
        s = 1 if f < 600 else 2 if f < 1100 else 3 if f < 1700 else 4
        j = (base + p + s) % N
        if p + s < N and j in visibles:
            segments.append((i, j))
            p += s
        else:
            j = (i + 1) % N
            if p + 1 < N and j in visibles:
                segments.append((i, j))
            p += 1
    segments.sort(key=lambda sg: visibles[sg[0]], reverse=True)
    cache, cache_pit = {}, {}
    for (i, j) in segments:
        for q in (i, j):
            if q not in cache:
                cache[q] = [sol(*ligne[q]) for ligne in LIGNES]
        a, b = cache[i], cache[j]
        vibreur = ROUGE if int(DIST[i] // 45) % 2 == 0 else BLANC
        polym(vibreur, [a[0], a[1], b[1], b[0]])
        polym(vibreur, [a[2], a[3], b[3], b[2]])
        polym(asph1 if int(DIST[i] // 90) % 2 == 0 else asph2, [a[1], a[2], b[2], b[1]])
        polym(trace, [a[4], a[5], b[5], b[4]])               # la trace de gomme au centre
        if i in PITSET and j in PITSET:
            for q in (i, j):
                if q not in cache_pit:
                    cache_pit[q] = [sol(*ligne[q]) for ligne in LIGNES_PIT]
            pa, pb = cache_pit[i], cache_pit[j]
            polym(melange((100, 100, 108), (60, 64, 76), pluie), [pa[0], pa[2], pb[2], pb[0]])
            polym((255, 210, 0) if int(DIST[i] // 60) % 2 == 0 else NOIR, [pa[1], pa[2], pb[2], pb[1]])
        if QUALITE >= 1 and visibles[i] < 1000:              # les rails de sécurité
            for cote, ligne in ((0, 6), (1, 7)):
                if cote == 0 and (i in PITSET or j in PITSET):
                    continue
                m0, m1 = a[ligne], b[ligne]
                h0, h1 = (m0[0], m0[1], m0[2] + 26), (m1[0], m1[1], m1[2] + 26)
                polym((196, 199, 206) if int(DIST[i] // 70) % 2 == 0 else (172, 175, 183), [m0, m1, h1, h0])
                if QUALITE >= 2:
                    polym((96, 100, 112), [(m0[0], m0[1], m0[2] + 17), (m1[0], m1[1], m1[2] + 17), h1, h0])

    for (x1, y1, z1, x2, y2, z2) in marques:
        f1, r1 = cam(x1, y1)
        f2, r2 = cam(x2, y2)
        if NEAR < f1 < FAR and f2 > NEAR:
            pygame.draw.line(ecran, fondu((24, 24, 26), f1), (W / 2 + r1 / f1 * FOCAL, HORIZON + (CAMZ - z1) / f1 * FOCAL),
                             (W / 2 + r2 / f2 * FOCAL, HORIZON + (CAMZ - z2) / f2 * FOCAL), max(1, int(6 * FOCAL / f1)))

    # --- Décors, effets et voiture, du plus loin au plus proche ---
    objets = [(cam(etat["x"], etat["y"])[0], "voiture", None)]
    for (ax, ay, az, genre, taille) in ARBRES:
        dx, dy = ax - camx, ay - camy
        f = dx * cs + dy * sn
        if NEAR < f < FAR:
            objets.append((f, "arbre", (-dx * sn + dy * cs, az, genre, taille)))
    for g in GARAGES:
        f = cam(*centre[g[5]])[0]
        if NEAR < f < FAR + 300:
            objets.append((f, "garage", g))
    if QUALITE >= 1:
        for t in TRIBUNES:
            f = cam(*centre[((t[0] + t[1]) // 2) % N])[0]
            if NEAR < f < FAR:
                objets.append((f, "tribune", t))
        for b in PANNEAUX:
            f = cam(*centre[((b[0] + b[1]) // 2) % N])[0]
            if NEAR < f < FAR:
                objets.append((f, "panneau", b))
        for (lx, ly) in LAMPES:
            f, r = cam(lx, ly)
            if NEAR < f < FAR:
                objets.append((f, "lampe", (r, hauteur(lx, ly))))
    for pa_ in particules:
        f = cam(pa_[0], pa_[1])[0]
        if f > NEAR:
            objets.append((f, "particule", pa_))
    objets.append((cam(*centre[0])[0], "portique", None))
    objets.sort(key=lambda o: o[0], reverse=True)
    for f, genre, donnees in objets:
        if genre == "arbre":
            dessiner_arbre(f, *donnees)
        elif genre == "garage":
            dessiner_garage(donnees)
        elif genre == "tribune":
            dessiner_tribune(donnees)
        elif genre == "panneau":
            dessiner_panneau(donnees)
        elif genre == "lampe":
            dessiner_lampe(f, donnees[0], donnees[1], nuit)
        elif genre == "particule":
            dessiner_particule(f, donnees)
        elif genre == "portique":
            dessiner_portique()
        else:
            dessiner_voiture(nuit)

    # --- Pluie, gouttes sur la caméra, nuit, phares, coins sombres ---
    if pluie > 0.05:
        for g in gouttes[:int(len(gouttes) * pluie)]:
            g[1] += g[2] * dt
            g[0] -= g[2] * 0.12 * dt
            if g[1] > H:
                g[0], g[1] = random.random() * W + 100, -20
            pygame.draw.line(ecran, (185, 195, 220), (g[0], g[1]), (g[0] + 2, g[1] + 16), 1)
        if QUALITE >= 1 and random.random() < 0.3 * pluie:
            lentille.append([random.uniform(0, W), random.uniform(0, H * 0.85), random.uniform(3, 8), 1.0])
    for gl in lentille:
        gl[3] -= dt * 0.35
        pygame.draw.circle(ecran, (205, 215, 235), (gl[0], gl[1]), gl[2], 1)
        pygame.draw.circle(ecran, (240, 245, 255), (gl[0] - gl[2] * 0.3, gl[1] - gl[2] * 0.3), max(1, gl[2] * 0.25))
    lentille[:] = [gl for gl in lentille if gl[3] > 0]
    alpha = int(185 * nuit ** 0.9 + 70 * pluie * (1 - nuit))
    if alpha > 2:
        couleur_nuit = melange((50, 55, 70), melange((30, 15, 45), (4, 6, 24), (nuit - 0.55) / 0.45), min(1, nuit * 2))
        OMBRE.fill((*couleur_nuit, min(230, alpha)))
        ecran.blit(OMBRE, (0, 0))
    if nuit > 0.2:
        dessiner_lumieres(nuit)
    if QUALITE >= 1:
        ecran.blit(VIGNETTE, (0, 0))

    # --- HUD ---
    blanc = (255, 255, 255)
    ecran.blit(PANNEAU_INFO, (10, 10))
    ecran.blit(police.render(f"Tour {etat['tours'] + 1}   {etat['chrono']:.1f}s", True, blanc), (24, 20))
    ecran.blit(police.render(f"Meilleur : {fmt(etat['meilleur'])}", True, (255, 220, 0)), (24, 48))
    ecran.blit(petite.render(f"{VOITURES[sel_car]['nom']}   {int(horloge.get_fps())} FPS   Qualite {['ECO', 'ELEVEE', 'ULTRA'][QUALITE]} (G)", True, (180, 188, 210)), (24, 80))
    for k, (nom, valeur, couleur) in enumerate([("GAZ", etat["gaz"], (60, 220, 60)), ("FREIN", etat["frein"], (240, 60, 60)),
                                                ("DISQ", etat["chaleur"], (255, 150, 30))]):
        bx = 20 + k * 58
        pygame.draw.rect(ecran, (8, 10, 18), (bx, H - 130, 40, 100), border_radius=6)
        pygame.draw.rect(ecran, couleur, (bx, H - 30 - 100 * valeur, 40, 100 * valeur), border_radius=6)
        pygame.draw.rect(ecran, (200, 205, 220), (bx, H - 130, 40, 100), 2, border_radius=6)
        ecran.blit(petite.render(nom, True, blanc), (bx - 4, H - 26))
    ecran.blit(petite.render(f"GRIP : {int(adherence * 100)}%   (appui aero +{int((appui - 1) * 100)}%)", True, (160, 220, 255)), (W // 2 - 150, H - 28))
    if nb_herbe == 4:
        ecran.blit(police.render("HORS PISTE !", True, (255, 80, 80)), (W // 2 - 80, 20))
    elif nb_herbe > 0:
        ecran.blit(police.render(f"{nb_herbe} roue(s) dans l'herbe", True, (255, 200, 60)), (W // 2 - 140, 20))
    if etat["msg"] > 0:
        ecran.blit(grande.render("LIMITES DE PISTE : +1s", True, (255, 80, 80)), (W // 2 - 210, 150))
    if avant_glisse:
        ecran.blit(police.render("AVANT QUI GLISSE : trop de frein en virage !", True, (255, 160, 40)), (W // 2 - 230, 50))
    if arriere_patine:
        ecran.blit(police.render("ARRIERE QUI PATINE : trop de gaz !", True, (255, 160, 40)), (W // 2 - 190, 80))
    mx, my = W - mini.get_width() - 10, 10
    ecran.blit(mini, (mx, my))
    px, py = mx + 10 + etat["x"] * ECH, my + 10 + etat["y"] * ECH
    pygame.draw.circle(ecran, (255, 60, 60), (px, py), 6)
    pygame.draw.line(ecran, (255, 255, 255), (px, py), (px + 13 * math.cos(etat["angle"]), py + 13 * math.sin(etat["angle"])), 3)
    nom_meteo = ["JOUR", "CREPUSCULE", "NUIT"][meteo["phase"]] + (" + PLUIE" if meteo["pluie_on"] else "")
    ecran.blit(petite.render(nom_meteo, True, blanc), (mx, my + mini.get_height() + 6))
    dessiner_compteur(v)
    ecran.blit(petite.render("N = jour/nuit  P = pluie  G = qualite  M = menu", True, (200, 200, 200)), (W // 2 - 210, H - 52))
    if en_stand:
        afficher_stands()

    pygame.display.flip()

pygame.quit()
