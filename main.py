"""Lance la V6 complète par défaut ; conserve la V4 modulaire en mode alternatif.

La migration de la V6 vers les modules n'est pas terminée. Les deux moteurs
coexistent volontairement sans modifier l'original conservé sur main.
"""
import logging
from pathlib import Path
import runpy
import sys


def main(argv: list[str] | None = None) -> None:
    args = list(sys.argv[1:] if argv is None else argv)
    if args in (["--help"], ["-h"]):
        print("Karting : python main.py [--v6 | --modular]\n"
              "  (aucune option) / --v6 : nouvelle V6 complète\n"
              "  --modular             : moteur V4 modulaire et frein à main")
        return
    if args in ([], ["--v6"]):
        path = Path(__file__).resolve().with_name("karting_v6.py")
        logging.basicConfig(filename="karting.log", level=logging.WARNING,
                            format="%(asctime)s %(levelname)s %(name)s: %(message)s")
        try:
            runpy.run_path(str(path), run_name="__main__")
        except Exception:
            logging.exception("Arrêt de Karting V6 après une erreur non récupérable")
            raise
        return
    if args == ["--modular"]:
        logging.basicConfig(filename="karting.log", level=logging.WARNING,
                            format="%(asctime)s %(levelname)s %(name)s: %(message)s")
        try:
            from karting.game import Game
            Game().run()
        except Exception:
            logging.exception("Arrêt du moteur modulaire")
            raise
        return
    raise SystemExit("Option inconnue. Lancer python main.py --help")


if __name__ == "__main__":
    main()
