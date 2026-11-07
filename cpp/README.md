# Karting V6 en C++20 + raylib

Ce dossier contient la version native **C++20 / raylib** de Karting, utilisée sur les branches `cpp-raylib` et `main-cpp`. Les anciens fichiers du jeu Python ont été retirés de ces deux branches ; ils restent consultables dans l'historique Git et les branches historiques.

## Viaduc moderne (barrières et talus volumiques)

Le pont conserve un tablier clair relativement fin, des piles rectangulaires
discrètes et un dégagement complet pour la route inférieure. Les barrières
ont été **abaissées de 84 à 48 cm**, avec désormais **une épaisseur réelle** :
socle en béton, poutre métallique fermée, faces latérales et capuchons,
montants et réflecteurs. Leur placement correspond au seuil de collision
déjà utilisé par la physique, sans déplacer les limites de conduite.

Les talus ne sont plus deux plaques verdâtres ouvertes. Les rampes sont
des **volumes de remblais fermés** : sections transversales en neuf points,
accotement herbeux, terre compactée, roche et pieds enterrés dans le terrain.
Les extrémités et le dessous sont fermés pour ne plus voir l'intérieur vide.
De légères irrégularités et quelques pierres apparentes cassent la monotonie
des grandes surfaces. Le tablier est élargi jusqu'aux barrières, muni
d'une sous-face et de joues latérales, sans combler le passage inférieur.

Cette révision est **graphique et géométrique** : aucun changement dans les
45 points de contrôle, les pentes physiques, les collisions, les records ou
la gravité. Le rendu est limité par la distance et la qualité graphique.
Le comportement en jeu et les FPS doivent être vérifiés sur Windows.

**Gestion des branches :** `main-cpp` a été synchronisée avec
`cpp-raylib` **avant** ce travail ; les nouveaux ajustements du pont sont
publiés uniquement sur `cpp-raylib` pour validation.

## Raccord terrain–chaussée

La chaussée est calculée avec le relief exact **sous chaque sommet de route**
(centre, bords et accotements), auquel s'ajoute seulement la surélévation
progressive du pont. Cela évite qu'un bord d'asphalte soit enterré dans
la colline tandis que l'autre flotte au-dessus du sol. Les portions
ordinaires restent posées sur le terrain ; les rampes basses possèdent
des talus latéraux et la travée haute garde son dégagement.

Ce correctif ne change ni le tracé en XY, ni les hauteurs de la ligne
centrale, ni les règles de conduite. Les collisions des deux niveaux
restent distinctes. Le rendu et les FPS doivent encore être vérifiés
sur la machine de jeu.

## Premier pont à deux niveaux

Sur `cpp-raylib`, l'épingle orientale tourne désormais vers le nord et
sa sortie recroise **une seule fois** la ligne droite d'approche. Le pont
passe au-dessus de celle-ci avec environ **8,5 mètres** de séparation
verticale. Les rampes suivent des profils lissés de part et d'autre.
Le parcours conserve quatre secteurs, les quatre véhicules et les réglages
de commande.

La voiture conserve une altitude et un indice de voie courante.
La recherche de chaussée, la collision et le chronométrage distinguent
les deux niveaux au croisement. Le renderer dessine un vrai tablier
avec un tablier clair et deux appuis discrets hors de la chaussée inférieure,
ainsi que ses barrières à la bonne hauteur. Le décor naturel ne suit pas
la hauteur du tablier : il reste au niveau du terrain.

Le pont est une expérimentation sur la branche de développement.
**Compilation et rendu réel restent à valider**, notamment la continuité
de trajectoire, le passage dessous et les performances. Ne pas fusionner
dans `main-cpp` avant validation.

## Adhérence visuelle de la voiture aux pentes

La position et l'inclinaison du véhicule sont calculées depuis quatre
échantillons du terrain sous les roues (essieux avant/arrière, gauche/droite).
Le plan obtenu donne un tangage longitudinal, un roulis transversal et
une hauteur d'appui cohérents. Le rendu applique ce plan à **l'ensemble
de la voiture, roues comprises**. Les mouvements de suspension de la
carrosserie s'y ajoutent sans annuler l'inclinaison de la route.

Le véhicule présenté dans le menu reste horizontal sur sa plateforme.
Aucune modification des points du tracé, de la vitesse, des collisions
ou des réglages d'adhérence. La qualité visuelle et les FPS doivent être
confirmés par un essai du jeu.

## Relief renforcé du circuit (sans modifier le tracé)

