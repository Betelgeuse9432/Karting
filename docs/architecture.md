# Architecture cible de Karting

> Statut : conception validée comme cible, migration non encore réalisée. Le prototype `karting_v3.py` reste exécutable et inchangé pendant la transition.

## Objectifs

1. Séparer les calculs du jeu du rendu Pygame et des entrées utilisateur.
2. Rendre chaque sous-système testable isolément, avec dépendances explicites.
3. Conserver la physique, le circuit, les contrôles et les graphismes de la V3 pendant la migration.
4. Détecter les défaillances, signaler leur cause et dégrader uniquement les fonctions optionnelles lorsque c'est possible.
5. Éviter les états partagés et les effets secondaires au moment de l'import.

## Structure proposée

```text
main.py                        # démarrage, erreurs fatales, fermeture propre
karting_v3.py                  # référence V3, inchangée pendant la migration
karting/
    __init__.py
    config.py                  # paramètres validés, objets de configuration
    state.py                   # VehicleState, RaceState, GameState, Controls
    input.py                   # traduction clavier -> Controls
    track.py                   # courbes, normales, piste, stands, garages, arbres
    physics.py                 # intégration dynamique, adhérence, glisse
    race.py                    # progression, temps au tour, records
    persistence.py             # lecture/écriture des records avec diagnostics
    effects.py                 # particules et traces, système facultatif
    camera.py                  # transformations et projection 3D
    render/
        __init__.py
        world.py               # ciel, terrain, piste, arbres, stands
        vehicle.py             # représentation 3D du kart
        hud.py                 # chronomètre, minimap, instruments, alertes
    game.py                    # boucle et coordination des modules
    faults.py                  # état des composants, journal et désactivation
tests/
    test_track.py
    test_physics.py
    test_race.py
    test_persistence.py
    test_failures.py
requirements.txt
docs/architecture.md
```

## Contrats et dépendances

- `main` crée `Game` et affiche une erreur exploitable si l'initialisation échoue.
- `Game` instancie les composants dans l'ordre : config -> piste -> état -> simulation -> caméra -> rendu. Il est le seul coordinateur.
- `Track` possède ses points, normales, géométries, stands et décors. Les autres modules l'interrogent via des méthodes (`nearest`, `surface_at`, `position_at`) et ne modifient pas sa géométrie.
- `Input` renvoie un `Controls` indépendant de Pygame ; seul le module d'entrée interprète les touches.
- `Physics.step(vehicle, controls, track, dt)` modifie uniquement l'état de simulation et renvoie une télémétrie (glisse, freinage, surface, etc.). Il n'importe pas Pygame.
- `Race.step(race, vehicle, track, dt)` suit la progression et émet les événements de tours ; l'enregistrement est délégué à `Persistence`.
- `Effects.step(vehicle, telemetry, dt)` gère fumée, flammes, étincelles et marques ; il n'influence jamais la physique.
- `Camera` projette les coordonnées sans accéder à l'état global de la partie.
- `Renderer.draw(snapshot)` lit un état cohérent, mais ne modifie ni la vitesse, ni le chrono, ni les records.
- `HUD` affiche les valeurs reçues et les erreurs signalées par le gestionnaire de pannes.

```text
Clavier -> Input -> Controls -> Physics -> VehicleState
                              |            |
                              v            +------> Camera -> Renderer -> Écran
                             Track         |                      ^
                              |            +------> Effects -------+
                              +----------> Race -> Persistence -> Record
                                                |
                                                +-----------------> HUD
Game supervise les appels et les pannes ; le flux ne remonte pas du rendu vers la physique.
```

## Stratégie de robustesse

Trois catégories de composants sont distinguées :

1. **Indispensables** : piste, état, physique et boucle d'événements. Si un état devient non fini ou si une mise à jour échoue, enregistrer l'exception, restaurer un état valide si possible, puis arrêter proprement après des échecs répétés ; ne jamais continuer avec une simulation corrompue.
2. **Dégradables** : particules, arbres, traces, minimap, décoration des garages. Une exception est journalisée avec traceback ; le composant peut être désactivé pour le reste de la session. Le kart continue à rouler.
3. **Persistance** : records absents, illisibles ou non inscriptibles. Le jeu continue ; un avertissement est affiché, le record reste disponible en mémoire.

Une tentative de réinitialisation contrôlée d'un composant désactivé peut être proposée. **Auto-réparation** signifie ici : détection, isolation, reconfiguration et réessai contrôlés ; pas modification automatique arbitraire du code source.

Règles : journaliser une panne au lieu de `except Exception: pass` ; ne pas attraper silencieusement les exceptions ; ne pas réessayer indéfiniment à chaque image ; fermer Pygame dans un bloc `finally`.

## État et temps

- Utiliser des dataclasses typées à la place du dictionnaire global `etat` et d'autres variables globales mutables.
- Paramètres immuables et validés au démarrage : dimensions positives, vitesse max positive, empattement non nul, circuit avec suffisamment de points.
- Une seule responsabilité pour chaque donnée : `VehicleState` (véhicule), `RaceState` (chronométrage), `EffectsState` (effets).
- Garder une simulation déterministe sous pas de temps borné, indépendante autant que possible du FPS ; envisager ensuite un pas fixe avec accumulateur et rendu découplé.
- Préserver les données de référence de la V3, y compris le parcours et les coefficients physiques, tant qu'une modification n'est pas explicitement décidée.
- `record_karting.txt` est une donnée locale et doit rester hors Git ; documenter le chemin de stockage.

## Tests et critères d'acceptation

- `python -m unittest discover -s tests -v` pour les tests unitaires sans ouverture de fenêtre.
- Création de la piste, cohérence des normales, détection route/herbe/stands.
- Accélération, frein, virage, sous-virage/survirage et bornes de vitesse à entrées contrôlées.
- Progression des tours et record, y compris fichier manquant/corrompu/non inscriptible.
- Injection d'une panne dans les effets/HUD : le jeu reste simulable, panne visible et tracée.
- Injection d'un état physique invalide : restauration ou arrêt explicite, jamais état silencieusement corrompu.
- Tests manuels : commandes ZQSD/WASD/flèches, remise à zéro R, ESC, rendu et FPS, voie des stands et scores.
- `karting_v3.py` est conservé comme référence pendant toute la migration ; `main.py` devient l'entrée recommandée seulement après une vérification de parité fonctionnelle.

## Séquence de migration

1. **Base** : documentation, dépendance Pygame et exclusion des fichiers locaux.
2. **Cœur sans Pygame** : configuration, piste, état, physique, course, persistance ; tests unitaires.
3. **Affichage** : caméra, rendu 3D, décor, kart, HUD, particules ; comparaison avec la V3.
4. **Orchestration** : boucle `Game`, entrée `main.py`, gestion explicite des défauts.
5. **Validation** : tests, exécution manuelle et comparaison comportementale avant toute suppression du prototype.

## Limites connues du prototype

- L'import de `karting_v3.py` initialise immédiatement Pygame et entre dans la boucle principale.
- `etat`, `ecran`, `camx`, `camy`, `cs`, `sn`, `centre` et plusieurs collections sont globaux et mutables.
- `plus_proche` balaie toute la courbe à chaque appel : acceptable aujourd'hui, mais point de coût à profiler avant toute optimisation.
- Les erreurs de lecture/écriture du record sont actuellement ignorées.
- Une exception de rendu peut interrompre la partie entière.
- La V3 compile, mais cela ne remplace ni un test d'exécution ni la validation des entrées/graphismes.
