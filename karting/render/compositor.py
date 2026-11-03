"""Tri de profondeur des éléments visuels indépendants."""
import pygame
from ..camera import Camera
from ..effects import Effects
from ..faults import FaultManager
from ..scenery import Tree
from ..state import VehicleState
from ..track import Track
from .world import WorldRenderer
from .vehicle import VehicleRenderer
from .particles import EffectsRenderer


class SceneCompositor:
    def __init__(self, world: WorldRenderer, vehicle: VehicleRenderer, effects_renderer: EffectsRenderer):
        self.world = world
        self.vehicle = vehicle
        self.effects_renderer = effects_renderer

    def draw(self, screen: pygame.Surface, camera: Camera, car: VehicleState,
             track: Track, trees: list[Tree], effects: Effects, faults: FaultManager) -> None:
        cfg = camera.cfg
        # La voiture, les décors et les particules partagent un tri de profondeur.
        objects: list[tuple[float,str,object]] = [(camera.position(car.x,car.y)[0],"vehicle",None)]
        # Chaque effet est présenté à son superviseur, même après une panne :
        # cela autorise de vrais réessais lorsque l'objet redevient visible.
        for tree in trees:
            forward,right = camera.position(tree.x,tree.y)
            if cfg.near < forward < cfg.far:
                objects.append((forward,"tree",(right,tree)))
        for garage in track.garages:
            forward = camera.position(*track.center[garage.track_index])[0]+200
            if cfg.near < forward < cfg.far+200:
                objects.append((forward,"garage",garage))
        for particle in effects.particles:
            forward = camera.position(particle.x,particle.y)[0]
            if forward > cfg.near:
                objects.append((forward,"particle",particle))
        objects.append((camera.position(*track.center[0])[0],"gantry",None))
        objects.sort(key=lambda obj:obj[0],reverse=True)
        for forward,kind,object_data in objects:
            if kind=="vehicle":
                faults.optional("render.vehicle", lambda: self.vehicle.draw(screen,camera,car))
            elif kind=="tree":
                right,tree=object_data
                faults.optional("render.trees",lambda: self.world.draw_tree(screen,camera,forward,right,tree))
            elif kind=="garage":
                faults.optional("render.garages",lambda: self.world.draw_garage(screen,camera,object_data))
            elif kind=="particle":
                faults.optional("render.particles",lambda: self.effects_renderer.draw_particle(screen,camera,forward,object_data))
            elif kind=="gantry":
                faults.optional("render.gantry",lambda: self.world.draw_gantry(screen,camera,track))
