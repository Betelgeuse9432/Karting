"""Interface V6 : compteurs, mini-carte, temps et panneaux des stands."""
import math
import pygame
from .mesh import melange


def display_time(seconds):
    return "--" if seconds is None else f"{seconds:.2f}s"


class V6HUD:
    def __init__(self,config,track):
        self.config=config
        self.font=pygame.font.SysFont("monospace",22,bold=True)
        self.small=pygame.font.SysFont("monospace",16,bold=True)
        self.large=pygame.font.SysFont("monospace",34,bold=True)
        self.huge=pygame.font.SysFont("monospace",46,bold=True)
        w,h=config.world_size
        self.scale=250/w
        self.mini=pygame.Surface((int(w*self.scale)+20,int(h*self.scale)+20),pygame.SRCALPHA)
        pygame.draw.rect(self.mini,(8,10,18,170),self.mini.get_rect(),border_radius=10)
        points=[(x*self.scale+10,y*self.scale+10) for x,y in track.center]
        pygame.draw.lines(self.mini,(40,44,56),True,points,8)
        pygame.draw.lines(self.mini,(225,228,235),True,points,4)
        pygame.draw.circle(self.mini,(255,210,0),tuple(map(int,points[0])),5)
        self.info_bg=pygame.Surface((345,108),pygame.SRCALPHA)
        pygame.draw.rect(self.info_bg,(8,10,18,160),self.info_bg.get_rect(),border_radius=12)

    def speedometer(self,screen,car,profile):
        ecran=screen
        W,H=self.config.width,self.config.height
        enorme,petite,police=self.huge,self.small,self.font
        VITESSE_MAX=profile["vmax"]
        v=car.v
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

    def pits(self,screen,race):
        W,H=self.config.width,self.config.height
        p=pygame.Surface((580,400),pygame.SRCALPHA)
        p.fill((10,10,25,220))
        pygame.draw.rect(p,(255,210,0),(0,0,580,400),4)
        screen.blit(p,(W//2-290,H//2-200))
        state=race.state
        rows=[("STANDS - TABLEAU DES SCORES",(255,210,0),self.large),
              (f"Tour en cours : {state.laps+1}",(255,255,255),self.font),
              (f"Dernier tour  : {display_time(state.last)}",(255,255,255),self.font),
              (f"Meilleur tour : {display_time(state.best)}",(255,220,0),self.font),
              (f"Vitesse max   : {int(state.max_speed*0.3)} km/h",(255,255,255),self.font),
              (f"Tours termines : {state.laps}",(255,255,255),self.font)]
        y=H//2-180
        for line,c,f in rows:
            screen.blit(f.render(line,True,c),(W//2-270,y))
            y+=48 if f is self.large else 34

    def draw(self,screen,session,fps):
        c=self.config
        car=session.car
        race=session.race
        telemetry=session.motion
        weather=session.weather
        profile=session.profile
        screen.blit(self.info_bg,(10,10))
        white=(255,255,255)
        screen.blit(self.font.render(f"Tour {race.state.laps+1}   {race.state.elapsed:.1f}s",True,white),(24,20))
        screen.blit(self.font.render(f"Meilleur : {display_time(race.state.best)}",True,(255,220,0)),(24,48))
        screen.blit(self.small.render(f"{profile['nom']}   {int(fps)} FPS   Qualite {['ECO','ELEVEE','ULTRA'][session.quality]} (G)",True,(180,188,210)),(24,80))
        for k,(name,val,col) in enumerate([("GAZ",car.gas,(60,220,60)),
                                          ("FREIN",car.brake,(240,60,60)),
                                          ("DISQ",car.heat,(255,150,30))]):
            x=20+k*58
            pygame.draw.rect(screen,(8,10,18),(x,c.height-130,40,100),border_radius=6)
            pygame.draw.rect(screen,col,(x,c.height-30-100*val,40,100*val),border_radius=6)
            pygame.draw.rect(screen,(200,205,220),(x,c.height-130,40,100),2,border_radius=6)
            screen.blit(self.small.render(name,True,white),(x-4,c.height-26))
        grip=telemetry.grip_factor if telemetry else 1
        screen.blit(self.small.render(f"GRIP : {int(grip*100)}%",True,(160,220,255)),(c.width//2-110,c.height-28))
        if telemetry and telemetry.grass_wheels:
            msg=("HORS PISTE !" if telemetry.grass_wheels==4
                 else f"{telemetry.grass_wheels} roue(s) dans l'herbe")
            screen.blit(self.font.render(msg,True,(255,120,70)),(c.width//2-150,20))
        if session.penalty_alert>0:
            screen.blit(self.large.render("LIMITES DE PISTE : +1s",True,(255,80,80)),(c.width//2-210,150))
        if telemetry and telemetry.telemetry.front_slip:
            screen.blit(self.font.render("AVANT QUI GLISSE !",True,(255,160,40)),(c.width//2-170,50))
        if telemetry and telemetry.telemetry.rear_slip:
            screen.blit(self.font.render("ARRIERE QUI PATINE !",True,(255,160,40)),(c.width//2-170,80))
        if telemetry and telemetry.telemetry.drifting:
            screen.blit(self.font.render("DERAPAGE !",True,(255,210,30)),(c.width//2-70,110))
        mx,my=c.width-self.mini.get_width()-10,10
        screen.blit(self.mini,(mx,my))
        px,my_car=mx+10+car.x*self.scale,my+10+car.y*self.scale
        pygame.draw.circle(screen,(255,60,60),(px,my_car),6)
        pygame.draw.line(screen,white,(px,my_car),
                         (px+13*math.cos(car.angle),my_car+13*math.sin(car.angle)),3)
        sky_label=["JOUR","CREPUSCULE","NUIT"][weather["phase"]]
        if weather["pluie_on"]:sky_label+=" + PLUIE"
        screen.blit(self.small.render(sky_label,True,white),(mx,my+self.mini.get_height()+6))
        self.speedometer(screen,car,profile)
        screen.blit(self.small.render("ESPACE = frein a main  N = ciel  P = pluie  G = qualite  M = menu",True,(200,200,200)),(c.width//2-255,c.height-52))
        if telemetry and telemetry.telemetry.in_pits:self.pits(screen,race)
