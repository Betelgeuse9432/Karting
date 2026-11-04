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
        definitions = [node for node in self.tree.body
                       if isinstance(node, ast.Assign)
                       and any(isinstance(target, ast.Name) and target.id == "VOITURES"
                               for target in node.targets)]
        self.assertEqual(len(definitions), 1)
        self.assertIsInstance(definitions[0].value, ast.List)
        self.assertEqual(len(definitions[0].value.elts), 4)

    def test_v6_core_functions_are_available(self):
        functions = {node.name for node in self.tree.body if isinstance(node, ast.FunctionDef)}
        expected = {"hauteur", "dessiner_menu", "dessiner_voiture",
                    "dessiner_ciel", "dessiner_lumieres", "dessiner_compteur",
                    "dessiner_tribune", "dessiner_panneau", "roue_dehors"}
        self.assertTrue(expected.issubset(functions), expected - functions)

    def test_v6_weather_and_handbrake_present(self):
        for token in ('meteo["pluie"]', 'meteo["nuit"]',
                      'main_frein = bool(t[pygame.K_SPACE])', 'v_laterale',
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
