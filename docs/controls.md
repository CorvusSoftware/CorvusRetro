# Écran de jeu et commandes

La géométrie originale de CorvusRetro est calculée en C dans `play_layout.c`.
Le dessin et le toucher utilisent le même résultat, dans la surface utile déjà
protégée des barres système et des encoches.

## Références RetroArch

Les configurations [retropad](https://github.com/libretro/common-overlays/blob/master/gamepads/retropad/retropad.cfg),
[flat/gba](https://github.com/libretro/common-overlays/blob/master/gamepads/flat/gba.cfg)
et [neo-retropad](https://github.com/libretro/common-overlays/blob/master/gamepads/neo-retropad/neo-retropad.cfg)
du projet libretro ont été consultées. Elles emploient des positions normalisées,
des dispositions portrait/paysage distinctes, des directions combinées et des
zones tactiles adaptées à la forme des commandes. Dans flat/gba portrait, le
menu est centré en bas (`x=0.5`, `y=0.94792`).

CorvusRetro reprend ces principes avec son propre code C et ses propres ressources.
Le menu est centré au-dessus des commandes en portrait et placé dans la
gouttière gauche, au-dessus de la croix, en paysage.
Aucun fichier d'overlay ni image de RetroArch n'est embarqué.

## Disposition

- Portrait : vidéo en haut, commandes en dessous, espace supplémentaire pour L/R du GBA.
- Paysage : vidéo centrée entre les commandes, dans des gouttières latérales séparées.
- Croix continue : quatre directions et quatre diagonales, centre neutre.
- A/B : boutons circulaires avec toucher radial ; L/R uniquement pour GBA.
- Select et Start suivent les commandes principales ; le menu circulaire reste accessible dans l'aire de jeu.
- La flèche masque et rétablit les commandes. Elle et le menu restent visibles lorsque les commandes sont masquées.

Les traits gris deviennent plus clairs à l'appui. GB/GBC utilisent la croix,
A, B, Start et Select ; GBA ajoute L/R. Voir [les profils et le menu rapide](game-interface.md).

Les ratios natifs sont conservés : GB/GBC `160:144`, GBA `240:160`.
Les commandes sont recalculées selon la taille disponible ; les surfaces de
très petite hauteur réduisent leur taille pour éviter les collisions.
Les cibles atteignent au moins 48 dp aux dimensions de téléphone validées
(portrait à partir de 320 x 480 dp, paysage à partir de 320 dp de hauteur).

Un appui sur le bouton de menu dessiné met le jeu en pause et relâche toutes
les entrées. Glisser depuis ce bouton ne déclenche aucune commande de jeu.
Le retour Android permet toujours d'ouvrir et de fermer le menu rapide.

Pendant une partie locale à deux joueurs, le menu rapide choisit le joueur
contrôlé par le tactile et le clavier. Deux manettes physiques peuvent commander
les consoles indépendamment. Leurs touches sont relâchées lors d'une pause,
d'une perte de focus ou de leur déconnexion.

## Vérification

`native_controls` vérifie les ratios, limites et séparations des cibles pour
trois systèmes et plusieurs formats, les diagonales, puis le toucher des coordonnées
réellement transmises au dessinateur. Il couvre aussi le menu avec un autre
doigt sur A, la reprise sans bouton coincé, le déplacement et l'annulation.
Les essais Android et leurs limites sont détaillés dans [la validation](validation-android.md).

Le retour visuel et les catégories de réglages sont décrits dans [la navigation](navigation.md).
