"""Point d'entrée officiel de la version modulaire du jeu Karting."""
import logging


def main() -> None:
    logging.basicConfig(filename="karting.log",level=logging.WARNING,
                        format="%(asctime)s %(levelname)s %(name)s: %(message)s")
    try:
        from karting.game import Game
        Game().run()
    except Exception:
        logging.exception("Arrêt du jeu suite à une erreur non récupérable")
        raise


if __name__ == "__main__":
    main()
