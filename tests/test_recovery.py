"""Tests de reprise autonome : pannes transitoires, persistantes et état préservé."""
import sys
import types
import unittest
from unittest.mock import patch

from karting.faults import FaultManager
from karting.fault_injection import FaultInjector, InjectionSettings


class RetryTests(unittest.TestCase):
    def test_retry_exécute_reellement_le_composant(self):
        manager = FaultManager()
        attempts = []
        def flaky():
            attempts.append(manager.time)
            if len(attempts) < 3:
                raise RuntimeError("défaut transitoire")
            return "OK"
        with self.assertLogs("karting", level="ERROR"):
            self.assertEqual(manager.optional("hud", flaky, "secours"), "secours")
        manager.advance(.49)
        self.assertEqual(manager.optional("hud", flaky, "secours"), "secours")
        self.assertEqual(len(attempts), 1)
        manager.advance(.01)
        with self.assertLogs("karting", level="ERROR"):
            self.assertEqual(manager.optional("hud", flaky, "secours"), "secours")
        manager.advance(.99)
        self.assertEqual(manager.optional("hud", flaky, "secours"), "secours")
        manager.advance(.02)
        with self.assertLogs("karting", level="INFO"):
            self.assertEqual(manager.optional("hud", flaky, "secours"), "OK")
        self.assertEqual(len(attempts), 3)
        self.assertNotIn("hud", manager.disabled)
        self.assertEqual(manager.total_recoveries, 1)

    def test_panne_permanente_ne_declenche_pas_de_fausse_reparation(self):
        manager = FaultManager()
        calls = []
        def broken():
            calls.append(1)
            raise OSError("indisponible")
        with self.assertLogs("karting", level="ERROR"):
            for i in range(9):
                self.assertEqual(manager.optional("disk", broken, "cache"), "cache")
                self.assertIn("disk", manager.disabled)
                self.assertEqual(manager.pending["disk"].attempts, i+1)
                delay = manager.pending["disk"].next_retry - manager.time
                self.assertLessEqual(delay, 8.00001)
                manager.advance(delay)
        self.assertEqual(len(calls), 9)
        self.assertEqual(manager.total_recoveries, 0)

    def test_reconstruction_puis_vrai_appel(self):
        manager = FaultManager()
        state = {"broken": True}
        def draw():
            if state["broken"]:
                raise RuntimeError("contexte endommagé")
            return 42
        with self.assertLogs("karting", level="ERROR"):
            self.assertIsNone(manager.optional("camera", draw))
        manager.advance(.5)
        with self.assertLogs("karting", level="INFO"):
            self.assertEqual(manager.optional("camera", draw, on_retry=lambda:state.update(broken=False)), 42)
        self.assertNotIn("camera", manager.disabled)

    def test_pas_de_boucle_d_exceptions_a_chaque_image(self):
        manager = FaultManager()
        with self.assertLogs("karting", level="ERROR"):
            manager.optional("screen", lambda:1/0)
        for _ in range(1000):
            self.assertIsNone(manager.optional("screen", lambda:"repaired"))
        self.assertIn("screen", manager.disabled)
        manager.advance(.5)
        with self.assertLogs("karting", level="INFO"):
            self.assertEqual(manager.optional("screen", lambda:"repaired"), "repaired")


class GameRecoveryTests(unittest.TestCase):
    def setUp(self):
        pygame = types.ModuleType("pygame")
        pygame.Surface = object
        pygame.key = types.SimpleNamespace(ScancodeWrapper=object)
        self.patch = patch.dict(sys.modules, {"pygame":pygame})
        self.patch.start()
        from karting.game import Game
        from karting.state import Controls
        self.Game = Game
        self.Controls = Controls

    def tearDown(self):
        self.patch.stop()

    def test_physique_repart_sans_reset(self):
        injector = FaultInjector(InjectionSettings(2, 3, seed=12))
        game = self.Game(injector=injector)
        game.step(self.Controls(gas=True), .016)
        before_v = game.car.v
        before_time = game.race.state.elapsed
        injector.pending = "physics"
        with self.assertLogs("karting", level="WARNING"):
            game.step(self.Controls(gas=True), .016)
        self.assertTrue(game.paused)
        self.assertEqual(game.car.v, before_v)
        self.assertEqual(game.race.state.elapsed, before_time)
        game.service_retries(.25)
        game.step(self.Controls(gas=True), .016)
        self.assertTrue(game.paused)
        game.service_retries(.25)
        with self.assertLogs("karting", level="INFO"):
            game.step(self.Controls(gas=True), .016)
        self.assertFalse(game.paused)
        self.assertGreater(game.car.v, before_v)
        self.assertGreater(game.race.state.elapsed, before_time)
        self.assertNotIn("physics", game.faults.disabled)

    def test_course_repart_sans_reset(self):
        injector = FaultInjector(InjectionSettings(2, 3, seed=21))
        game = self.Game(injector=injector)
        injector.pending = "race"
        with self.assertLogs("karting", level="WARNING"):
            game.step(self.Controls(gas=True), .016)
        self.assertTrue(game.paused)
        self.assertEqual(game.race.state.elapsed, 0)
        game.service_retries(.5)
        with self.assertLogs("karting", level="INFO"):
            game.step(self.Controls(), .016)
        self.assertFalse(game.paused)
        self.assertGreater(game.race.state.elapsed, 0)

    def test_sauvegarde_retentee_jusqua_reussite(self):
        game = self.Game()
        saved = []
        def save(value):
            saved.append(value)
            if len(saved) < 3:
                raise OSError("dossier bloqué")
        game.storage.save = save
        game._pending_save = 12.2
        with self.assertLogs("karting", level="ERROR"):
            game.service_retries(.01)
            game.service_retries(.5)
        game.service_retries(.5)
        self.assertEqual(len(saved), 2)
        with self.assertLogs("karting", level="INFO"):
            game.service_retries(.5)
        self.assertEqual(saved, [12.2, 12.2, 12.2])
        self.assertIsNone(game._pending_save)


if __name__ == "__main__":
    unittest.main()
