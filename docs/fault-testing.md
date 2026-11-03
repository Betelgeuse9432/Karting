# 🧪 Campagne de pannes aléatoires (`test`)

Cette branche permet de tester la **résilience** de Karting V4 en injectant des exceptions intentionnelles dans différents composants en cours de partie. Le code du jeu sur `dev` et `main` ne change pas.

## Lancer

```powershell
python -m pip install -r requirements.txt
python main.py
```

Le sabotage est **actif par défaut uniquement sur la branche `test`**. Un composant est choisi au hasard après un délai aléatoire compris entre **2 et 5 secondes** de simulation. Après chaque panne déclenchée, un nouveau délai est tiré. Une seule panne est armée à la fois.

```powershell
python main.py --seed 42                  # suite reproductible avec les mêmes pas de temps
python main.py --min-interval 1 --max-interval 3
python main.py --no-essential            # seulement les modules facultatifs
python main.py --safe                    # jouer sans sabotage
python -m unittest discover -s tests -v
```

## Contrôles de test

| Touche | Action |
| --- | --- |
| `F9` | Suspendre ou reprendre les pannes aléatoires |
| `F10` | Armer immédiatement un nouveau composant choisi au hasard |
| `R` | Réinitialisation manuelle de la partie (facultative) |
| `Échap` | Quitter le jeu |

Un bandeau indique le nombre d'injections, la graine utilisée et l'état du sabotage. Le journal `karting.log` conserve l'identité de la cible, l'instant de simulation et la trace des exceptions.

## Cibles et réactions attendues

- **Optionnelles** : `input`, `camera`, `effects.sim`, `render.background`, `render.track`, `render.marks`, `render.vehicle`, `render.gantry`, `render.hud`. Le composant reste isolé entre deux tentatives, puis son opération réelle est réessayée automatiquement, sans bloquer les autres. En cas de défaillance du HUD, le bandeau d'injection est indépendant et le titre de fenêtre signale la panne.
- **Indispensables** : `physics`, `race`. Le moteur restaure l'état concerné et met la simulation **en pause temporaire**. Il rejoue ensuite le calcul interrompu jusqu'à la première réussite, puis reprend automatiquement sans `R`.

Le choix des cibles privilégie les systèmes invoqués à chaque image pour éviter qu'une panne ne reste armée indéfiniment. Les cibles déjà désactivées sont exclues des tirages suivants ; un défaut sur un composant indispensable peut également être tiré, mais avec un poids plus faible pour permettre d'observer plusieurs dégradations avant un éventuel arrêt de la simulation.

## Ce que ce test mesure (et ne mesure pas)

Chaque exception `InjectedFault` se produit **avant** l'appel du composant. Ce scénario vérifie le traitement des erreurs et l'isolation des services ; il ne simule ni la corruption silencieuse de l'état, ni la perte du contexte graphique, ni un crash du processus Python. Ce sont des scénarios distincts, à ajouter ultérieurement.

Les résultats reproductibles nécessitent la même graine **et** la même progression du temps ; la fréquence réelle des images peut modifier les instants précis. Une graine différente est utilisée par défaut à chaque lancement et figure dans le bandeau et le journal.

## Critères de réussite

1. Une panne optionnelle n'arrête ni la boucle principale ni l'avancement de la physique.
2. Une panne physique ne laisse pas un état partiellement modifié et met la simulation en pause.
3. Une panne de chronométrage restaure son état antérieur et met la simulation en pause.
4. Les pannes sont identifiées et conservées dans `karting.log`, sans répétition chaque image.
5. Les services sont réintégrés uniquement après une exécution réussie ; `R` reste une commande de remise à zéro manuelle.

**Attention :** une fenêtre Pygame et un clavier réels sont nécessaires à la validation graphique finale. Les tests unitaires et d'intégration utilisent un Pygame simulé pour la logique et ne prouvent pas le rendu.

## 🔄 Réparation automatique

Une panne ne se résout pas en remettant un indicateur sur « actif ». Le superviseur **réexécute l'opération qui a échoué**. Il peut reconstruire certains objets locaux avant de réessayer (caméra, HUD, effets, rendu). Une exception persistante laisse le module en défaut.

Délais entre tentatives : **0,5 s → 1 s → 2 s → 4 s → 8 s**, puis 8 s tant que le problème existe. Le moteur peut retenter sans limite, sans lancer une exception à chaque image. Le délai est compté même lorsque la simulation physique est en pause.

- **Physique et course :** l'état est préservé, le pas interrompu est réexécuté de façon contrôlée, puis la simulation reprend uniquement en cas de succès.
- **Affichage et commandes :** le module fautif est isolé, puis réessayé à sa prochaine utilisation. Un élément non visible n'est pas réessayé avant d'être sollicité.
- **Sauvegardes :** le meilleur temps non sauvegardé reste en mémoire et l'écriture est retentée jusqu'à réussite.
- **Diagnostic :** `karting.log` enregistre les échecs, délais, tentatives et messages `REPRISE`. Le HUD présente le prochain délai.

La récupération ne modifie pas arbitrairement le code source et ne garantit pas la résolution d'un bug logique permanent. Les pannes sont injectées avant les appels ; les corruptions silencieuses et arrêts brutaux du processus feront l'objet d'autres scénarios.
