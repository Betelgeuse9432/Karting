# Karting V6 en C++20 + raylib

Cette branche **cpp-raylib** dérive de **dev**. Le jeu natif est dans cpp/, et le code Python d'origine reste en lecture pour faciliter les comparaisons. Il ne s'exécute pas depuis le binaire C++.

## Démarrer sous Windows (PowerShell)

Pré-requis : **Git, CMake >= 3.21**, un compilateur C++20 (Visual Studio avec « Développement Desktop en C++ » ou MinGW-w64) et une connexion Internet au premier configure. CMake télécharge automatiquement **raylib 5.5** si elle n'est pas déjà installée.

Depuis la racine du dépôt :

```powershell
git fetch origin
git switch --track origin/cpp-raylib
cmake -S cpp -B cpp/build
cmake --build cpp/build --config Release --parallel
.\cpp\build\Release\karting_cpp.exe
```

Avec Ninja ou MinGW, le binaire se trouvera généralement dans cpp/build/karting_cpp.exe (sans sous-dossier Release). Ne pas utiliser la commande Windows pour exécuter un binaire Linux.

Sous Linux/macOS :

```bash
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build --parallel
./cpp/build/karting_cpp
```

### Exécuter les tests sans installer raylib

```powershell
cmake -S cpp -B cpp/build-core -DKARTING_BUILD_GAME=OFF
cmake --build cpp/build-core --config Release
ctest --test-dir cpp/build-core -C Release --output-on-failure
```

Les tests C++ valident les quatre profils de voitures, le circuit V6 interpolé, l'accélération, l'adhérence sous pluie, le frein à main, les collisions, le chrono et la cadence physique indépendante des FPS.

## Commandes

| Action | Touche |
| --- | --- |
| Avancer | Flèche haut / Z / W |
| Freiner et reculer | Flèche bas / S |
| Tourner | Flèches gauche/droite / Q / A / D |
| Frein à main | Espace (pendant la course) |
| Changer de voiture | Gauche/droite dans le menu |
| Changer de couleur | Haut/bas dans le menu |
| Commencer | Entrée / Espace dans le menu |
| Jour / crépuscule / nuit | N |
| Pluie | P |
| Qualité ÉCO / ÉLEVÉ / ULTRA | G |
| Réinitialiser | R |
| Revenir au menu | M |
| Quitter | Échap |

## Architecture C++

```text
cpp/
  CMakeLists.txt               2 cibles : moteur C++ pur + jeu raylib
  include/karting/
    core.hpp                   interfaces simulation, piste, physique, course
    game.hpp                   interfaces graphique, effets et orchestration
  src/
    core.cpp                   26 points, Catmull-Rom, 4 profils, physique, météo, tours
    render.cpp                 scène 3D, route, vibreurs, murs, arbres, véhicules
    effects.cpp                fumée, étincelles, traces, flammes
    ui.cpp                     menu, compteurs, mini-carte, stands
    game.cpp                   boucle raylib, caméra, entrées, sauvegarde
    main.cpp                   point d'entrée
  tests/core_tests.cpp         tests autonomes sans bibliothèque graphique
```

Le **moteur logique ne contient aucun include raylib**. Le rendu dessine des objets 3D par GPU ; il n'utilise pas le moteur logiciel Pygame de l'ancienne version. Les sous-systèmes visuels peuvent être désactivés indépendamment en cas d'exception.

## État de migration

Portés depuis la V6 Python :
- Les 26 points de contrôle du circuit et leur interpolation fermée
- Les quatre voitures (Kart / 4x4 / GT3 / WEC) avec leurs performances récentes
- L'accélération, le freinage, le grip avant/arrière, la dérive avec frein à main, l'appui, la pluie et le relief
- Les rails, les pénalités si quatre roues hors piste, la voie des stands, le chrono, les records et leur sauvegarde
- La sélection du véhicule et de la couleur, les trois qualités, le jour/crépuscule/nuit
- Un rendu 3D GPU avec arbres, garages, véhicules distincts et particules

**À affiner par rapport à la V6 Python :** les carrosseries sont pour l'instant des formes 3D simplifiées plutôt que les meshes originaux polygone par polygone ; les tribunes détaillées, les panneaux publicitaires, l'éclairage sophistiqué et les effets météo avancés doivent encore être reproduits visuellement. Le multijoueur et les IA adverses n'existaient pas encore dans la V6 et ne sont pas ajoutés artificiellement.

Ce port est une **première version native jouable à valider**, pas une certification de fidélité graphique à l'ancienne V6. Le workflow GitHub Actions vérifie la compilation et les tests à chaque modification de cette branche.
