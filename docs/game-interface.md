# Interface de jeu

CorvusRetro dessine sa surimpression originale en C11/OpenGL ES2. Les captures
RetroArch et ses descriptions RetroPad servent de références de disposition ;
aucun dessin, code ou écran de jeu de RetroArch n'est embarqué.

## Commandes par console

| Console | Commandes de jeu |
| --- | --- |
| Game Boy | Croix directionnelle, A, B, Start, Select |
| Game Boy Color | Croix directionnelle, A, B, Start, Select |
| Game Boy Advance | Croix directionnelle, A, B, L, R, Start, Select |

En portrait, la vidéo conserve son ratio natif au-dessus du panneau noir. En
paysage, les commandes occupent les gouttières latérales et la vidéo est centrée,
avec toute la hauteur disponible si le ratio et les marges le permettent. Les
fenêtres étroites réduisent la vidéo pour préserver les commandes. Les cibles
restent à 48dp sur téléphone ; les petites fenêtres sont réduites proportionnellement.

La croix propose huit directions et une petite zone centrale neutre. Les
contours deviennent plus clairs à l'appui, y compris lors d'un appui bref entre
deux images. Les commandes sont multitouch. Le bouton menu ouvre le menu rapide
et relâche les touches de jeu. La flèche masque ou rétablit les commandes ; elle
et le bouton menu restent disponibles lorsque la surimpression est masquée.

## Menu rapide

Le menu rapide est une liste plein écran qui suspend l'émulation. Il propose :

1. Reprendre le contenu.
2. Redémarrer le contenu.
3. Fermer le contenu en conservant le cœur chargé.
4. Sauvegardes d'état, avec sauvegarde et chargement réels.
5. Commandes, pour l'affichage de la surimpression.
6. Succès RetroAchievements.
7. Informations sur le cœur.
8. Menu principal.

Une opération non disponible pour le cœur est désactivée. Le retour d'un
sous-menu conserve la pause, la sélection et le défilement du menu rapide.
Une sauvegarde impossible laisse le contenu chargé et affiche l'erreur.
Les fichiers d'état restent associés au contenu et à la version du cœur,
sans changement du format ou des chemins existants.

## Vérification

Les tests natifs couvrent les profils, les ratios vidéo, les zones tactiles,
les diagonales, le multitouch, le masquage et la navigation. Les états sont
sauvegardés et chargés avec les véritables moteurs. La phase Android `overlays`
de `RetroAchievementsInstrumentation` vérifie chaque console en portrait et
paysage, avec des programmes originaux. Les ROM commerciales des captures de
référence ne sont pas des fixtures et ne sont pas distribuées.
