"""Point d'entrée de la branche test : sabotage aléatoire activé par défaut."""
import argparse
import logging


def main() -> None:
    parser=argparse.ArgumentParser(description="Karting - tests de pannes aléatoires")
    parser.add_argument("--safe",action="store_true",help="désactiver les pannes injectées")
    parser.add_argument("--seed",type=int,default=None,help="graine pour reproduire les pannes")
    parser.add_argument("--min-interval",type=float,default=2.0,help="délai minimum entre pannes (secondes)")
    parser.add_argument("--max-interval",type=float,default=5.0,help="délai maximum entre pannes (secondes)")
    parser.add_argument("--no-essential",action="store_true",help="ne saboter que des composants facultatifs")
    args=parser.parse_args()
    logging.basicConfig(filename="karting.log",level=logging.INFO,
                        format="%(asctime)s %(levelname)s %(name)s: %(message)s")
    try:
        from karting.game import Game
        from karting.fault_injection import FaultInjector, InjectionSettings
        injector=None if args.safe else FaultInjector(InjectionSettings(
            min_interval=args.min_interval,max_interval=args.max_interval,
            seed=args.seed,include_essential=not args.no_essential))
        Game(injector=injector).run()
    except Exception:
        logging.exception("Arrêt du jeu suite à une erreur non récupérable")
        raise


if __name__ == "__main__":
    main()
