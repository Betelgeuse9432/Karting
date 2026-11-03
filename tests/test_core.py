"""Tests exécutables sans Pygame ni ouverture de fenêtre."""
import math
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from karting.camera import Camera
from karting.config import GameConfig
from karting.effects import Effects
from karting.faults import FaultManager
from karting.persistence import RecordStore
from karting.physics import VehiclePhysics
from karting.race import RaceTracker
from karting.scenery import generate_trees
from karting.state import Controls, VehicleState
from karting.track import Track


class ConfigTests(unittest.TestCase):
    def test_invalid_dimensions(self):
        with self.assertRaises(ValueError):
            GameConfig(width=0).validate()
        with self.assertRaises(ValueError):
            GameConfig(wheelbase=0).validate()
        with self.assertRaises(ValueError):
            GameConfig(near=1500,far=100).validate()


class TrackTests(unittest.TestCase):
    def setUp(self):
        self.cfg=GameConfig()
        self.track=Track(self.cfg)

    def test_geometry(self):
        self.assertEqual(self.track.count,18*45)
        self.assertEqual(len(self.track.lines),6)
        self.assertTrue(all(len(line)==self.track.count for line in self.track.lines))
        self.assertEqual(len(self.track.pit_indices),95)
        self.assertEqual(len(self.track.garages),6)
        self.assertEqual(len(self.track.normals),self.track.count)

    def test_center_is_on_road(self):
        for i in range(0,self.track.count,30):
            point=self.track.center[i]
            info=self.track.info(*point)
            self.assertFalse(info.on_grass)
            self.assertLess(info.distance,1e-6)

    def test_stands_and_grass(self):
        i=self.track.pit_indices[20]
        in_pit=self.track.info(*self.track.position(i,self.track.pit_d1+10))
        self.assertTrue(in_pit.in_pits)
        self.assertFalse(in_pit.on_grass)
        away=self.track.info(*self.track.position(300,300))
        self.assertTrue(away.on_grass)

    def test_scenery_is_deterministic(self):
        first=generate_trees(self.track,7)
        self.assertTrue(first)
        self.assertEqual(first,generate_trees(self.track,7))
        self.assertNotEqual(first,generate_trees(self.track,9))


class PhysicsTests(unittest.TestCase):
    def setUp(self):
        self.cfg=GameConfig()
        self.track=Track(self.cfg)
        self.physics=VehiclePhysics(self.cfg)
        x,y,a=self.track.start_position()
        self.car=VehicleState(x,y,a)

    def test_acceleration(self):
        t=self.physics.step(self.car,Controls(gas=True),self.track,0.016)
        self.assertGreater(self.car.v,0)
        self.assertGreater(self.car.gas,0)
        self.assertGreater(t.downforce_factor,0)

    def test_braking_and_reverse(self):
        self.car.v=100
        self.physics.step(self.car,Controls(brake=True),self.track,0.03)
        self.assertLess(self.car.v,100)
        self.assertGreater(self.car.brake,0)
        self.car.v=0
        self.car.brake=0
        self.physics.step(self.car,Controls(brake=True),self.track,0.03)
        self.assertLess(self.car.v,0)

    def test_speed_is_bounded(self):
        for _ in range(200):
            self.physics.step(self.car,Controls(gas=True),self.track,0.05)
        self.assertLessEqual(self.car.v,self.cfg.max_speed)
        self.assertTrue(math.isfinite(self.car.angle))

    def test_bad_timestep_and_state(self):
        with self.assertRaises(ValueError):
            self.physics.step(self.car,Controls(),self.track,0)
        with self.assertRaises(ValueError):
            self.physics.step(self.car,Controls(),self.track,1)
        self.car.v=float('nan')
        with self.assertRaises(FloatingPointError):
            self.physics.step(self.car,Controls(),self.track,0.01)


class RaceTests(unittest.TestCase):
    def test_four_sectors_complete_lap(self):
        track=Track(GameConfig())
        racer=RaceTracker(track)
        car=VehicleState(*track.start_position())
        for sector in (1,2,3,0):
            idx=(sector*track.count//4)+2
            car.x,car.y=track.center[idx]
            result=racer.step(car,0.5)
        self.assertEqual(result,2.0)
        self.assertEqual(racer.state.laps,1)
        self.assertEqual(racer.state.best,2.0)
        self.assertEqual(racer.state.next_sector,1)
        racer.reset()
        self.assertEqual(racer.state.laps,0)
        self.assertEqual(racer.state.best,2.0)


class PersistenceTests(unittest.TestCase):
    def test_missing_save_read_and_corrupt(self):
        with TemporaryDirectory() as directory:
            file=Path(directory)/'record.txt'
            store=RecordStore(file)
            self.assertIsNone(store.load())
            store.save(12.34)
            self.assertAlmostEqual(store.load(),12.34)
            file.write_text('not-a-number')
            with self.assertRaises(ValueError):
                store.load()
            file.write_text('nan')
            with self.assertRaises(ValueError):
                store.load()
            with self.assertRaises(ValueError):
                store.save(-1)


class FaultTests(unittest.TestCase):
    def test_optional_component_is_disabled_then_recovered(self):
        manager=FaultManager()
        times=[]
        def broken():
            times.append(1)
            raise RuntimeError('panne injectée')
        with self.assertLogs('karting',level='ERROR'):
            self.assertEqual(manager.optional('smoke',broken,'fallback'),'fallback')
        self.assertEqual(manager.optional('smoke',broken,'fallback'),'fallback')
        self.assertEqual(len(times),1)
        self.assertIn('smoke',manager.disabled)
        manager.recover()
        self.assertNotIn('smoke',manager.disabled)
        self.assertEqual(manager.optional('smoke',lambda:'ok'),'ok')


class CameraTests(unittest.TestCase):
    def test_projection_and_clipping(self):
        cfg=GameConfig()
        camera=Camera(cfg)
        self.assertIsNone(camera.project([(-10,0,0),(-10,1,0),(-10,-1,0)]))
        projected=camera.project([(100,-10,0),(100,10,0),(100,0,10)])
        self.assertEqual(len(projected),3)
        self.assertTrue(all(math.isfinite(x) and math.isfinite(y) for x,y in projected))


class EffectsTests(unittest.TestCase):
    def test_marks_and_particle_lifecycle(self):
        effects=Effects(7)
        car=VehicleState(100,100,0,v=300)
        effects.emit(car,'fumee',0,0,2,0,0,10,0.1,8)
        self.assertEqual(len(effects.particles),1)
        effects.update_particles(0.2)
        self.assertFalse(effects.particles)
        effects.clear()
        self.assertEqual(len(effects.previous_wheels),4)


if __name__=='__main__':
    unittest.main()
