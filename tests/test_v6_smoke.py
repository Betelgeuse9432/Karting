"""Smoke test SDL de la V6 modulaire : menu, course, météo et qualité."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import textwrap
import unittest


ROOT=Path(__file__).resolve().parents[1]


class V6SmokeTest(unittest.TestCase):
    @unittest.skipUnless(importlib.util.find_spec("pygame") is not None,"Pygame absent")
    def test_modular_v6_headless(self):
        script=textwrap.dedent("""
            import pygame
            from unittest.mock import patch
            from karting.v6.app import V6Game

            frames=0
            def events():
                global frames
                frames+=1
                key={1:pygame.K_RETURN,2:pygame.K_n,3:pygame.K_p,
                     4:pygame.K_g}.get(frames)
                if key is not None:
                    return [pygame.event.Event(pygame.KEYDOWN,key=key)]
                if frames>=6:
                    return [pygame.event.Event(pygame.QUIT)]
                return []

            app=V6Game()
            with patch.object(pygame.event,"get",side_effect=events):
                app.run()
            assert app.mode=="race",app.mode
            assert app.session.weather["phase"]==1
            assert app.session.weather["pluie_on"]
            assert app.session.quality==2
            assert frames>=6
        """)
        env=os.environ.copy()
        env["SDL_VIDEODRIVER"]="dummy"
        env["SDL_AUDIODRIVER"]="dummy"
        env["PYGAME_HIDE_SUPPORT_PROMPT"]="1"
        process=subprocess.run([sys.executable,"-c",script],cwd=ROOT,env=env,
                               stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                               text=True,timeout=90,check=False)
        self.assertEqual(process.returncode,0,
                         "V6 modulaire a échoué :\n"+process.stdout[-4000:]
                         +process.stderr[-6000:])


if __name__=="__main__":
    unittest.main()
