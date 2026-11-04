"""Orchestration de la course V6 : aucun affichage, aucune boucle Pygame."""
from copy import deepcopy
from dataclasses import replace

from .effects import V6Effects
from ..faults import FaultManager
from ..persistence import RecordStore
from ..race import RaceTracker
from ..state import Controls, VehicleState
from .config import V6Config
from .physics import V6Physics
from .scenery import V6Scenery
from .track import V6Track
from .vehicles import PALETTE, VOITURES
from .weather import step_weather


class V6Session:
    def __init__(self, config=None, selection=2):
        self.config=config or V6Config()
        self.config.validate()
        self.faults=FaultManager()
        self.track=V6Track(self.config)
        self.physics=V6Physics(self.track)
        self.effects=V6Effects(self.config.scenery_seed)
        self.scenery=V6Scenery(self.track,self.config.scenery_seed)
        self.storage=RecordStore(self.config.record_path)
        best=self.faults.optional("record.load",self.storage.load)
        self.race=RaceTracker(self.track,best)
        self.selected_index=selection
        self.selected_color=VOITURES[selection]["defaut"]
        self.weather={"phase":0,"nuit":0.0,"pluie_on":False,"pluie":0.0}
        self.quality=1
        self.elapsed=0.0
        self.penalty_alert=0.0
        self.outside_four=False
        self.motion=None
        self.paused=False
        self.car=self.spawn_car()

    @property
    def profile(self):
        return VOITURES[self.selected_index]

    def spawn_car(self):
        x,y,heading=self.track.start_position()
        return VehicleState(x,y,heading)

    def reset(self):
        self.car=self.spawn_car()
        self.race.reset()
        self.effects.clear()
        self.motion=None
        self.penalty_alert=0.0
        self.outside_four=False
        self.paused=False
        self.faults.recover()

    def select(self,index,color=None):
        if not 0<=index<len(VOITURES):
            raise ValueError("Sélection de véhicule invalide")
        if color is None:color=VOITURES[index]["defaut"]
        if not 0<=color<len(PALETTE):
            raise ValueError("Couleur invalide")
        self.selected_index=index
        self.selected_color=color
        self.reset()

    def step(self,controls:Controls,dt:float):
        if self.paused:return
        self.elapsed+=dt
        step_weather(self.weather,dt)
        wheels=[(w[0],w[1]) for w in self.wheels]
        before=replace(self.car)
        try:
            self.motion=self.physics.step(self.car,controls,self.profile,wheels,
                                          self.weather["pluie"],dt)
        except Exception as exc:
            self.car=before
            self.paused=True
            self.faults.record("physics",exc,essential=True)
            return
        # La pénalité intervient une seule fois lors du passage à quatre roues dehors.
        if self.motion.grass_wheels==4 and not self.outside_four:
            self.race.state.elapsed+=1.0
            self.penalty_alert=2.0
        self.outside_four=(self.motion.grass_wheels==4)
        self.penalty_alert=max(0.0,self.penalty_alert-dt)
        previous_best=self.race.state.best
        backup=deepcopy(self.race.state)
        try:
            finished=self.race.step(self.car,dt)
        except Exception as exc:
            self.race.state=backup
            self.paused=True
            self.faults.record("race",exc,essential=True)
            return
        if finished is not None and (previous_best is None or finished<previous_best):
            self.faults.optional("record.save",lambda:self.storage.save(finished))
        self.faults.optional("effects.sim",lambda:self.effects.step(self.car,self.motion.telemetry,dt))

    @property
    def wheels(self):
        # Remplies par la vue 3D construite au changement de véhicule ;
        # propriété par défaut indépendante du moteur graphique.
        defaults={
            0:[(-15,-18),(-15,18),(17,-16),(17,16)],
            1:[(-30,-26),(-30,26),(30,-26),(30,26)],
            2:[(-22,-21),(-22,21),(22,-21),(22,21)],
            3:[(-30,-22),(-30,22),(30,-22),(30,22)],
        }
        return [(x,y,i>=2,10,6) for i,(x,y) in enumerate(defaults[self.selected_index])]
