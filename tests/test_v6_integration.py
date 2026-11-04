"""Intégration V6 : tests de structure et du point d'entrée sans lancer Pygame."""
import ast
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

    def test_main_entry_defaults_to_v6(self):
        with patch.object(main.runpy, "run_path") as launch:
            main.main([])
        self.assertEqual(launch.call_count, 1)
        self.assertEqual(Path(launch.call_args.args[0]).name, "karting_v6.py")
        self.assertEqual(launch.call_args.kwargs["run_name"], "__main__")

    def test_main_rejects_unknown_modes(self):
        with self.assertRaises(SystemExit):
            main.main(["--nonexistent"])


if __name__ == "__main__":
    unittest.main()
