# 🏎️ Karting

**Jeu de karting en Python/Pygame avec projection 3D, physique arcade et chronométrage.**

> **Branche `test` : campagne expérimentale de pannes aléatoires.** Le sabotage est actif par défaut dans `python main.py` ; `--safe` permet de jouer sans injection. Les branches `dev` et `main` restent indépendantes.

## Installer et lancer

Python 3.10+ recommandé.

```powershell
git clone -b test https://github.com/Betelgeuse9432/Karting.git
cd Karting
python -m pip install -r requirements.txt
python main.py
```

Pour jouer avec l'ancien prototype :

```powershell
python karting_v3.py
```

Si le dépôt existe déjà en local sur `dev` : `git fetch origin`, puis `git switch --track origin/test`. Ensuite : `git pull origin test`.

## 🧪 Sabotage aléatoire sur `test`

Le moteur choisit un composant au hasard toutes les **2 à 5 secondes** de simulation et y lève volontairement une exception. Les modules optionnels sont isolés puis réessayés automatiquement. Une panne physique ou de chronométrage met temporairement la simulation en pause, puis le calcul interrompu est rejoué jusqu'à réussir. Les événements sont enregistrés dans `karting.log`.

```powershell
python main.py --seed 42       # scénario reproductible
python main.py --no-essential # ne touche pas à la physique ni au chrono
python main.py --safe         # sans pannes injectées
```

**F9** active/désactive le sabotage ; **F10** déclenche une panne aléatoire dès le prochain appel du composant choisi ; **R** réinitialise volontairement le jeu. Les réparations sont désormais automatiques, sans besoin d'appuyer sur R. Un bandeau affiche les injections.

Voir [docs/fault-testing.md](docs/fault-testing.md) pour le protocole complet, les cibles, les limites et les tests.

## Contrôles

| Action | Touches |
| --- | --- |
| Accélération | `↑`, `Z`, `W` |
| Frein et marche arrière | `↓`, `S` |
| Tourner à gauche | `←`, `Q`, `A` |
| Tourner à droite | `→`, `D` |
| Réinitialisation manuelle de la partie (facultative) | `R` |
| Quitter | `Échap` |

## Fonctionnalités conservées depuis la V3

- Circuit fermé lissé, vibreurs, stands, garages, arbres.
- Voiture constituée de surfaces 3D projetées en perspective, caméra suiveuse.
- Accélération et freinage progressifs, perte d'adhérence avant/arrière, appui aérodynamique.
- Carrosserie animée, aileron mobile, roues et disques de frein.
- Fumée, étincelles, flammes et traces de glisse.
- Mini-carte, affichage des pédales, du chrono, des temps par tour et des records.

Ces paramètres représentent une **physique de jeu**, pas un modèle certifié de conduite réelle.

## Structure de la refonte

```text
Karting/
├── main.py                 # point d'entrée V4
├── karting_v3.py           # prototype original conservé
├── karting/
│   ├── config.py           # configuration validée et immuable
│   ├── state.py            # état du kart et commandes
│   ├── track.py            # piste, normales, stands et garages
│   ├── scenery.py          # arbres générés indépendamment
│   ├── input.py            # adaptation clavier Pygame
│   ├── physics.py          # physique sans Pygame
│   ├── race.py             # secteurs et temps au tour
│   ├── persistence.py      # records et sauvegarde atomique
│   ├── camera.py           # transformation et projection 3D sans Pygame
│   ├── effects.py          # particules et traces sans Pygame
│   ├── faults.py           # diagnostic et isolation des erreurs
│   ├── fault_injection.py  # injection de pannes aléatoires sur test
│   ├── game.py             # orchestration de tous les services
│   └── render/
│       ├── common.py       # primitives et mélange de couleurs
│       ├── world.py        # ciel, piste, végétation, stands
│       ├── vehicle.py      # dessin de la voiture
│       ├── particles.py    # dessin des effets
│       ├── hud.py          # interface et panneaux
│       └── compositor.py   # ordre de profondeur des objets
├── tests/
│   ├── test_core.py        # tests sans fenêtre
│   ├── test_fault_injection.py # tests du saboteur
│   └── test_recovery.py      # vrais retries et pannes permanentes
├── docs/
│   ├── architecture.md     # contrats et décisions d'architecture
│   └── fault-testing.md    # protocole de pannes aléatoires
├── requirements.txt
└── .gitignore
```

**Règle :** la physique ne connaît ni l'écran, ni Pygame, ni les particules. Chaque composant reçoit explicitement les objets dont il a besoin. Un défaut du rendu optionnel ne doit pas interrompre la simulation.

## Tests

```powershell
python -m unittest discover -s tests -v
python -m compileall -q karting
```

La suite couvre la géométrie, les stands, la physique, la course, les records, les projections, les effets et la gestion des défaillances. La parité numérique de la physique a également été vérifiée sur 120 pas contre la V3 originale. **Les tests ne remplacent pas une vérification graphique et des FPS sous Pygame.**

## Résilience et limites

- Les erreurs des éléments optionnels sont journalisées dans `karting.log` : décors, particules, HUD, sauvegarde et autres rendus.
- Un élément en défaut est isolé, puis sa fonction réelle est réessayée avec délai croissant (0,5, 1, 2, 4 et 8 secondes). Il n'est déclaré réparé qu'après réussite.
- Une erreur physique ou de chronométrage provoque une pause contrôlée ; l'opération interrompue est rejouée automatiquement depuis un état conservé avant reprise.
- Les records sont sauvegardés atomiquement dans `record_karting.txt`. Ce fichier local n'est pas versionné.
- La refonte n'est **pas** un moteur d'auto-réparation du code source : elle détecte, isole, réessaie et reconstruit certains objets défectueux. Une panne permanente reste diagnostiquée.

La prochaine étape est la validation visuelle sur Windows avec Pygame, puis l'optimisation des calculs et l'amélioration du modèle de conduite.
