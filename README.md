# 🚗 Portage natif C++20 / raylib

**Cette branche `cpp-raylib` lance Karting depuis `cpp/`, pas depuis `python main.py`.**
La branche est issue de `dev` ; les fichiers Python restent présents uniquement comme référence pour les comparaisons et les tests de parité.

Pour compiler sous Windows avec CMake et Visual Studio C++ :

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --config Release --parallel
.\cpp\build\Release\karting_cpp.exe
```

Pour les commandes complètes, la structure et les limites du portage, consulte [le guide C++](cpp/README.md).

---

# 🏎️ Karting V6, moteur modulaire

**La branche `dev` utilise la V6 dans un moteur réellement modulaire.** La V6 originale de Nathan reste conservée dans `karting_v6.py` pour comparaison, mais **n'est plus exécutée par défaut**.

## Conduite plus rapide et optimisations

Les quatre véhicules ont été recalibrés sur `dev` pour accélérer plus fort et atteindre une vitesse plus élevée :

| Véhicule | Vitesse max. configurée avant → après | Puissance avant → après |
| --- | --- | --- |
| Kart | 520 → 650 | 900 → 1 300 |
| 4×4 | 560 → 680 | 600 → 1 050 |
| GT3 | 700 → 860 | 800 → 1 400 |
| WEC | 820 → 1 020 | 950 → 1 700 |

Ces valeurs sont des unités internes à la simulation, **pas des km/h réellement mesurés sur piste**. La résistance à l'avancement en accélération est aussi réduite. Les temps au tour et l'équilibrage sont à revoir après essai.

Le jeu demande moins de travail par image : relief du circuit pré-calculé, terrain mis en cache, recherche de piste par index spatial exact, décor hors champ ignoré, niveau de détail selon la distance. L'horloge physique reste à **60 pas/s** et peut exécuter jusqu'à huit pas par image si le rendu ralentit, afin que la course ne passe pas au ralenti lors d'une baisse de FPS. Une surcharge prolongée peut encore ralentir le temps simulé pour éviter d'aggraver le problème.

Pour alléger encore les graphismes en cours de partie, appuyer sur **`G`** jusqu'au mode **ECO**. Le compteur FPS est visible dans le HUD.

## Lancer

```powershell
git switch dev
git pull origin dev
python -m pip install -r requirements.txt
python -m unittest discover -s tests -v
python main.py
```

Options :

- `python main.py` : nouvelle **V6 modulaire** (jeu normal).
- `python main.py --modular` : ancien moteur V4 modulaire pour comparaison.
- `python main.py --legacy-v6` : V6 monolithique archivée, avec le frein à main ajouté.
- `python karting_v6.py` : lancement direct de la V6 archivée.

## Fonctionnalités de la V6 modulaire

- Quatre voitures avec géométries 3D distinctes, paramètres de conduite et couleurs : **KART, 4×4, GT3, WEC**.
- Grand circuit V6, relief, vibreurs, rails de sécurité, stands, garages et pénalités lorsque les quatre roues sont hors piste.
- Physique propre à chaque voiture, adhérence différente sur herbe et sous la pluie, frein à main `Espace` et dérapages avec inertie latérale.
- Menu 3D de sélection de véhicule et de couleur, statuts des voitures et caractéristiques affichées.
- Cycle jour/crépuscule/nuit, pluie, étoiles, soleil, lune, nuages, brume, lampadaires, phares et feux.
- Décors : arbres, tribunes, publicités, garages, portique ; tri visuel selon la profondeur.
- Trois niveaux graphiques, mini-carte, compteur, tours, records et tableau des stands.
- Particules, traces de pneus, flammes, fumée, étincelles et effets météorologiques.

### Commandes

| Action | Touche |
| --- | --- |
| Accélérer | `↑`, `Z`, `W` |
| Freiner / marche arrière | `↓`, `S` |
| Direction | `←` / `→`, `Q` / `D` |
| Frein à main / dérapage en course | **`Espace`** |
| Choisir voiture / couleur au menu | Flèches |
| Lancer la course | `Entrée` ou `Espace` au menu |
| Jour / crépuscule / nuit | `N` |
| Pluie | `P` |
| Qualité graphique | `G` |
| Recommencer | `R` |
| Menu principal | `M` |
| Quitter | `Échap` |

## Architecture

```text
main.py                      lance la V6 modulaire
karting/
├── v6/
│   ├── app.py               seule boucle Pygame, événements et orchestration des rendus
│   ├── session.py           moteur de course, état, chronomètre, effets et sauvegarde
│   ├── physics.py           conduite, dérapage, appui, pluie, herbe, pentes, collisions
│   ├── track.py             piste, stands, rails, relief, limites des quatre roues
│   ├── track_layout.py      points du circuit, sans affichage
│   ├── vehicles.py          profils de voitures et palette
│   ├── config.py            paramètres immuables du jeu
│   ├── weather.py           transitions météo déterministes
│   ├── camera.py            caméra 3D, relief et projection perspective
│   ├── mesh.py              géométrie exacte des quatre modèles 3D V6
│   ├── vehicle_render.py    carrosseries 3D et animations
│   ├── world_render.py      terrain, piste, stands, rails
│   ├── scenery.py           arbres, tribunes, pubs, lampadaires
│   ├── decor_render.py      rendu des décors indépendants
│   ├── sky.py               soleil, lune, nuages, étoiles et montagnes
│   ├── atmosphere_render.py pluie, nuit et lumières
│   ├── effects.py           effets du relief
│   ├── particles_render.py  particules et traces en profondeur
│   ├── hud.py               compteurs, mini-carte et tableaux des stands
│   ├── menu.py              sélection de véhicule et de couleur
│   └── compositor.py        tri de profondeur, isolation des rendus
├── config.py, track.py, state.py, physics.py, ...
│                           bibliothèque du moteur V4, toujours disponible
└── render/                  composants graphiques réutilisés du moteur précédent
karting_v6.py                archive jouable de la V6 monolithique
karting_v3.py                prototype d'origine
tests/                       validation du moteur V4 et V6
```

**Contrats :** les modules `track`, `physics`, `weather`, `session`, `mesh` et `scenery` n'importent pas Pygame. La boucle de jeu ne calcule ni trajectoire ni géométrie. Les rendus reçoivent explicitement l'écran, la caméra et l'état nécessaire.

Le mécanisme de gestion des fautes existant est réutilisé pour isoler les rendus facultatifs, la sauvegarde et la lecture des commandes. La branche `test` reste le laboratoire distinct d'injection de pannes ; la V6 modulaire de `dev` ne déclenche pas de pannes artificielles.

## Tests

```powershell
python -m compileall -q main.py karting_v6.py karting tests
python -m unittest discover -s tests -v
```

Le workflow GitHub Actions `.github/workflows/dev-tests.yml` est prévu pour exécuter la suite à chaque push sur `dev`, sous SDL fictif pour la fenêtre Pygame.

**Limite :** l'architecture a été refactorisée, mais il faut confirmer sur un PC réel que tous les véhicules, éclairages, effets météo et FPS restent conformes à la V6 monolithique. L'ancienne version reste disponible pour comparer, et tout écart pourra être corrigé dans le composant concerné plutôt que dans un fichier unique.
