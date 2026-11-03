"""Interface de course et tableau des stands, sans écriture dans l'état du jeu."""
import pygame
from ..config import GameConfig
from ..faults import FaultManager
from ..race import RaceTracker
from ..state import Telemetry, VehicleState
from ..track import Track


def format_time(seconds: float | None) -> str:
    return "--" if seconds is None else f"{seconds:.2f}s"


class HudRenderer:
    def __init__(self, config: GameConfig, track: Track):
        self.cfg = config
        self.font = pygame.font.SysFont("monospace", 22, bold=True)
        self.small = pygame.font.SysFont("monospace", 16, bold=True)
        self.large = pygame.font.SysFont("monospace", 34, bold=True)
        self.alert = pygame.font.SysFont("monospace", 15, bold=True)
        self.scale = 0.05
        w,h = config.world_size
        self.minimap = pygame.Surface((int(w*self.scale)+10,int(h*self.scale)+10),pygame.SRCALPHA)
        self.minimap.fill((0,0,0,120))
        pygame.draw.lines(self.minimap,(255,255,255),True,
                          [(x*self.scale+5,y*self.scale+5) for x,y in track.center],4)

    def draw(self, screen: pygame.Surface, car: VehicleState, race: RaceTracker,
             telemetry: Telemetry | None, faults: FaultManager, paused: bool) -> None:
        cfg = self.cfg
        state = race.state
        white=(255,255,255)
        screen.blit(self.font.render(f"{int(abs(car.v)*0.3)} km/h",True,white),(20,20))
        screen.blit(self.font.render(f"Tour {state.laps+1}  {state.elapsed:.1f}s",True,white),(20,48))
        screen.blit(self.font.render(f"Meilleur : {format_time(state.best)}",True,(255,220,0)),(20,76))
        levels=(("GAZ",car.gas,(60,220,60)),("FREIN",car.brake,(240,60,60)),
                ("DISQ",car.heat,(255,150,30)))
        for k,(name,value,color) in enumerate(levels):
            x = 20+k*58
            pygame.draw.rect(screen,(0,0,0),(x,cfg.height-130,40,100))
            pygame.draw.rect(screen,color,(x,cfg.height-30-100*value,40,100*value))
            pygame.draw.rect(screen,white,(x,cfg.height-130,40,100),2)
            screen.blit(self.small.render(name,True,white),(x-2,cfg.height-28))
        downforce = telemetry.downforce_factor if telemetry else 1
        screen.blit(self.small.render(f"APPUI AERO : +{int((downforce-1)*100)}%",True,(160,220,255)),
                    (cfg.width//2-80,cfg.height-28))
        if telemetry:
            if telemetry.on_grass:
                screen.blit(self.font.render("HORS PISTE !",True,(255,80,80)),(cfg.width//2-80,20))
            if telemetry.front_slip:
                screen.blit(self.font.render("AVANT QUI GLISSE : trop de frein en virage !",True,(255,160,40)),
                            (cfg.width//2-230,50))
            if telemetry.rear_slip:
                screen.blit(self.font.render("ARRIERE QUI PATINE : trop de gaz !",True,(255,160,40)),
                            (cfg.width//2-190,80))
        screen.blit(self.minimap,(cfg.width-self.minimap.get_width()-10,10))
        pygame.draw.circle(screen,(255,60,60),
                           (cfg.width-self.minimap.get_width()-10+car.x*self.scale+5,
                            10+car.y*self.scale+5),5)
        if telemetry and telemetry.in_pits:
            self.draw_pits(screen,state)
        self.draw_faults(screen,faults,paused)

    def draw_pits(self, screen: pygame.Surface, state) -> None:
        cfg = self.cfg
        panel=pygame.Surface((580,400),pygame.SRCALPHA)
        panel.fill((10,10,25,220))
        pygame.draw.rect(panel,(255,210,0),(0,0,580,400),4)
        screen.blit(panel,(cfg.width//2-290,cfg.height//2-200))
        lines=(("STANDS - TABLEAU DES SCORES",(255,210,0),self.large),
               (f"Tour en cours   : {state.laps+1}",(255,255,255),self.font),
               (f"Dernier tour    : {format_time(state.last)}",(255,255,255),self.font),
               (f"Meilleur tour   : {format_time(state.best)}",(255,220,0),self.font),
               (f"Vitesse max     : {int(state.max_speed*0.3)} km/h",(255,255,255),self.font),
               (f"Tours termines  : {state.laps}",(255,255,255),self.font))
        y=cfg.height//2-180
        for line,color,font in lines:
            screen.blit(font.render(line,True,color),(cfg.width//2-270,y))
            y+=48 if font is self.large else 34
        recent="   ".join(f"T{state.laps-len(state.recent)+k+1}: {t:.1f}s"
                            for k,t in enumerate(state.recent[-4:]))
        screen.blit(self.small.render(recent or "Termine un tour pour voir tes temps !",True,(180,200,255)),
                    (cfg.width//2-270,y+6))
        screen.blit(self.small.render("Disques et pneus refroidis !",True,(120,255,160)),
                    (cfg.width//2-270,y+34))

    def draw_faults(self, screen: pygame.Surface, faults: FaultManager, paused: bool) -> None:
        if not faults.faults and not paused:
            return
        if paused:
            self.draw_warning(screen,"SIMULATION EN PAUSE - REPARATION AUTOMATIQUE",(255,80,80),0)
        waiting = sorted(faults.pending.items(), key=lambda pair: pair[1].next_retry)
        for idx, (component, status) in enumerate(waiting[:3]):
            remaining = max(0.0, status.next_retry - faults.time)
            msg = f"{component} : tentative {status.attempts+1} dans {remaining:.1f}s"
            self.draw_warning(screen, msg, (255, 210, 80), idx+1)
        if not waiting and faults.recoveries:
            self.draw_warning(screen, faults.recoveries[-1], (110, 255, 170), 1)

    def draw_warning(self, screen: pygame.Surface, text: str, color, line: int) -> None:
        y=self.cfg.height-178-line*21
        overlay=pygame.Surface((self.cfg.width-20,21),pygame.SRCALPHA)
        overlay.fill((0,0,0,180))
        screen.blit(overlay,(10,y))
        screen.blit(self.alert.render(text,True,color),(15,y+2))
