# 🏎️ Karting

**Jeu de karting en Python/Pygame avec projection 3D, physique arcade et chronométrage.**

> **Branche `dev` : refonte modulaire V4 expérimentale.** Le prototype V3 est conservé dans `karting_v3.py` pour référence. Le rendu visuel de la V4 doit encore être validé avec un vrai Pygame.

## Installer et lancer

Python 3.10+ recommandé.

```powershell
git clone -b dev https://github.com/Betelgeuse9432/Karting.git
cd Karting
python -m pip install -r requirements.txt
python main.py
```

Pour jouer avec l'ancien prototype :

```powershell
python karting_v3.py
```

Si le dépôt existe déjà en local : `git pull origin dev` avant de lancer le jeu.

## Contrôles

| Action | Touches |
| --- | --- |
| Accélération | `↑`, `Z`, `W` |
| Frein et marche arrière | `↓`, `S` |
| Frein à main / dérapage | `Espace` |
| Tourner à gauche | `←`, `Q`, `A` |
| Tourner à droite | `→`, `D` |
| Réinitialiser le kart et réessayer les composants désactivés | `R` |
| Quitter | `Échap` |

## Fonctionnalités conservées depuis la V3

- Circuit fermé lissé, vibreurs, stands, garages, arbres.
- Voiture constituée de surfaces 3D projetées en perspective, caméra suiveuse.
- Accélération et freinage progressifs, perte d'adhérence avant/arrière, appui aérodynamique.
- **Nouveau sur `dev` :** frein à main sur `Espace`, inertie latérale pendant les dérapages, traces de pneus et avertissement à l’écran.
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
│   ├── game.py             # orchestration de tous les services
│   └── render/
│       ├── common.py       # primitives et mélange de couleurs
│       ├── world.py        # ciel, piste, végétation, stands
│       ├── vehicle.py      # dessin de la voiture
│       ├── particles.py    # dessin des effets
│       ├── hud.py          # interface et panneaux
│       └── compositor.py   # ordre de profondeur des objets
├── tests/
│   └── test_core.py        # tests sans fenêtre
├── docs/
│   └── architecture.md     # contrats et décisions d'architecture
├── requirements.txt
└── .gitignore
```

**Règle :** la physique ne connaît ni l'écran, ni Pygame, ni les particules. Chaque composant reçoit explicitement les objets dont il a besoin. Un défaut du rendu optionnel ne doit pas interrompre la simulation.

## Tests

```powershell
python -m unittest discover -s tests -v
python -m compileall -q karting
```

La suite couvre la géométrie, les stands, la physique, la course, les records, les projections, les effets et la gestion des défaillances. La parité numérique V3 a été vérifiée avant cet ajout ; elle n’est plus attendue quand le frein à main est activé. **Les tests ne remplacent pas une vérification graphique et des FPS sous Pygame.**

## Résilience et limites

- Les erreurs des éléments optionnels sont journalisées dans `karting.log` : décors, particules, HUD, sauvegarde et autres rendus.
- Un élément en défaut est temporairement désactivé. Le jeu continue tant que la simulation reste valide.
- Une erreur physique ou de chronométrage provoque une pause contrôlée. `R` réinitialise et autorise une nouvelle tentative.
- Les records sont sauvegardés atomiquement dans `record_karting.txt`. Ce fichier local n'est pas versionné.
- La refonte n'est **pas** un moteur d'auto-réparation du code source : elle détecte, isole et permet de relancer les services après une panne.

La prochaine étape est la validation visuelle sur Windows avec Pygame, puis l'optimisation des calculs et l'amélioration du modèle de conduite.
