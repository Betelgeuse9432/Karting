"""Presets de voitures V6 : données métier sans dépendance à Pygame."""

PALETTE = [(220, 30, 30), (40, 90, 220), (240, 200, 30), (240, 240, 245), (40, 170, 90), (235, 120, 20)]

# Les réglages de chaque voiture (c'est ici que tu peux les bidouiller !)
VOITURES = [
    dict(nom="KART", defaut=0, vmax=520, puissance=900, pedale_gaz=1.4, pedale_frein=2.0, freinage=800,
         grip_av=950, grip_ar=950, appui=0.0, emp=30, braquage=0.7, vsens=200, roulis_k=0.00005, roulis_max=0.03,
         tangage_k=0.4, herbe_frein=0.2, herbe_grip=0.5, camh=48, camdist=125,
         stats=(0.45, 0.9, 0.75, 0.85, 0.0, 0.4),
         notes=["Ultra léger, tourne sec", "Freine très fort", "Pas d'aileron, pas d'appui", "Se pilote à l'instinct"]),
    dict(nom="4x4", defaut=5, vmax=560, puissance=600, pedale_gaz=0.9, pedale_frein=1.3, freinage=560,
         grip_av=600, grip_ar=650, appui=0.05, emp=52, braquage=0.5, vsens=150, roulis_k=0.0003, roulis_max=0.12,
         tangage_k=1.8, herbe_frein=0.04, herbe_grip=0.1, camh=85, camdist=175,
         stats=(0.55, 0.55, 0.5, 0.5, 0.05, 1.0),
         notes=["Haut : penche beaucoup", "Très doux sur l'herbe", "Peu de grip sur l'asphalte", "Freine moins fort"]),
    dict(nom="GT3", defaut=3, vmax=700, puissance=800, pedale_gaz=1.0, pedale_frein=1.6, freinage=760,
         grip_av=740, grip_ar=780, appui=0.55, emp=46, braquage=0.55, vsens=150, roulis_k=0.00012, roulis_max=0.07,
         tangage_k=1.0, herbe_frein=0.18, herbe_grip=0.45, camh=65, camdist=150,
         stats=(0.8, 0.7, 0.7, 0.7, 0.5, 0.4),
         notes=["L'équilibrée : un peu de tout", "Bon appui aéro", "Freinage puissant", "Facile à prendre en main"]),
    dict(nom="WEC", defaut=1, vmax=820, puissance=950, pedale_gaz=0.9, pedale_frein=1.5, freinage=840,
         grip_av=760, grip_ar=800, appui=1.3, emp=50, braquage=0.5, vsens=140, roulis_k=0.00008, roulis_max=0.05,
         tangage_k=0.6, herbe_frein=0.3, herbe_grip=0.6, camh=58, camdist=160,
         stats=(1.0, 0.8, 0.9, 0.75, 1.0, 0.2),
         notes=["Pointe de vitesse énorme", "Appui aéro colossal", "Peu de grip à basse vitesse", "Déteste l'herbe"]),
]
