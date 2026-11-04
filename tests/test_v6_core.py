"""Tests unitaires du vrai moteur V6 modulaire, sans fenêtre Pygame."""
import math
import unittest
from unittest.mock import patch

from karting.state import Controls, VehicleState
from karting.v6.config import V6Config
from karting.v6.physics import V6Physics
from karting.v6.session import V6Session
from karting.v6.track import V6Track, height
from karting.v6.vehicles import VOITURES


class V6CoreTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cfg=V6Config()
        cls.cfg.validate()
        cls.track=V6Track(cls.cfg)

    def make_car(self,v=250):
        x,y,a=self.track.start_position()
        return VehicleState(x,y,a,v=v)

    def test_v6_world_geometry(self):
        self.assertEqual(self.track.count,26*self.cfg.smoothing_steps)
        self.assertGreater(self.track.length,15000)
        self.assertEqual(len(self.track.lines),8)
        self.assertGreater(len(self.track.pit_set),20)
        self.assertTrue(self.track.garages)

    def test_relief_is_finite(self):
        for pt in ((0,0),(3200,1700),(8000,6700)):
            self.assertTrue(math.isfinite(height(*pt)))

    def test_four_vehicle_physics_are_stable(self):
        simulator=V6Physics(self.track)
        for profile in VOITURES:
            with self.subTest(vehicle=profile["nom"]):
                car=self.make_car()
                wheels=[(-22,-20),(-22,20),(22,-20),(22,20)]
                for _ in range(12):
                    motion=simulator.step(car,Controls(gas=True,steering=1),
                                          profile,wheels,0.0,0.016)
                car.validate()
                self.assertGreater(car.v,0)
                self.assertGreater(motion.grip_factor,0)

    def test_handbrake_slows_vehicle(self):
        simulator=V6Physics(self.track)
        profile=VOITURES[2]
        wheels=[(-22,-21),(-22,21),(22,-21),(22,21)]
        normal=self.make_car(300)
        drifting=self.make_car(300)
        simulator.step(normal,Controls(),profile,wheels,0,0.016)
        simulator.step(drifting,Controls(handbrake=True,steering=1),
                       profile,wheels,0,0.016)
        self.assertLess(drifting.v,normal.v)

    def test_rain_reduces_adhesion(self):
        simulator=V6Physics(self.track)
        profile=VOITURES[2]
        wheels=[(-22,-21),(-22,21),(22,-21),(22,21)]
        clear=simulator.step(self.make_car(),Controls(),profile,wheels,0,0.016)
        rain=simulator.step(self.make_car(),Controls(),profile,wheels,1,0.016)
        self.assertLess(rain.grip_factor,clear.grip_factor)

    def test_barrier_clamps_car_and_dissipates_impact(self):
        car=self.make_car(300)
        x,y=self.track.center[0]
        nx,ny=self.track.normals[0]
        car.x,car.y=x+nx*600,y+ny*600
        self.track.barrier(car,0.016)
        info=self.track.info(car.x,car.y)
        self.assertLessEqual(info.distance,self.track.pit_d2+25+5)

    def test_invalid_simulation_input(self):
        simulator=V6Physics(self.track)
        with self.assertRaises(ValueError):
            simulator.step(self.make_car(),Controls(),VOITURES[2],
                           [(-20,-20),(20,20)],float("nan"),0.016)
        with self.assertRaises(ValueError):
            simulator.step(self.make_car(),Controls(),VOITURES[2],
                           [(-20,-20),(20,20)],0,0)

    def test_session_runs_without_pygame(self):
        session=V6Session(self.cfg)
        old=session.car.x,session.car.y
        session.step(Controls(gas=True),0.016)
        self.assertFalse(session.paused)
        self.assertIsNotNone(session.motion)
        self.assertNotEqual((session.car.x,session.car.y),old)

    def test_4_wheels_penalty_is_only_applied_once(self):
        session=V6Session(self.cfg)
        with patch.object(session.track,"wheels_on_grass",return_value=4):
            session.step(Controls(),0.016)
            session.step(Controls(),0.016)
        self.assertAlmostEqual(session.race.state.elapsed,1.032,places=3)

    def test_switching_vehicle_retains_current_best(self):
        session=V6Session(self.cfg)
        session.race.state.best=75.0
        session.select(3)
        self.assertEqual(session.profile["nom"],"WEC")
        self.assertEqual(session.race.state.best,75.0)
        self.assertEqual(session.car.v,0.0)


if __name__=="__main__":
    unittest.main()
