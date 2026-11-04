"""Smoke test réel de la V6 sous pygame avec pilote vidéo factice.

Pas de fenêtre ouverte, pas de simulation infinie : le processus reçoit
Entrée, N, P, G puis Quitter sur quelques frames seulement.
"""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[1]


class V6PygameSmokeTest(unittest.TestCase):
    @unittest.skipUnless(importlib.util.find_spec("pygame") is not None,
                         "Pygame n'est pas installé")
    def test_v6_opens_menu_and_renders_game(self):
        script = textwrap.dedent("""
            import pygame
            import runpy
            from unittest.mock import patch

            count = 0
            def events():
                global count
                count += 1
                key = {1: pygame.K_RETURN, 2: pygame.K_n,
                       3: pygame.K_p, 4: pygame.K_g}.get(count)
                if key is not None:
                    return [pygame.event.Event(pygame.KEYDOWN, key=key)]
                if count >= 6:
                    return [pygame.event.Event(pygame.QUIT)]
                return []

            with patch.object(pygame.event, "get", side_effect=events):
                state = runpy.run_path("karting_v6.py", run_name="__main__")
            assert state["mode"] == "jeu", state["mode"]
            assert state["meteo"]["phase"] == 1
            assert state["meteo"]["pluie_on"]
            assert state["QUALITE"] != 1
            assert count >= 6
        """)
        env = os.environ.copy()
        env["SDL_VIDEODRIVER"] = "dummy"
        env["SDL_AUDIODRIVER"] = "dummy"
        env["PYGAME_HIDE_SUPPORT_PROMPT"] = "1"
        process = subprocess.run([sys.executable, "-c", script],
                                 cwd=ROOT, env=env, text=True,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 timeout=90, check=False)
        self.assertEqual(process.returncode, 0,
                         "V6 n'a pas passé le rendu headless :\n"
                         + process.stdout[-4000:] + process.stderr[-6000:])


if __name__ == "__main__":
    unittest.main()
