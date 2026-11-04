"""Dessiner des traces et particules V6 sur terrain en relief, sans toucher à leur état."""
import pygame
from ..render.particles import EffectsRenderer
from .track import height
from .mesh import melange


class V6ParticleRenderer(EffectsRenderer):
    def draw_marks(self,screen,camera,effects,weather):
        near=camera.cfg.near
        fog=weather.get("fog_color",(170,215,255))
        for x1,y1,x2,y2 in effects.marks:
            f1,r1=camera.position(x1,y1)
            f2,r2=camera.position(x2,y2)
            if not (near<f1<camera.far and f2>near):continue
            scale=camera.focal
            z1,z2=height(x1,y1),height(x2,y2)
            tint=melange((24,24,26),fog,min(1,f1/camera.far)**1.6*0.92)
            pygame.draw.line(screen,tint,
                             (camera.cfg.width/2+r1/f1*scale,
                              camera.cfg.horizon+(camera.z-z1)/f1*scale),
                             (camera.cfg.width/2+r2/f2*scale,
                              camera.cfg.horizon+(camera.z-z2)/f2*scale),
                             max(1,int(6*scale/f1)))

    def draw_particle(self,screen,camera,forward,p,weather):
        if forward<=camera.cfg.near:return
        s=camera.focal/forward
        x=camera.cfg.width/2+camera.position(p.x,p.y)[1]*s
        y=camera.cfg.horizon+(camera.z-p.z)*s
        age=max(0,min(1,p.lifetime/p.initial_lifetime))
        fog=weather.get("fog_color",(170,215,255))
        haze=min(1,forward/camera.far)**1.6*0.92
        if p.kind=="etincelle":
            color=melange((255,int(120+135*age),int(40*age)),fog,haze)
            pygame.draw.circle(screen,color,(x,y),max(1,2.5*s))
        elif p.kind=="flamme":
            color=melange((255,int(70+170*age),int(20+100*age*age)),fog,haze)
            pygame.draw.circle(screen,color,(x,y),max(1,p.size*age*s))
        else:
            diameter=int(max(2,min(300,2*p.size*s)))
            img=pygame.transform.scale(self.circle,(diameter,diameter))
            img.set_alpha(int(110*age))
            screen.blit(img,(x-diameter/2,y-diameter/2))
