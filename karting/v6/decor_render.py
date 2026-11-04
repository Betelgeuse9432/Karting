"""Décors de V6 en profondeur : arbres, garages, portique, tribunes, publicités, lampes."""
import pygame
from .mesh import melange
from .track import height


def milieu(a,b,t):
    return (a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t)


BLANC=(240,240,240)
NOIR=(25,25,25)


class V6DecorRenderer:
    def __init__(self,world):
        self.world=world

    def draw_tree(self,screen,camera,track,weather,quality,tree):
        ecran=screen
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR=camera.z,camera.cfg.near
        QUALITE=quality
        hauteur=height
        MUR=track.mur
        LIGNES=track.lines
        pos=lambda i,d:track.position(i%track.count,d)
        cam=camera.position
        def c3(x,y,z):
            return camera.space(x,y,z)
        def p3(q,dz=0.0):
            return camera.space(q[0],q[1],height(q[0],q[1])+dz)
        def polym(colour,pts):
            return self.world.polygon(screen,camera,colour,pts,weather)
        def fondu(colour,f):
            intensity=max(0,min(1,f/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            return melange(colour,weather.get("fog_color",(170,215,255)),intensity)

        f,r=cam(tree.x,tree.y)
        if not (NEAR<f<camera.far):return
        z,genre,taille=height(tree.x,tree.y),tree.kind,tree.scale
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

    def draw_garage(self,screen,camera,track,weather,quality,garage):
        ecran=screen
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR=camera.z,camera.cfg.near
        QUALITE=quality
        hauteur=height
        MUR=track.mur
        LIGNES=track.lines
        pos=lambda i,d:track.position(i%track.count,d)
        cam=camera.position
        def c3(x,y,z):
            return camera.space(x,y,z)
        def p3(q,dz=0.0):
            return camera.space(q[0],q[1],height(q[0],q[1])+dz)
        def polym(colour,pts):
            return self.world.polygon(screen,camera,colour,pts,weather)
        def fondu(colour,f):
            intensity=max(0,min(1,f/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            return melange(colour,weather.get("fog_color",(170,215,255)),intensity)

        g=(garage.a0,garage.a1,garage.b0,garage.b1,garage.door_color,garage.track_index)
        A0, A1, B0, B1, porte, _ = g
        h = 60
        polym((52, 54, 64), [p3(A0, h), p3(A1, h), p3(B1, h), p3(B0, h)])
        polym((214, 216, 222), [p3(A0, 0), p3(A1, 0), p3(A1, h), p3(A0, h)])
        polym((60, 62, 74), [p3(A0, h - 8), p3(A1, h - 8), p3(A1, h), p3(A0, h)])
        for k in range(3):                              # trois portes de garage par bâtiment
            d0, d1 = milieu(A0, A1, 0.06 + k * 0.31), milieu(A0, A1, 0.31 + k * 0.31)
            polym(porte, [p3(d0, 0), p3(d1, 0), p3(d1, 40), p3(d0, 40)])
            polym((30, 32, 40), [p3(d0, 40), p3(d1, 40), p3(d1, 44), p3(d0, 44)])

    def draw_gantry(self,screen,camera,track,weather,quality):
        ecran=screen
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR=camera.z,camera.cfg.near
        QUALITE=quality
        hauteur=height
        MUR=track.mur
        LIGNES=track.lines
        pos=lambda i,d:track.position(i%track.count,d)
        cam=camera.position
        def c3(x,y,z):
            return camera.space(x,y,z)
        def p3(q,dz=0.0):
            return camera.space(q[0],q[1],height(q[0],q[1])+dz)
        def polym(colour,pts):
            return self.world.polygon(screen,camera,colour,pts,weather)
        def fondu(colour,f):
            intensity=max(0,min(1,f/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            return melange(colour,weather.get("fog_color",(170,215,255)),intensity)

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

    def draw_tribune(self,screen,camera,track,weather,quality,t):
        ecran=screen
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR=camera.z,camera.cfg.near
        QUALITE=quality
        hauteur=height
        MUR=track.mur
        LIGNES=track.lines
        pos=lambda i,d:track.position(i%track.count,d)
        cam=camera.position
        def c3(x,y,z):
            return camera.space(x,y,z)
        def p3(q,dz=0.0):
            return camera.space(q[0],q[1],height(q[0],q[1])+dz)
        def polym(colour,pts):
            return self.world.polygon(screen,camera,colour,pts,weather)
        def fondu(colour,f):
            intensity=max(0,min(1,f/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            return melange(colour,weather.get("fog_color",(170,215,255)),intensity)

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

    def draw_ad(self,screen,camera,track,weather,quality,b):
        ecran=screen
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR=camera.z,camera.cfg.near
        QUALITE=quality
        hauteur=height
        MUR=track.mur
        LIGNES=track.lines
        pos=lambda i,d:track.position(i%track.count,d)
        cam=camera.position
        def c3(x,y,z):
            return camera.space(x,y,z)
        def p3(q,dz=0.0):
            return camera.space(q[0],q[1],height(q[0],q[1])+dz)
        def polym(colour,pts):
            return self.world.polygon(screen,camera,colour,pts,weather)
        def fondu(colour,f):
            intensity=max(0,min(1,f/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            return melange(colour,weather.get("fog_color",(170,215,255)),intensity)

        i0, i1, cote, (c1, c2) = b
        a0, a1 = pos(i0, cote * (MUR + 14)), pos(i1, cote * (MUR + 14))
        polym(c1, [p3(a0, 28), p3(a1, 28), p3(a1, 110), p3(a0, 110)])
        polym(c2, [p3(a0, 52), p3(a1, 52), p3(a1, 72), p3(a0, 72)])
        polym((60, 60, 66), [p3(a0, 20), p3(a1, 20), p3(a1, 28), p3(a0, 28)])

    def draw_lamp(self,screen,camera,track,weather,quality,x,y,night):
        ecran=screen
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR=camera.z,camera.cfg.near
        QUALITE=quality
        hauteur=height
        MUR=track.mur
        LIGNES=track.lines
        pos=lambda i,d:track.position(i%track.count,d)
        cam=camera.position
        def c3(x,y,z):
            return camera.space(x,y,z)
        def p3(q,dz=0.0):
            return camera.space(q[0],q[1],height(q[0],q[1])+dz)
        def polym(colour,pts):
            return self.world.polygon(screen,camera,colour,pts,weather)
        def fondu(colour,f):
            intensity=max(0,min(1,f/(camera.far*(1-0.45*weather["pluie"]))))**1.6*0.92
            return melange(colour,weather.get("fog_color",(170,215,255)),intensity)

        f,r=cam(x,y)
        if not (NEAR<f<camera.far):return
        z=height(x,y)
        nuit=night
        s = FOCAL / f
        sx, sy = W / 2 + r / f * FOCAL, HORIZON + (CAMZ - z) / f * FOCAL
        if sx < -100 or sx > W + 100:
            return
        pygame.draw.line(ecran, fondu((70, 72, 80), f), (sx, sy), (sx, sy - 230 * s), max(1, int(7 * s)))
        pygame.draw.rect(ecran, fondu((255, 244, 200) if nuit > 0.3 else (190, 190, 195), f), (sx - 14 * s, sy - 238 * s, 28 * s, 9 * s))
    
    # ===== 8. TABLEAU DES SCORES DES STANDS =====
