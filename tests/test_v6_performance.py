"""Régressions performance V6 : vitesse de simulation et géométrie exacte."""
import math
import random
import unittest

from karting.state import Controls, VehicleState
from karting.v6.config import V6Config
from karting.v6.physics import V6Physics
from karting.v6.timing import FixedStepClock
from karting.v6.track import V6Track, height
from karting.v6.vehicles import VOITURES


class FixedClockTests(unittest.TestCase):
    def test_10_fps_does_not_slow_the_simulated_second(self):
        clock=FixedStepClock()
        steps=sum(clock.consume(0.1) for _ in range(10))
        self.assertEqual(steps,60)
        self.assertAlmostEqual(steps*clock.dt,1.0,places=8)

    def test_high_fps_does_not_speed_up_simulation(self):
        clock=FixedStepClock()
        self.assertEqual(sum(clock.consume(1/120) for _ in range(120)),60)

    def test_one_second_of_very_slow_render_is_bounded(self):
        clock=FixedStepClock()
        self.assertEqual(clock.consume(1.0),8)
        self.assertLessEqual(clock.accumulator,clock.dt)

    def test_clock_rejects_invalid_elapsed_time(self):
        clock=FixedStepClock()
        for bad in (-1.0,float("nan"),float("inf")):
            with self.subTest(bad=bad),self.assertRaises(ValueError):
                clock.consume(bad)


class TrackPerformanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.track=V6Track(V6Config())

    def test_spatial_search_matches_full_scan(self):
        track=self.track
        rng=random.Random(3011)
        samples=list(track.center[::61])
        samples.extend((rng.uniform(-800,10800),rng.uniform(-800,9200))
                       for _ in range(40))
        for x,y in samples:
            with self.subTest(x=round(x),y=round(y)):
                actual,distance=track.nearest(x,y)
                expected=min(range(track.count),
                             key=lambda i:(track.center[i][0]-x)**2
                                          +(track.center[i][1]-y)**2)
                self.assertEqual(actual,expected)
                self.assertAlmostEqual(distance,math.dist((x,y),track.center[actual]))

    def test_cached_terrain_heights_match_source_function(self):
        track=self.track
        for k in range(len(track.lines)):
            for i in range(0,track.count,99):
                with self.subTest(k=k,index=i):
                    x,y=track.lines[k][i]
                    self.assertAlmostEqual(track.line_heights[k][i],height(x,y),places=12)

    def test_nearest_index_can_be_reused_for_wheels(self):
        track=self.track
        x,y,angle=track.start_position()
        car=VehicleState(x,y,angle)
        wheels=[(-22,-21),(-22,21),(22,-21),(22,21)]
        idx,_=track.nearest(x,y)
        self.assertEqual(track.wheels_on_grass(car,wheels),
                         track.wheels_on_grass(car,wheels,idx))


class VehicleSpeedTests(unittest.TestCase):
    def test_all_four_vehicles_have_higher_power_and_speed(self):
        for profile in VOITURES:
            with self.subTest(vehicle=profile["nom"]):
                self.assertGreaterEqual(profile["vmax"],650)
                self.assertGreaterEqual(profile["puissance"],1050)

    def test_physically_accelerates_from_standing_start(self):
        track=V6Track(V6Config())
        physics=V6Physics(track)
        for profile in VOITURES:
            with self.subTest(vehicle=profile["nom"]):
                x,y,angle=track.start_position()
                car=VehicleState(x,y,angle)
                wheels=[(-22,-21),(-22,21),(22,-21),(22,21)]
                for _ in range(60):
                    physics.step(car,Controls(gas=True),profile,wheels,0.0,1/60)
                self.assertGreater(car.v,100)
                car.validate()


if __name__=="__main__":
    unittest.main()
