# 🏎️ Karting, version native C++20 / raylib

Le jeu est développé en **C++20 avec raylib**, dans le dossier [`cpp/`](cpp/). Les branches `main-cpp` (version validée) et `cpp-raylib` (développement) contiennent uniquement cette implémentation. Les anciennes sources Python ont été retirées de ces branches ; leur historique Git et les autres branches restent intacts.

## Lancer sous Windows

Depuis la racine du dépôt, après avoir installé CMake et un compilateur C++20 :

```powershell
cmake -S cpp -B cpp/build-ucrt -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build-ucrt --parallel
ctest --test-dir cpp/build-ucrt --output-on-failure
.\cpp\build-ucrt\karting_cpp.exe
```

Si le dossier `cpp/build-ucrt` existe déjà, la commande de configuration est facultative. La configuration doit être exécutée avec le compilateur et l'environnement appropriés.

## Commandes

| Action | Touche |
| --- | --- |
| Accélérer | `↑`, `Z`, `W` |
| Freiner | `↓`, `S` |
| Direction | `←` / `→`, `Q` / `D` |
| Frein à main | `Espace` pendant la course |
| Choix du véhicule | Flèches au menu |
| Démarrer | `Entrée` ou `Espace` au menu |
| Menu | `M` (compatible AZERTY) |
| Réinitialiser | `R` |
| Qualité graphique | `G` |
| Cycle jour/nuit | `N` |
| Pluie | `P` |

## Organisation

- `cpp/src/` : simulation, rendu 3D, modèles, effets et interface.
- `cpp/include/karting/` : interfaces C++ et filtrage de visibilité.
- `cpp/tests/` : tests de logique et de simulation.
- `cpp/README.md` : architecture, fonctionnalités et compilation détaillées.

Les améliorations incluent les quatre catégories de véhicules, un tracé à quatre secteurs, la gestion du champ de vision de la caméra, des tribunes optimisées et le ciel dynamique.

Pour plus de détails : **[Guide complet C++](cpp/README.md)**.
