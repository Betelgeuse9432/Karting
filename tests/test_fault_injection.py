"""Tests du saboteur et de son intégration au gestionnaire d'erreurs."""
import sys
import types
import unittest
from unittest.mock import patch

from karting.fault_injection import (
    FaultInjector, InjectedFault, InjectionSettings, OPTIONAL_TARGETS,
)
from karting.faults import FaultManager


class InjectionTests(unittest.TestCase):
    def test_interval_validation(self):
        for minimum, maximum in ((0,2),(2,1),(float('nan'),2),(2,float('inf'))):
            with self.assertRaises(ValueError):
                InjectionSettings(min_interval=minimum,max_interval=maximum).validate()

    def test_repeatability(self):
        settings=InjectionSettings(0.1,0.5,seed=2026,include_essential=False)
        def sequence():
            injector=FaultInjector(settings)
            selected=[]
            manager=FaultManager(injector.before_call)
            for _ in range(25):
                injector.advance(0.5,manager.disabled)
                if injector.pending:
                    selected.append(injector.pending)
                    with self.assertLogs('karting',level='WARNING'):
                        manager.optional(injector.pending,lambda:None)
                if not set(OPTIONAL_TARGETS)-manager.disabled:
                    break
            return selected
        self.assertEqual(sequence(),sequence())

    def test_optional_failure_is_isolated(self):
        injector=FaultInjector(InjectionSettings(.1,.1,seed=13,include_essential=False))
        manager=FaultManager(injector.before_call)
        injector.advance(.1,manager.disabled)
        component=injector.pending
        self.assertIn(component,OPTIONAL_TARGETS)
        touched=[]
        with self.assertLogs('karting',level='WARNING'):
            self.assertEqual(manager.optional(component,lambda:touched.append(1),'secours'),'secours')
        self.assertFalse(touched)
        self.assertIn(component,manager.disabled)
        self.assertEqual(injector.count,1)
        self.assertEqual(manager.optional('unrelated',lambda:'OK'),'OK')
        self.assertEqual(manager.optional(component,lambda:'NO','secours'),'secours')
        self.assertEqual(injector.count,1)
        manager.recover()
        injector.reset()
        self.assertEqual(manager.optional(component,lambda:'OK'),'OK')

    def test_toggle_and_manual_injection(self):
        injector=FaultInjector(InjectionSettings(2,3,seed=3,include_essential=False))
        self.assertFalse(injector.toggle())
        self.assertIsNone(injector.force_random(set()))
        injector.advance(20,set())
        self.assertEqual(injector.time,0)
        self.assertTrue(injector.toggle())
        component=injector.force_random(set())
        self.assertIsNotNone(component)
        with self.assertRaises(InjectedFault):
            injector.before_call(component)
        self.assertEqual(injector.count,1)

    def test_excludes_disabled_optional_targets(self):
        injector=FaultInjector(InjectionSettings(.1,.1,seed=8,include_essential=False))
        disabled=set(OPTIONAL_TARGETS)
        injector.advance(.1,disabled)
        self.assertIsNone(injector.pending)
        disabled.remove('render.hud')
        injector.advance(.1,disabled)
        self.assertEqual(injector.pending,'render.hud')


class IntegrationTests(unittest.TestCase):
    def test_game_rollbacks_and_resets(self):
        # Faux module pygame uniquement pour importer les classes de rendu.
        # Aucun rendu n'est exécuté : la physique demeure indépendante de Pygame.
        pygame=types.ModuleType('pygame')
        pygame.Surface=object
        pygame.key=types.SimpleNamespace(ScancodeWrapper=object)
        with patch.dict(sys.modules,{'pygame':pygame}):
            from karting.game import Game
            from karting.state import Controls
            injector=FaultInjector(InjectionSettings(2,3,seed=7))
            game=Game(injector=injector)
            injector.pending='effects.sim'
            with self.assertLogs('karting',level='WARNING'):
                game.step(Controls(gas=True),.016)
            self.assertGreater(game.car.v,0)
            self.assertFalse(game.paused)
            self.assertIn('effects.sim',game.faults.disabled)
            injector.pending='physics'
            before=game.car.v
            with self.assertLogs('karting',level='WARNING'):
                game.step(Controls(gas=True),.016)
            self.assertTrue(game.paused)
            self.assertEqual(game.car.v,before)
            self.assertIn('physics',game.faults.disabled)
            game.reset()
            self.assertFalse(game.paused)
            self.assertFalse(game.faults.disabled)
            game.step(Controls(gas=True),.016)
            self.assertGreater(game.car.v,0)


if __name__=='__main__':
    unittest.main()
