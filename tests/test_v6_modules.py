"""Tests purs des sous-modules V6 (sans fenêtre ni Pygame)."""
import unittest

from karting.v6.track_layout import BRUT
from karting.v6.vehicles import VOITURES, PALETTE
from karting.v6.weather import step_weather


class V6ModuleTests(unittest.TestCase):
    def test_large_track_has_unique_sections(self):
        self.assertGreaterEqual(len(BRUT), 20)
        self.assertGreater(len(set(BRUT)), 20)

    def test_vehicle_profiles_remain_independent(self):
        self.assertEqual(len(VOITURES), 4)
        self.assertEqual(len({v["nom"] for v in VOITURES}), 4)
        for vehicle in VOITURES:
            with self.subTest(vehicle=vehicle["nom"]):
                self.assertGreater(vehicle["vmax"], 0)
                self.assertGreater(vehicle["grip_av"], 0)
                self.assertGreater(vehicle["emp"], 0)
                self.assertLess(vehicle["defaut"], len(PALETTE))

    def test_weather_transitions_gradually(self):
        weather = {"phase": 2, "nuit": 0.0, "pluie_on": True, "pluie": 0.0}
        night, rain = step_weather(weather, 0.05)
        self.assertAlmostEqual(night, 0.04)
        self.assertAlmostEqual(rain, 0.03)
        self.assertEqual(weather["phase"], 2)
        self.assertTrue(weather["pluie_on"])

    def test_weather_can_return_to_day_and_dry(self):
        weather = {"phase": 0, "nuit": 1.0, "pluie_on": False, "pluie": 1.0}
        for _ in range(20):
            step_weather(weather, 0.05)
        self.assertLess(weather["nuit"], 1.0)
        self.assertLess(weather["pluie"], 1.0)
        self.assertGreaterEqual(weather["nuit"], 0.0)
        self.assertGreaterEqual(weather["pluie"], 0.0)

    def test_weather_rejects_invalid_steps_and_phases(self):
        for bad_dt in (-0.1, 0.1, float("nan")):
            with self.subTest(dt=bad_dt), self.assertRaises(ValueError):
                step_weather({"phase": 0, "nuit": 0, "pluie_on": False, "pluie": 0}, bad_dt)
        with self.assertRaises(ValueError):
            step_weather({"phase": 3, "nuit": 0, "pluie_on": False, "pluie": 0}, 0.01)


if __name__ == "__main__":
    unittest.main()
