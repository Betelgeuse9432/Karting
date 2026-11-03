# 🏎️ Karting

Un prototype de jeu de karting en **Python avec Pygame**, avec une vue 3D simulée, un circuit, une conduite influencée par l'adhérence et plusieurs effets visuels.

> **État du projet :** prototype V3 disponible. Une architecture modulaire et plus robuste est en préparation sur la branche `dev`. La refonte n'est pas encore implémentée.

## Fonctionnalités du prototype V3

- Circuit fermé, avec bordures, herbe et voie des stands.
- Voiture représentée par des volumes 3D projetés à l'écran.
- Accélération, freinage, marche arrière et direction progressive.
- Perte d'adhérence à l'avant ou à l'arrière selon le freinage, l'accélération et les virages.
- Appui aérodynamique variant avec la vitesse et aileron mobile au freinage.
- Fumée, étincelles, flammes d'échappement et traces de pneus.
- Chronométrage des tours, meilleur temps sauvegardé localement et tableau des scores dans les stands.
- Affichage de la vitesse, des pédales, de la température des disques et d'une mini-carte.

Les réglages physiques actuels sont des paramètres de jeu, pas une simulation certifiée de dynamique automobile.

## Installation

**Prérequis :** Python et `pip` installés.

Récupérer la branche de développement :

```powershell
git clone -b dev https://github.com/Betelgeuse9432/Karting.git
cd Karting
```

Installer la dépendance :

```powershell
python -m pip install -r requirements.txt
```

Lancer le prototype :

```powershell
python karting_v3.py
```

## Commandes

| Action | Touches |
| --- | --- |
| Accélérer | `↑`, `Z` ou `W` |
| Freiner / marche arrière | `↓` ou `S` |
| Tourner à gauche | `←`, `Q` ou `A` |
| Tourner à droite | `→` ou `D` |
| Recommencer | `R` |
| Quitter | `Échap` |

Les pédales réagissent progressivement au maintien des touches.

## Organisation actuelle

```text
Karting/
├── karting_v3.py          # Prototype jouable, actuellement monolithique
├── docs/
│   └── architecture.md    # Architecture cible et plan de migration
├── requirements.txt       # Dépendance Python
├── .gitignore             # Exclusions Git
└── README.md
```

## Architecture prévue

La migration cherchera à séparer :

- **Simulation :** véhicule, physique, adhérence, circuit et chronométrage.
- **Affichage :** caméra, rendu 3D, décors, effets et interface.
- **Orchestration :** boucle de jeu, entrées, état et configuration.
- **Fiabilité :** tests, diagnostics, persistance et dégradation contrôlée des composants facultatifs.

L'objectif est notamment de pouvoir **tester la physique sans lancer Pygame**, puis de détecter et isoler les erreurs qui ne doivent pas interrompre le jeu. Un composant indispensable défaillant doit, lui, provoquer une réaction contrôlée plutôt qu'une poursuite dans un état incohérent.

📐 **Voir le document de conception :** [docs/architecture.md](docs/architecture.md).

## Branches

- `main` : référence du prototype V3 d'origine.
- `dev` : évolution de l'architecture et du jeu.

Pendant la migration, `karting_v3.py` reste notre référence fonctionnelle. Les futurs modules et le nouveau point d'entrée seront ajoutés progressivement, puis validés avant leur remplacement.

## Prochaines étapes

1. Extraire les paramètres, l'état et les données du circuit.
2. Isoler la physique et le suivi des tours, avec des tests unitaires.
3. Séparer la caméra, les graphismes, les effets et l'interface.
4. Construire une boucle de jeu capable de signaler les défaillances et de désactiver les éléments non essentiels.
5. Vérifier que le comportement reste cohérent avec la V3.
