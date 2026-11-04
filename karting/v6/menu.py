"""Menu V6 : choix du véhicule, de la couleur, stats et modèle 3D tournant."""
import math
import pygame
from ..state import VehicleState
from .camera import V6Camera
from .mesh import melange
from .vehicles import PALETTE,VOITURES


class V6MenuRenderer:
    def __init__(self,config):
        self.cfg=config
        self.font=pygame.font.SysFont("monospace",22,bold=True)
        self.small=pygame.font.SysFont("monospace",16,bold=True)
        self.big=pygame.font.SysFont("monospace",34,bold=True)

    def draw(self,screen,renderer,index,color,time):
        W,H=self.cfg.width,self.cfg.height
        horizon=self.cfg.horizon
        profile=VOITURES[index]
        screen.fill((18,20,30))
        pygame.draw.rect(screen,(26,30,44),(0,0,W,horizon))
        pygame.draw.rect(screen,(40,44,54),(0,horizon,W,H-horizon))
        camera=V6Camera(self.cfg,profile)
        camera.x=-renderer.model["menu_dist"]
        camera.y=0
        camera.z=45
        camera.cs,camera.sn=1,0
        camera.focal=self.cfg.focal
        car=VehicleState(0.0,0.0,time*0.7,brake=1.0 if int(time*1.2)%2 else 0.0,
                         pitch=0.01*math.sin(time*2),roll=0.015*math.sin(time*1.3),
                         wing=0.1+0.45*(0.5+0.5*math.sin(time*2)),
                         wheel_rotation=time*3)
        ring=[camera.space(95*math.cos(k*2*math.pi/28),
                           95*math.sin(k*2*math.pi/28),0) for k in range(28)]
        vertices=camera.project(ring)
        if vertices:pygame.draw.polygon(screen,(72,78,92),vertices)
        renderer.draw(screen,camera,car,0,0,fog_distance=1e9)
        title=self.big.render("CHOISIS TA VOITURE",True,(255,210,0))
        screen.blit(title,(W//2-title.get_width()//2,16))
        for i,vehicle in enumerate(VOITURES):
            x=W//2-400+i*200
            active=i==index
            pygame.draw.rect(screen,(255,210,0) if active else (50,56,72),(x,70,180,40),border_radius=6)
            text=self.font.render(vehicle["nom"],True,(20,20,20) if active else (230,230,230))
            screen.blit(text,(x+90-text.get_width()//2,77))
        screen.blit(self.big.render("<",True,(255,210,0)),(W//2-340,330))
        screen.blit(self.big.render(">",True,(255,210,0)),(W//2+320,330))
        screen.blit(self.font.render("STATS",True,(255,255,255)),(50,150))
        for k,(name,value) in enumerate(zip(("Vitesse","Acceleration","Freinage","Grip","Appui aero","Herbe"),profile["stats"])):
            y=195+k*40
            screen.blit(self.small.render(name,True,(200,205,220)),(50,y))
            pygame.draw.rect(screen,(50,56,72),(200,y+3,170,16),border_radius=4)
            if value>0:
                pygame.draw.rect(screen,melange((240,90,60),(90,220,120),value),
                                 (200,y+3,int(170*value),16),border_radius=4)
        screen.blit(self.font.render("PHYSIQUE",True,(255,255,255)),(W-400,150))
        for k,line in enumerate(profile["notes"]):
            screen.blit(self.small.render("- "+line,True,(200,205,220)),(W-400,195+k*34))
        for k,col in enumerate(PALETTE):
            x=W//2-3*52+k*52+6
            pygame.draw.rect(screen,col,(x,H-120,40,40),border_radius=6)
            if k==color:
                pygame.draw.rect(screen,(255,255,255),(x-4,H-124,48,48),3,border_radius=8)
        hint="GAUCHE/DROITE : voiture    HAUT/BAS : couleur    ENTREE : jouer    ECHAP : quitter"
        text=self.small.render(hint,True,(200,205,220))
        screen.blit(text,(W//2-text.get_width()//2,H-50))
