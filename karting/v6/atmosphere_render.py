"""Effets météorologiques V6 : nuit, feux, lampes et pluie. Optionnels."""
import random
import pygame
from .track import height
from .mesh import melange


class V6AtmosphereRenderer:
    def __init__(self,config,seed=7):
        self.rng=random.Random(seed+1)
        self.dark=pygame.Surface((config.width,config.height),pygame.SRCALPHA)
        self.LUMIERE=pygame.Surface((config.width,config.height))
        self.drops=[[self.rng.random()*config.width,self.rng.random()*config.height,
                    self.rng.uniform(700,1100)] for _ in range(170)]
        self.lens=[]

    def draw(self,screen,camera,car,model,lamps,weather,dt,quality):
        W,H=camera.cfg.width,camera.cfg.height
        night,rain=weather["nuit"],weather["pluie"]
        if rain>0.05:
            for d in self.drops[:int(len(self.drops)*rain)]:
                d[1]+=d[2]*dt
                d[0]-=d[2]*0.12*dt
                if d[1]>H:
                    d[0],d[1]=self.rng.random()*W+100,-20
                pygame.draw.line(screen,(185,195,220),(d[0],d[1]),(d[0]+2,d[1]+16),1)
            if quality>=1 and self.rng.random()<0.3*rain:
                self.lens.append([self.rng.uniform(0,W),self.rng.uniform(0,H*0.85),
                                  self.rng.uniform(3,8),1.0])
        for g in self.lens:
            g[3]-=dt*0.35
            pygame.draw.circle(screen,(205,215,235),(g[0],g[1]),g[2],1)
            pygame.draw.circle(screen,(240,245,255),(g[0]-g[2]*0.3,g[1]-g[2]*0.3),
                               max(1,g[2]*0.25))
        self.lens[:]=[g for g in self.lens if g[3]>0]
        alpha=int(185*night**0.9+70*rain*(1-night))
        if alpha>2:
            night_color=melange((50,55,70),melange((30,15,45),(4,6,24),
                                                  (night-0.55)/0.45),min(1,night*2))
            self.dark.fill((*night_color,min(230,alpha)))
            screen.blit(self.dark,(0,0))
        if night>0.2:
            self.lights(screen,camera,car,model,lamps,night)

    def lights(self,screen,camera,car,model,lamps,night):
        W,H=camera.cfg.width,camera.cfg.height
        HORIZON,FOCAL=camera.cfg.horizon,camera.focal
        CAMZ,NEAR,FAR=camera.z,camera.cfg.near,camera.far
        ecran=screen
        LUMIERE=self.LUMIERE
        MODELE=model
        LAMPES=lamps
        etat={"frein":car.brake}
        cam=camera.position
        hauteur=height
        def loc2mon(lx,ly,lz):
            x,y,_=car.local_to_world(lx,ly,lz)
            return x,y,height(car.x,car.y)+lz
        sol=camera.ground
        def poly(colour,points,target=None):
            transformed=camera.project(points)
            if transformed:pygame.draw.polygon(target or screen,colour,transformed)
        nuit=night
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