La fonction de relief continu emploie désormais des ondulations de grande
longueur d'onde, plus amples. Sur le tour actuel, le dénivelé est d'environ
**15 m en unités de rendu**, avec des pentes dont le maximum échantillonné
reste inférieur à **20 %**. Les 42 points de contrôle en XY, la largeur,
les virages, les commandes et les profils des véhicules sont inchangés.

La chaussée reste au contact du terrain naturel : c'est une phase de relief
général, pas encore un pont ni une route sur pilotis. La caméra, la voiture,
les arbres et les autres éléments de scène utilisent la même fonction de
relief. Le plan de terrain lointain a été abaissé pour éviter de masquer
les nouvelles vallées. L'effet sur la conduite et les FPS doit être validé
dans le jeu.

## Architecture de la carte : terrain et chaussée distincts

La représentation conserve **exactement le tracé actuel**. La carte expose
désormais deux altitudes indépendantes :

- `Track::terrainHeight(position)` : hauteur du sol selon la position XY.
- `Track::roadHeight(index)` : hauteur du tablier à un échantillon donné
  de la route, égale au terrain par défaut.
- `Track::setRoadHeightProfile(...)` : permet de fournir un profil de hauteur
  spécifique à la route, après validation du nombre d'échantillons et des valeurs.
- `Track::infoAtHeight(position, altitude, hint)` : recherche la portion de
  chaussée en tenant compte de la distance en trois dimensions, avec un indice
  optionnel de continuité pour départager les égalités.

Cette API prépare les routes superposées, rampes et ponts. **Elle ne modifie
pas encore** la boucle de conduite, les collisions ou le rendu de la route :
ceux-ci utilisent toujours les méthodes historiques, afin de garantir un
résultat visuel et comportemental inchangé. Le tracé ne comporte actuellement
ni pont ni croisement. Une intégration verticale complète nécessitera ensuite
de brancher le rendu, la voiture, le suivi de course et les collisions sur
ces altitudes et indices de chaussée.

## Démarrer sous Windows (PowerShell)

Pré-requis : **Git, CMake >= 3.21**, un compilateur C++20 (Visual Studio avec « Développement Desktop en C++ » ou MinGW-w64) et une connexion Internet au premier configure. CMake télécharge automatiquement **raylib 5.5** si elle n'est pas déjà installée.

Le plus simple est d'exécuter le script de compilation et de lancement :

```powershell
powershell -ExecutionPolicy Bypass -File cpp/build_windows.ps1
```

Il conserve les messages d'erreur de CMake et du compilateur, ce qui facilite le diagnostic.

Depuis la racine du dépôt, la méthode manuelle équivalente :

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

Les tests C++ valident les quatre profils de voitures, le grand circuit interpolé, l'accélération, l'adhérence sous pluie, le frein à main, les collisions, le chrono et la cadence physique indépendante des FPS.

## Modèles 3D remaniés (silhouettes lisibles)

Les quatre voitures disposent désormais d'une géométrie procédurale dédiée,
dans `cpp/src/vehicle_models.cpp`. Aucune dépendance Blender, aucun modèle
externe et aucune modification des commandes ni du suivi de caméra.

- **Kart** : petit châssis effilé, pare-chocs, pilote, siège et moteur visible.
- **4x4** : carrosserie haute à capot effilé, vitrages inclinés, galerie et roue de secours.
- **GT3** : long capot, pavillon bas, ailes profilées, prises d'air et **aileron mince et bas**.
- **WEC** : nez de prototype, pontons séparés, cockpit bas, petite dérive et **aileron arrière fin**, dimensionné pour limiter l'obstruction de la vue arrière.

Les roues ont une géométrie distincte selon le véhicule, des jantes visibles
et une animation liée à la rotation physique. Les hauteurs de caisse restent
suffisamment basses pour conserver le cadrage de référence. Le résultat
visuel reste à valider en jeu avant toute nouvelle retouche.

## Calibration vitesse et sensations

La vitesse affichée et le déplacement 3D utilisent maintenant la même
conversion physique : `toKmH(car.speed) = car.speed × 0,3`.
Le facteur `kTravelScale = 0,3/(3,6 × kWorldScale)` est appliqué
**seulement à la translation**, de sorte que le compteur 300 km/h signifie
environ 83,3 mètres 3D par seconde. Les rapports de transmission n'existent
toujours pas. La loi de rotation du volant, les commandes et les profils de
puissance restent inchangés ; le rayon de virage à haute vitesse devient plus
grand, ce qui nécessite de freiner davantage.

Les indices visuels sont renforcés sans déplacer la caméra latéralement :
- petits réflecteurs de bord de piste tous les ~25-30 mètres ;
- champ de vision élargi progressivement de 64 à 78 degrés ;
- quelques filets de vitesse discrets à la périphérie, seulement à haute
  vitesse et en qualité graphique au moins « élevée ».

