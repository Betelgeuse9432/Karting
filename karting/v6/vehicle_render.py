"""Rendu des quatre modèles V6 d'origine, avec état passé explicitement."""
import math
import pygame
from .mesh import boite, construire, melange, teinte


class V6VehicleRenderer:
    def __init__(self, vehicle_index: int, color: tuple[int, int, int]):
        self.model = construire(vehicle_index, color)

    def draw(self, screen: pygame.Surface, camera, car, ground_height: float,
             night: float = 0.0, fog_color=(170, 215, 255), fog_distance=2100.0) -> None:
        """Dessine le modèle sélectionné sans modifier la physique ou l'état de course."""
        etat = dict(x=car.x, y=car.y, z=ground_height, angle=car.angle,
                    tangage=car.pitch, roulis=car.roll, volant=car.steering,
                    chaleur=car.heat, roue=car.wheel_rotation, aile=car.wing,
                    frein=car.brake)
        MODELE = self.model
        camx, camy, cs, sn = camera.x, camera.y, camera.cs, camera.sn
        nuit = night

        def poly(color, faces):
            projected = camera.project(faces)
            if projected:
                f = max(0, sum(p[0] for p in faces)/len(faces))
                intensity = max(0.0, min(1.0, f/fog_distance))**1.6 * 0.92
                tint = melange(color, fog_color, intensity)
                pygame.draw.polygon(screen, tint, projected)

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
