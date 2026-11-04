"""Point d'entrée du moteur V6 modulaire (avec archives jouables séparément)."""
import logging
from pathlib import Path
import runpy
import sys


def main(argv: list[str] | None = None) -> None:
    args=list(sys.argv[1:] if argv is None else argv)
    if args in (["--help"],["-h"]):
        print("Karting : python main.py [--v6 | --modular | --legacy-v6]\n"
              "  (aucune option) / --v6 : moteur V6 réellement modulaire\n"
              "  --modular             : ancien moteur V4 modulaire\n"
              "  --legacy-v6           : V6 monolithique originale, pour comparaison")
        return
    logging.basicConfig(filename="karting.log",level=logging.WARNING,
                        format="%(asctime)s %(levelname)s %(name)s: %(message)s")
    try:
        if args in ([],["--v6"]):
            from karting.v6.app import V6Game
            V6Game().run()
        elif args==["--modular"]:
            from karting.game import Game
            Game().run()
        elif args==["--legacy-v6"]:
            source=Path(__file__).resolve().with_name("karting_v6.py")
            runpy.run_path(str(source),run_name="__main__")
        else:
            raise SystemExit("Option inconnue. Lancer python main.py --help")
    except Exception:
        logging.exception("Arrêt du jeu suite à une erreur non récupérable")
        raise


if __name__=="__main__":
    main()
