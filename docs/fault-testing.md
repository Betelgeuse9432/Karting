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
| `R` | Réinitialiser la partie et réactiver les composants désactivés |
| `Échap` | Quitter le jeu |

Un bandeau indique le nombre d'injections, la graine utilisée et l'état du sabotage. Le journal `karting.log` conserve l'identité de la cible, l'instant de simulation et la trace des exceptions.

## Cibles et réactions attendues

- **Optionnelles** : `input`, `camera`, `effects.sim`, `render.background`, `render.track`, `render.marks`, `render.vehicle`, `render.gantry`, `render.hud`. Le système est désactivé pour le reste de la session, mais les autres doivent continuer à fonctionner. En cas de défaillance du HUD, le bandeau d'injection est indépendant et le titre de fenêtre signale la panne.
- **Indispensables** : `physics`, `race`. Le moteur capture l'exception, restaure l'état concerné et met la simulation **en pause**. Appuyer sur `R` pour recommencer et autoriser d'autres pannes.

Le choix des cibles privilégie les systèmes invoqués à chaque image pour éviter qu'une panne ne reste armée indéfiniment. Les cibles déjà désactivées sont exclues des tirages suivants ; un défaut sur un composant indispensable peut également être tiré, mais avec un poids plus faible pour permettre d'observer plusieurs dégradations avant un éventuel arrêt de la simulation.

## Ce que ce test mesure (et ne mesure pas)

Chaque exception `InjectedFault` se produit **avant** l'appel du composant. Ce scénario vérifie le traitement des erreurs et l'isolation des services ; il ne simule ni la corruption silencieuse de l'état, ni la perte du contexte graphique, ni un crash du processus Python. Ce sont des scénarios distincts, à ajouter ultérieurement.

Les résultats reproductibles nécessitent la même graine **et** la même progression du temps ; la fréquence réelle des images peut modifier les instants précis. Une graine différente est utilisée par défaut à chaque lancement et figure dans le bandeau et le journal.

## Critères de réussite

1. Une panne optionnelle n'arrête ni la boucle principale ni l'avancement de la physique.
2. Une panne physique ne laisse pas un état partiellement modifié et met la simulation en pause.
3. Une panne de chronométrage restaure son état antérieur et met la simulation en pause.
4. Les pannes sont identifiées et conservées dans `karting.log`, sans répétition chaque image.
5. `R` remet les composants en service et de nouvelles pannes peuvent être tirées.

**Attention :** une fenêtre Pygame et un clavier réels sont nécessaires à la validation graphique finale. Les tests unitaires et d'intégration utilisent un Pygame simulé pour la logique et ne prouvent pas le rendu.
