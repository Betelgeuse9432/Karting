"""Intégration V6 : tests de structure et du point d'entrée sans lancer Pygame."""
import ast
import sys
from types import SimpleNamespace
from unittest.mock import MagicMock
from pathlib import Path
import unittest
from unittest.mock import patch

import main


ROOT = Path(__file__).resolve().parents[1]
V6 = ROOT / "karting_v6.py"


class V6IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = V6.read_text(encoding="utf-8")
        cls.tree = ast.parse(cls.source, filename=str(V6))

    def test_v6_is_valid_python(self):
        compile(self.tree, str(V6), "exec")

    def test_four_car_presets(self):
        from karting.v6.vehicles import PALETTE, VOITURES
        self.assertEqual(len(VOITURES), 4)
        self.assertEqual([car["nom"] for car in VOITURES], ["KART", "4x4", "GT3", "WEC"])
        self.assertGreaterEqual(len(PALETTE), 4)

    def test_v6_core_functions_are_available(self):
        functions = {node.name for node in self.tree.body if isinstance(node, ast.FunctionDef)}
        expected = {"hauteur", "dessiner_menu", "dessiner_voiture",
                    "dessiner_ciel", "dessiner_lumieres", "dessiner_compteur",
                    "dessiner_tribune", "dessiner_panneau", "roue_dehors"}
        self.assertTrue(expected.issubset(functions), expected - functions)

    def test_v6_weather_and_handbrake_present(self):
        for token in ('meteo["pluie"]', 'meteo["nuit"]',
                      'commandes.handbrake', 'v_laterale',
                      'record_karting_v6.txt', 'DERAPAGE'):
            with self.subTest(token=token):
                self.assertIn(token, self.source)

    def test_main_entry_defaults_to_modular_v6(self):
        engine=MagicMock()
        fake_module=SimpleNamespace(V6Game=engine)
        with patch.dict(sys.modules, {"karting.v6.app":fake_module}), patch.object(main.runpy, "run_path") as legacy:
            main.main([])
        engine.assert_called_once()
        engine.return_value.run.assert_called_once()
        legacy.assert_not_called()

    def test_legacy_v6_still_available(self):
        with patch.object(main.runpy,"run_path") as launch:
            main.main(["--legacy-v6"])
        self.assertEqual(Path(launch.call_args.args[0]).name,"karting_v6.py")

    def test_main_rejects_unknown_modes(self):
        with self.assertRaises(SystemExit):
            main.main(["--nonexistent"])


if __name__ == "__main__":
    unittest.main()
