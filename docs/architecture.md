# Architecture V4 du projet Karting

## État actuel

La V4 est une **refonte effective** du prototype monolithique `karting_v3.py`. L'ancien fichier est conservé intact. Le cœur logique est testé sans Pygame. Le rendu Pygame est implémenté et a fait l'objet d'une simulation d'appels, mais pas encore d'une inspection graphique réelle dans cette session.

## Flux des données

```text
Pygame clavier -> karting.input -> Controls
                                       |
                              VehiclePhysics.step
                                |            |
                               Track     VehicleState
                                |            |
                                +----> RaceTracker --> RecordStore
                                             |
                               Effects <-----+----- Telemetry
                                  |                  |
                                  v                  v
                               Camera <--------- VehicleState
                                  |
                        SceneCompositor + HudRenderer
                                  |
                             Pygame écran
```

`Game` compose et coordonne les services. Les données vont du moteur de simulation vers l'affichage, jamais l'inverse. `Track`, `VehiclePhysics`, `RaceTracker`, `Effects`, `Camera`, `RecordStore` sont utilisables sans ouvrir de fenêtre.

## Responsabilités et contrats

| Module | Responsabilité | Dépendances externes |
| --- | --- | --- |
| `config` | Paramètres et validation | standard Python |
| `state` | Objets de données | standard Python |
| `track` | Courbe, normales, stands, recherche du point le plus proche | `config` |
| `scenery` | Arbres déterministes, indépendants du circuit | `track`, `random.Random` local |
| `physics` | Accélération, freinage, adhérence, mouvement | `config`, `state`, `track` |
| `race` | Progression des quatre secteurs, records en mémoire | `state`, `track` |
| `persistence` | Lecture/écriture de record | système de fichiers |
| `camera` | Position caméra, perspective et écrêtage proche | `config`, `state` |
| `effects` | Particules, traces de pneus et génération d'événements visuels | `state` |
| `input` | Adaptation du clavier | Pygame |
| `render/*` | Affichage et composition des couches | Pygame, données en lecture |
| `faults` | Diagnostic, désactivation des composants optionnels, réessai | `logging` |
| `game` | Boucle, création et ordre d'exécution des services | modules ci-dessus |

## Robustesse

### Indispensables

La piste, la physique, le chronométrage, la boucle Pygame. Une exception pendant une mise à jour physique restaure l'état précédent et **met la simulation en pause**. Une exception du chronométrage restaure l'état de course précédent, et met également la simulation en pause. `R` réinitialise la partie. Une erreur Pygame empêchant le programme d'ouvrir ou de fermer la fenêtre reste une erreur fatale signalée par `main.py`.

### Dégradables

Les effets, les décors, les segments de rendu et la persistance peuvent échouer sans arrêter la course. `FaultManager` journalise l'exception complète la première fois puis désactive ce composant. Les avertissements visibles apparaissent dans le HUD lorsque celui-ci fonctionne. `R` peut réessayer les composants désactivés.

La gestion de défauts ne garantit **pas** la récupération de n'importe quelle exception : mémoire épuisée, pilote graphique indisponible et état matériel défaillant peuvent nécessiter un arrêt propre.

### Persistance

Absence du fichier record : pas de record. Fichier non interprétable : avertissement, départ sans meilleur temps enregistré. Écriture par fichier temporaire + remplacement atomique dans le même dossier pour réduire le risque de corruption. En cas de refus d'écriture, le record reste en mémoire.

## Invariants

- Aucune initialisation Pygame au moment de l'import des modules logiques.
- Aucune collection mutable globale de véhicules ou d'effets.
- `GameConfig` est immuable et validé avant démarrage.
- Les `VehicleState` doivent toujours contenir des nombres finis.
- Les fonctions de rendu n'ajustent pas la physique ou les tours.
- Les effets ne modifient pas `VehicleState`.
- La V3 reste exécutable et non modifiée.

## Conservation des comportements

Le circuit (18 points originaux, interpolation de Catmull-Rom et 45 subdivisions), les coefficients de physique et les seuils d'adhérence, les stands, les contrôles et le modèle de projection proviennent de la V3. La simulation a été comparée avec le code V3 sur 120 images successives en accélérant, tournant et freinant : **valeurs physiques identiques à une tolérance de flottants près**.

Les générateurs de nombres aléatoires sont désormais propres à chaque système. Le décor reste déterministe avec la graine `7`, mais la succession exacte des effets graphiques peut différer de l'ancien prototype.

## Tests et prochaines vérifications

```sh
python -m unittest discover -s tests -v
python main.py
```

- Tests automatiques : circuit, gaz, frein, marche arrière, limites de vitesse, état non fini, chronométrage, record, caméra, particules, défaillances.
- Tests manuels nécessaires sous Windows : rendu réel Pygame, contrôle clavier, fluidité, visibilité de la piste, décors, voie des stands, HUD, particules et sauvegarde.
- Optimisations ultérieures : recherche géométrique des segments les plus proches, pré-culling par secteur et séparation simulation à pas fixe/rendu interpolé. Ne pas optimiser sans profiler.

## Campagne d'injection sur la branche `test`

Le module `fault_injection` est branché sur les frontières d'appel des composants. `FaultManager` accepte un crochet `before_call` facultatif ; sans injection, la gestion des erreurs reste identique. Les opérations critiques (`physics`, `race`) ont leur point d'injection dans `Game.step`, à l'intérieur des protections déjà existantes. Le calendrier des pannes est indépendant de la physique et des effets visuels.

Les exceptions artificielles se produisent **avant** les effets de bord et vérifient donc l'isolation en cas de panne d'entrée de composant. La corruption interne après mise à jour partielle n'est pas testée par ce dispositif. Voir [fault-testing.md](fault-testing.md).
