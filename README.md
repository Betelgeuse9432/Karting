# 🏎️ Karting

Projet Python/Pygame. **Sur la branche `dev`, `python main.py` lance désormais la V6 complète.**
La V6 provient du fichier `v6` de la branche `main`, sans modification de cet original.

## Installation et démarrage

```powershell
git switch dev
git pull origin dev
python -m pip install -r requirements.txt
python main.py
```

Pour continuer à tester l'ancien moteur modulaire V4 (conservé dans le dossier `karting/`) :

```powershell
python main.py --modular
```

`python karting_v6.py` lance directement la nouvelle version. `python main.py --help` explique les modes.

## Nouveautés V6 intégrées et jouables

- **Sélection de quatre véhicules** : kart, 4×4, GT3 et WEC, chacun avec ses réglages de physique et sa carrosserie 3D, ainsi que plusieurs coloris.
- **Nouveau grand circuit** avec relief, montées, descentes, rails de sécurité et collisions, vibreurs et stands.
- **Graphismes** : tribunes, publicités, lampadaires, végétation, ciel, nuages, soleil, lune, étoiles, éclairages, phares et brume.
- **Météo** : jour, crépuscule, nuit et pluie, avec influence sur l'adhérence.
- **Optimisation graphique** : trois niveaux de qualité et distance de rendu ajustable.
- **Course** : compte-tours, chronométrage, records, mini-carte, compteurs, pénalité de piste lorsque les quatre roues sortent.
- **Effets** : fumée, étincelles, flammes, traces de pneus, secousses de caméra et effets de freinage.
- **Ajout conservé depuis `dev`** : frein à main sur Espace avec dérive latérale et récupération progressive de l'adhérence.

Le fichier de records de la V6 est `record_karting_v6.txt`, distinct de celui de la V4 puisque les circuits sont différents.

## Contrôles V6

| Action | Touches |
| --- | --- |
| Accélérer | `↑`, `Z`, `W` |
| Freiner / marche arrière | `↓`, `S` |
| Direction | `←`, `→`, `Q`, `A`, `D` |
| **Frein à main / dérapage (en course)** | **`Espace`** |
| Choisir véhicule / couleur | Flèches au menu |
| Démarrer la course | `Entrée` ou `Espace` au menu |
| Cycle jour / crépuscule / nuit | `N` |
| Activer / désactiver pluie | `P` |
| Qualité graphique | `G` |
| Réinitialiser | `R` |
| Retour au menu | `M` |
| Quitter | `Échap` |

## Architecture et état de migration

```text
main.py              lance la V6 par défaut, ou la V4 avec --modular
karting_v6.py        V6 complète, préservant son fonctionnement d'origine
karting_v3.py        prototype V3 historique
karting/             moteur V4 modulaire (physique, piste, caméra, rendu...)
tests/               tests du moteur et vérification de l'intégration V6
docs/                notes d'architecture et de conception
```

**Important :** toutes les fonctionnalités V6 sont présentes dans `dev` et accessibles depuis l'entrée principale, mais **le moteur interne V6 reste pour l'instant monolithique**. Il n'a pas encore été refactorisé dans les classes et services de `karting/`. On conserve ce dernier comme base de la migration ultérieure, sans prétendre qu'un simple lancement constitue une vraie refonte modulaire.

La branche `test` reste le laboratoire séparé d'injection de pannes. La V6 ne bénéficie pas automatiquement de son système d'auto-réparation tant que ses composants n'ont pas été isolés.

## Vérifications

```powershell
python -m unittest discover -s tests -v
python -m compileall -q karting main.py karting_v6.py
```

Les tests statiques vérifient la présence de la V6 et la syntaxe, mais le rendu et les FPS doivent être validés sous Pygame, notamment pour chaque véhicule, en pluie et de nuit.