Le test `testSpeedDistanceCalibration()` vérifie la correspondance entre
la vitesse affichée et la distance parcourue. Il faut tester la maniabilité
et les collisions sur le PC réel, surtout en virage à haute vitesse.

## Parcours plus vivant

Des éléments légers et contextualisés rendent les secteurs plus identifiables :
postes de commissaires avec deux silhouettes et drapeaux animés, panneaux de
freinage en approche des virages, chevrons directionnels et piles de pneus dans
les zones de dégagement. Quatre secteurs disposent de repères spécifiques,
complétés par des postes de commissaires régulièrement espacés.

Leur placement est lié à la progression sur le tour (pas au numéro de sommet).
Les nouveaux objets profitent du filtrage par distance et du frustum culling
déjà présent, et ne modifient ni le tracé ni les collisions. Les drapeaux
s'animent uniquement lorsque leur poste est effectivement rendu.

## Intégration sélective de la branche de Nathan

La version reprend les fonctionnalités retenues de `cpp-raylibb-nathan` :

- **Touche M / AZERTY** : lecture du caractère saisi en complément de la touche physique.
- **Ciel** : nuages dynamiques projetés selon l'orientation et le champ de vision
  de la caméra. Les montagnes artificielles ont été retirées après essai.
- **WEC** : carrosserie Hypercar révisée, sans modifier la GT3 ni les roues.
- **Effets GT3** : retour de flamme bref lors du lever de pied, déclenché par
  la variation de l'accélérateur. Étincelles peu nombreuses et plus visibles
  à proximité des roues arrière pendant un freinage appuyé.

**Non repris** : boost, BMW M4 GT3 de Nathan, ancien tracé et ancien décor.
Les modifications ne changent ni la physique ni le Frustum Culling.
Les FPS et le résultat visuel des effets doivent encore être validés en jeu.

## Frustum culling (champ visible de la caméra)

Le rendu ignore désormais les **objets de décor**, les **sections de
route**, les **réflecteurs** et les **dalles de terrain** situés hors du
champ de la caméra. Un test de sphère englobante avec marge conserve les
objets partiellement visibles aux bords de l'écran. Le filtrage par
distance et les modèles à faible niveau de détail restent actifs. Le
calcul est effectué une fois par image à partir de la caméra utilisée
par `BeginMode3D`, sans changer sa position ni ses commandes.

Cette optimisation économise des appels de dessin, mais ne retire pas
les objets occultés par les reliefs : il ne s'agit pas d'occlusion culling.

## Optimisation du rendu du décor

Le rendu conserve les forêts, tribunes et paddocks, mais réduit les opérations
répétitives par image :

- Le maillage du terrain est envoyé par lots `rlgl` au lieu de milliers
  d'appels séparés à `DrawTriangle3D`. Les hauteurs des sommets sont
  calculées une seule fois pour chaque sommet partagé.
- La position 3D des éléments statiques est calculée à la génération.
  Aucun appel de calcul de relief n'est nécessaire pour les replacer
  à chaque frame.
- Les arbres et les tribunes possèdent un niveau de détail éloigné simplifié.
  Les modèles proches restent détaillés, sans réduire la densité des forêts.
- Les limites d'affichage en fonction de la qualité restent inchangées.

Ces changements visent la chute de performances observée après
l'augmentation de densité du décor. **Les FPS réels doivent être mesurés
sur la machine de jeu** ; aucun chiffre n'est garanti avant essai.

## Grand circuit et environnement enrichi

Le circuit utilise **42 points de contrôle** pour un tour d'environ
**71 200 unités**. L'**épingle à 180°** et sa ligne droite d'entrée sont
strictement préservées. Le reste du tour comporte désormais quatre secteurs
reconnaissables : ligne droite et épingle, S rapides à l'est, complexe à
double apex au sud avec relances, et virages techniques à l'ouest avant
le retour sur la ligne de départ. La courbe est fermée, sans intersection,
et la mini-carte s'adapte automatiquement.

La largeur totale d'asphalte passe de **300 à 350 unités** (+16,7 %).
Les barrières ont été déplacées de 242 à 275 unités du centre pour
préserver la marge de sécurité. Les bordures, les stands et les collisions
utilisent ces nouvelles dimensions. Les réglages de puissance, la caméra
et les commandes validés dans la version précédente sont inchangés.

