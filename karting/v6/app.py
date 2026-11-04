"""Application V6 modulaire : seule cette couche connaît la boucle SDL/Pygame."""
import logging
import pygame

from ..input import read_controls
from ..state import Controls
from .atmosphere_render import V6AtmosphereRenderer
from .camera import V6Camera
from .compositor import V6Compositor
from .config import V6Config
from .decor_render import V6DecorRenderer
from .hud import V6HUD
from .menu import V6MenuRenderer
from .particles_render import V6ParticleRenderer
from .session import V6Session
from .sky import SkyRenderer
from .vehicle_render import V6VehicleRenderer
from .vehicles import PALETTE, VOITURES
from .world_render import V6WorldRenderer

logger=logging.getLogger("karting.v6")


class V6Game:
    def __init__(self,config=None):
        self.config=config or V6Config()
        self.config.validate()
        self.running=True
        self.mode="menu"
        self.menu_time=0.0
        self.session=None
        self.camera=None
        self.screen=None
        self.clock=None
        self.world=None
        self.vehicle=None
        self.decor=None
        self.compositor=None
        self.particles=None
        self.sky=None
        self.atmosphere=None
        self.hud=None
        self.menu=None

    def setup(self):
        """Appelée après pygame.init ; aucun composant logique ne lance de fenêtre."""
        self.screen=pygame.display.set_mode((self.config.width,self.config.height))
        pygame.display.set_caption("Mini Karting V6 - moteur modulaire")
        self.clock=pygame.time.Clock()
        self.session=V6Session(self.config)
        self.world=V6WorldRenderer()
        self.vehicle=V6VehicleRenderer(self.session.selected_index,
                                        PALETTE[self.session.selected_color])
        self.decor=V6DecorRenderer(self.world)
        self.particles=V6ParticleRenderer()
        self.compositor=V6Compositor(self.decor,self.vehicle,self.particles)
        self.sky=SkyRenderer(self.config.scenery_seed)
        self.atmosphere=V6AtmosphereRenderer(self.config,self.config.scenery_seed)
        self.hud=V6HUD(self.config,self.session.track)
        self.menu=V6MenuRenderer(self.config)
        self.camera=V6Camera(self.config,self.session.profile,self.session.car.angle)
        self.camera.follow(self.session.car,0)

    def choose(self,index,color=None):
        self.session.select(index,color)
        self.vehicle=V6VehicleRenderer(self.session.selected_index,
                                        PALETTE[self.session.selected_color])
        self.compositor.vehicle=self.vehicle
        self.camera.set_profile(self.session.profile)

    def handle(self,event):
        if event.type==pygame.QUIT:
            self.running=False
            return
        if event.type!=pygame.KEYDOWN:return
        key=event.key
        if key==pygame.K_ESCAPE:
            self.running=False
            return
        if self.mode=="menu":
            if key in (pygame.K_LEFT,pygame.K_q,pygame.K_a,
                       pygame.K_RIGHT,pygame.K_d):
                direction=-1 if key in (pygame.K_LEFT,pygame.K_q,pygame.K_a) else 1
                self.choose((self.session.selected_index+direction)%len(VOITURES))
            elif key in (pygame.K_UP,pygame.K_DOWN,pygame.K_c):
                direction=-1 if key==pygame.K_UP else 1
                self.choose(self.session.selected_index,
                            (self.session.selected_color+direction)%len(PALETTE))
            elif key in (pygame.K_RETURN,pygame.K_SPACE):
                self.mode="race"
                self.session.reset()
                self.camera=V6Camera(self.config,self.session.profile,self.session.car.angle)
                self.camera.follow(self.session.car,0)
        else:
            if key==pygame.K_r:
                self.session.reset()
                self.camera=V6Camera(self.config,self.session.profile,self.session.car.angle)
                self.camera.follow(self.session.car,0)
            elif key==pygame.K_m:
                self.mode="menu"
                self.session.reset()
        if key==pygame.K_n:
            self.session.weather["phase"]=(self.session.weather["phase"]+1)%3
        elif key==pygame.K_p:
            self.session.weather["pluie_on"]=not self.session.weather["pluie_on"]
        elif key==pygame.K_g:
            self.session.quality=(self.session.quality+1)%3

    def draw(self,dt):
        s=self.session
        screen=self.screen
        if self.mode=="menu":
            self.menu.draw(screen,self.vehicle,s.selected_index,s.selected_color,self.menu_time)
            pygame.display.flip()
            return
        faults=s.faults
        screen.fill((40,70,80))
        faults.optional("render.background",
                       lambda:self.world.background(screen,self.camera,self.sky,s.weather,
                                                    s.quality,s.elapsed))
        faults.optional("render.track",
                       lambda:self.world.track(screen,self.camera,s.track,s.weather,s.quality))
        faults.optional("render.marks",
                       lambda:self.particles.draw_marks(screen,self.camera,s.effects,s.weather))
        faults.optional("render.scene",lambda:self.compositor.draw(screen,self.camera,s))
        faults.optional("render.weather",
                       lambda:self.atmosphere.draw(screen,self.camera,s.car,self.vehicle.model,
                                                   s.scenery.lamps,s.weather,dt,s.quality))
        faults.optional("render.hud",lambda:self.hud.draw(screen,s,self.clock.get_fps()))
        if s.paused:
            font=pygame.font.SysFont("monospace",24,bold=True)
            msg=font.render("SIMULATION EN PAUSE - R pour recommencer",True,(255,70,70))
            screen.blit(msg,(20,screen.get_height()-185))
        if faults.faults:
            pygame.display.set_caption("Karting V6 - module en panne (karting.log)")
        pygame.display.flip()

    def run(self):
        pygame.init()
        try:
            self.setup()
            while self.running:
                dt=min(self.clock.tick(self.config.frame_rate)/1000,self.config.max_delta)
                for event in pygame.event.get():
                    self.handle(event)
                if not self.running:break
                if self.mode=="menu":
                    self.menu_time+=dt
                else:
                    controls=self.session.faults.optional(
                        "input",lambda:read_controls(pygame.key.get_pressed()),Controls())
                    self.session.step(controls,dt)
                    self.session.faults.optional("camera",
                        lambda:self.camera.follow(self.session.car,dt,
                            quality=self.session.quality,
                            rain=self.session.weather["pluie"],
                            intensity=self.session.motion.collision_impact if self.session.motion else 0))
                self.draw(dt)
        finally:
            pygame.quit()
