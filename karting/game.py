"""Composition des services, boucle Pygame et isolation des défaillances."""
import logging
from dataclasses import replace
from copy import deepcopy
import pygame

from .camera import Camera
from .config import GameConfig
from .effects import Effects
from .faults import FaultManager
from .fault_injection import FaultInjector
from .input import read_controls
from .persistence import RecordStore
from .physics import VehiclePhysics
from .race import RaceTracker
from .scenery import generate_trees
from .state import Controls, Telemetry, VehicleState
from .track import Track
from .render.compositor import SceneCompositor
from .render.hud import HudRenderer
from .render.particles import EffectsRenderer
from .render.vehicle import VehicleRenderer
from .render.world import WorldRenderer

logger=logging.getLogger("karting")


class Game:
    def __init__(self, config: GameConfig | None = None, injector: FaultInjector | None = None):
        self.config=config or GameConfig()
        self.config.validate()
        self.injector=injector
        self.faults=FaultManager(injector.before_call if injector else None)
        self.track=Track(self.config)
        self.physics=VehiclePhysics(self.config)
        self.storage=RecordStore(self.config.record_path)
        best=self.faults.optional("record.load",self.storage.load)
        self.race=RaceTracker(self.track,best)
        self.car=self.spawn_car()
        self.camera=Camera(self.config,self.car.angle)
        self.camera.follow(self.car,0)
        self.effects=Effects(self.config.scenery_seed)
        self.trees=self.faults.optional("scenery.trees",
                                        lambda:generate_trees(self.track,self.config.scenery_seed),[])
        self.telemetry: Telemetry | None = None
        self.running=True
        self.paused=False
        self.screen: pygame.Surface | None = None
        self.world=WorldRenderer()
        self.vehicle_renderer=VehicleRenderer(self.config)
        self.particle_renderer: EffectsRenderer | None = None
        self.hud: HudRenderer | None = None
        self.compositor: SceneCompositor | None = None

    def spawn_car(self) -> VehicleState:
        x,y,angle=self.track.start_position()
        return VehicleState(x,y,angle)

    def reset(self) -> None:
        self.car=self.spawn_car()
        self.race.reset()  # meilleur temps conservé en mémoire
        self.effects.clear()
        self.camera=Camera(self.config,self.car.angle)
        self.camera.follow(self.car,0)
        self.telemetry=None
        self.paused=False
        self.faults.recover()  # réessai contrôlé de composants désactivés
        if self.injector is not None:
            self.injector.reset()
        # Rechargement des décors si leur création initiale avait échoué.
        if not self.trees:
            self.trees=self.faults.optional("scenery.trees",
                                            lambda:generate_trees(self.track,self.config.scenery_seed),[])

    def step(self, controls: Controls, dt: float) -> None:
        """Le moteur logique peut être exécuté sans fenêtre (sauf import du module game)."""
        if self.paused:
            return
        before=replace(self.car)
        try:
            if self.injector is not None:
                self.injector.before_call("physics")
            self.telemetry=self.physics.step(self.car,controls,self.track,dt)
        except Exception as exc:
            self.car=before  # invalide après une mise à jour partiellement effectuée
            self.paused=True
            self.faults.record("physics",exc,essential=True)
            return
        previous_best=self.race.state.best
        race_backup=deepcopy(self.race.state)
        try:
            if self.injector is not None:
                self.injector.before_call("race")
            finished=self.race.step(self.car,dt)
        except Exception as exc:
            self.race.state=race_backup
            self.paused=True
            self.faults.record("race",exc,essential=True)
            return
        if finished is not None and (previous_best is None or finished < previous_best):
            self.faults.optional("record.save",lambda:self.storage.save(finished))
        if self.telemetry:
            self.faults.optional("effects.sim",lambda:self.effects.step(self.car,self.telemetry,dt))

    def draw(self) -> None:
        assert self.screen is not None and self.compositor is not None and self.hud is not None
        screen=self.screen
        screen.fill((40,70,80))
        self.faults.optional("render.background",lambda:self.world.draw_background(screen,self.camera))
        self.faults.optional("render.track",lambda:self.world.draw_track(screen,self.camera,self.track))
        self.faults.optional("render.marks",lambda:self.particle_renderer.draw_marks(screen,self.camera,self.effects))
        self.faults.optional("render.scene",lambda:self.compositor.draw(screen,self.camera,self.car,
                                                                         self.track,self.trees,self.effects,self.faults))
        self.faults.optional("render.hud",lambda:self.hud.draw(screen,self.car,self.race,
                                                                self.telemetry,self.faults,self.paused))
        if "render.hud" in self.faults.disabled:
            # Minimum vital, même si l'affichage complet des alertes a planté.
            pygame.display.set_caption("Karting - ERREUR HUD (details : karting.log)")
        if self.injector is not None:
            self.draw_injection_status(screen)
        pygame.display.flip()

    def draw_injection_status(self, screen: pygame.Surface) -> None:
        """Bandeau minimal, indépendant du HUD pouvant lui-même être saboté."""
        injector=self.injector
        if injector is None:
            return
        try:
            font=pygame.font.SysFont("monospace", 15, bold=True)
            status="ACTIF" if injector.enabled else "OFF (F9)"
            line=(f"TEST PANNES {status} | seed {injector.seed} | "
                  f"{injector.count} injectee(s) | F10 force | R relance")
            area=pygame.Surface((min(self.config.width-20, len(line)*10+16),26), pygame.SRCALPHA)
            area.fill((10,10,20,220))
            screen.blit(area,(10,108))
            screen.blit(font.render(line,True,(255,210,80)),(15,112))
        except Exception:
            # Ce bandeau ne doit en aucun cas être critique pour le jeu.
            logger.exception("Bandeau de test indisponible")

    def run(self) -> None:
        pygame.init()
        try:
            self.screen=pygame.display.set_mode((self.config.width,self.config.height))
            pygame.display.set_caption("Karting - refonte modulaire")
            self.particle_renderer=EffectsRenderer()
            self.hud=HudRenderer(self.config,self.track)
            self.compositor=SceneCompositor(self.world,self.vehicle_renderer,self.particle_renderer)
            clock=pygame.time.Clock()
            while self.running:
                dt=min(clock.tick(self.config.frame_rate)/1000,self.config.max_delta)
                for event in pygame.event.get():
                    if event.type==pygame.QUIT:
                        self.running=False
                    elif event.type==pygame.KEYDOWN:
                        if event.key==pygame.K_ESCAPE:
                            self.running=False
                        elif event.key==pygame.K_r:
                            self.reset()
                        elif event.key==pygame.K_F9 and self.injector is not None:
                            self.injector.toggle()
                        elif event.key==pygame.K_F10 and self.injector is not None:
                            component=self.injector.force_random(self.faults.disabled)
                            logger.warning("F10 : panne immédiatement armée sur %s",component)
                if not self.running:
                    break
                if self.injector is not None and not self.paused:
                    self.injector.advance(dt,self.faults.disabled)
                controls=self.faults.optional("input",lambda:read_controls(pygame.key.get_pressed()),Controls())
                self.step(controls,dt)
                self.faults.optional("camera",lambda:self.camera.follow(self.car,dt))
                self.draw()
        finally:
            pygame.quit()