Après calibration des distances en mètres 3D, le premier réglage moteur
s'est révélé trop agressif. Les moteurs ont donc été **fortement ralentis** :
la puissance a été réduite d'environ 60 % et la montée des gaz est plus
progressive. Les plafonds théoriques affichés sont désormais :

| Véhicule | Pointe théorique | Puissance moteur (unités du jeu) |
| --- | ---: | ---: |
| Kart | 90 km/h | 320 |
| 4x4 | 96 km/h | 280 |
| GT3 | 120 km/h | 390 |
| WEC | 135 km/h | 460 |

Le déplacement 3D reste calibré sur les km/h réels, sans modifier la
direction, les commandes, les modèles ou le décor. Le comportement doit être
validé en course, particulièrement dans les virages serrés.
Aucune boîte de vitesses n'est introduite.

Le décor est construit dans `src/scenery.cpp` et rendu dans
`src/render.cpp` :

- **Terrain 3D réel** : maillage adapté à la caméra dont chaque sommet utilise
  la même fonction de relief que la piste ; couleurs de sol nuancées.
  Le sol plat de l'ancienne version est repoussé en arrière-plan.
- **Forêts denses** : feuillus ramifiés, conifères à plusieurs étages,
  broussailles et groupes rocheux, répartis près de la route.
- **Secteurs fonctionnels** : bâtiments de paddock et tentes autour du
  départ/stands ; tribunes et signalisation sur des secteurs spectateurs ;
  forêt et rochers dans les autres portions de la piste.
- **Vraies structures visuelles** : portes de garages et vitrages, tribunes
  étagées avec sièges/public, toits de tentes, lampadaires et éoliennes.

Le placement est **déterministe** et exprimé en unités de **distance
parcourue**, pas en numéros de points : l'ajout d'un virage ou un changement
du nombre de points interpolés ne font plus disparaître la forêt. Les zones
de paddock, tribunes et rochers sont définies en fractions du tour, et cinq
rangées de végétation par côté densifient les secteurs boisés. Les objets
sont orientés selon la tangente de la route, posés sur le terrain, tenus à
distance des autres branches du circuit, puis filtrés par distance et qualité
graphique avant affichage. Les véhicules, la caméra et les commandes restent
inchangés.

Ce travail reste **procédural et stylisé** : pas de textures PBR, de shaders
d'éclairage physique ni d'ombres dynamiques. Un essai en conditions réelles
est nécessaire pour valider les visuels et les 60 FPS.

Les essais graphiques et la mesure des FPS sur le PC restent nécessaires.

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
    core.cpp                   42 points, Catmull-Rom, 4 profils, physique, météo, tours
    render.cpp                 scène 3D, route, vibreurs, murs, arbres
    vehicle_models.cpp         modèles procéduraux des quatre voitures
    scenery.cpp                décors, tribunes, bâtiments, végétation et culling
    effects.cpp                fumée, étincelles, traces, flammes
    ui.cpp                     menu, compteurs, mini-carte, stands
    game.cpp                   boucle raylib, caméra, entrées, sauvegarde
    main.cpp                   point d'entrée
  tests/core_tests.cpp         tests autonomes sans bibliothèque graphique
```

Le **moteur logique ne contient aucun include raylib**. Le rendu dessine des objets 3D par GPU ; il ne dépend pas de Pygame. Les sous-systèmes visuels peuvent être désactivés indépendamment en cas d'exception.

## État de migration

Portés depuis la V6 Python :
- Les 42 points de contrôle du circuit à quatre secteurs et leur interpolation fermée
- Les quatre voitures (Kart / 4x4 / GT3 / WEC) avec leurs performances récentes
- L'accélération, le freinage, le grip avant/arrière, la dérive avec frein à main, l'appui, la pluie et le relief
- Les rails, le ralentissement naturel dans l'herbe sans pénalité chronométrique, la voie des stands, le chrono, les records et leur sauvegarde
- La sélection du véhicule et de la couleur, les trois qualités, le jour/crépuscule/nuit
- Un rendu 3D GPU avec arbres, garages, véhicules distincts et particules

**À affiner par rapport à la V6 Python :** les carrosseries sont pour l'instant des formes 3D simplifiées plutôt que les meshes originaux polygone par polygone ; les tribunes détaillées, les panneaux publicitaires, l'éclairage sophistiqué et les effets météo avancés doivent encore être reproduits visuellement. Le multijoueur et les IA adverses n'existaient pas encore dans la V6 et ne sont pas ajoutés artificiellement.

Ce port est une **première version native jouable à valider**, pas une certification de fidélité graphique à l'ancienne V6. Le workflow GitHub Actions vérifie la compilation et les tests à chaque modification de cette branche.
