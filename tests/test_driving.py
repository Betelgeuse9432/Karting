"""Tests de conduite et de dérive, sans dépendance à Pygame."""
import math
import unittest

from karting.config import GameConfig
from karting.physics import VehiclePhysics
from karting.state import Controls, VehicleState
from karting.track import Track


class HandbrakeTests(unittest.TestCase):
    def setUp(self):
        self.cfg = GameConfig()
        self.track = Track(self.cfg)
        self.physics = VehiclePhysics(self.cfg)
        self.x, self.y, self.heading = self.track.start_position()

    def make_car(self, speed=300):
        return VehicleState(self.x, self.y, self.heading, v=speed)

    def test_handbrake_decelerates_on_straight(self):
        car = self.make_car(300)
        normal = self.make_car(300)
        self.physics.step(car, Controls(handbrake=True), self.track, 0.016)
        self.physics.step(normal, Controls(), self.track, 0.016)
        self.assertLess(car.v, normal.v)
        self.assertEqual(car.lateral_speed, 0)

    def test_corner_with_handbrake_creates_lateral_inertia(self):
        car = self.make_car(330)
        drift_seen = False
        for _ in range(24):
            telemetry = self.physics.step(car, Controls(steering=1, handbrake=True), self.track, 0.016)
            drift_seen = drift_seen or telemetry.drifting
        self.assertTrue(drift_seen)
        self.assertGreater(abs(car.lateral_speed), 12)
        self.assertTrue(all(math.isfinite(x) for x in (car.x, car.y, car.v, car.angle)))

    def test_lateral_velocity_recovers_after_release(self):
        car = self.make_car(300)
        car.lateral_speed = 90
        self.physics.step(car, Controls(), self.track, 0.016)
        self.assertLess(abs(car.lateral_speed), 90)
        for _ in range(20):
            self.physics.step(car, Controls(), self.track, 0.016)
        self.assertEqual(car.lateral_speed, 0)

    def test_invalid_handbrake_settings(self):
        for kwargs in (dict(handbrake_drag=0), dict(handbrake_rear_grip=0),
                       dict(handbrake_rear_grip=2), dict(lateral_recovery=0),
                       dict(handbrake_lateral_factor=-1)):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                GameConfig(**kwargs).validate()


if __name__ == "__main__":
    unittest.main()
