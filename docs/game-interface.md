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

Le menu rapide est un panneau sur la dernière image du jeu et suspend l'émulation.
Il propose, dans cet ordre :

1. Reprendre le contenu.
2. Redémarrer le contenu.
3. Charger un état nommé.
4. Sauvegarder un état nommé avec aperçu.
5. États classiques : sauvegarde manuelle et dernier état automatique disponible.
6. [Sauvegardes .sav / .srm](save-files.md), pour importer ou exporter la mémoire batterie.
7. Options du cœur, ou un état vide si aucun réglage n'est proposé.
8. Commandes, pour l'affichage de la surimpression.
9. Joueur des commandes tactiles et du clavier, seulement à deux joueurs.
10. Succès RetroAchievements.
11. Informations sur le cœur.
12. Accueil.
13. Quitter le jeu, en dernier, en conservant le cœur chargé.

Le panneau défile pour donner accès aux outils complémentaires. La collection
d'états et leurs actions sont décrites dans [l'interface mobile](interface-redesign.md).

Une opération non disponible pour le cœur est désactivée. Le retour d'un
sous-menu conserve la pause, la sélection et le défilement du menu rapide.
Une sauvegarde impossible laisse le contenu chargé et affiche l'erreur.
Les fichiers d'état restent associés au contenu et à la version du cœur,
sans changement du format ou des chemins solo existants. Les modules officiels
désactivent les états pendant le câble link, car ils ne sérialisent pas encore
la session complète. Voir [les menus du cœur](core-menus.md).

## Vérification

Les tests natifs couvrent les profils, les ratios vidéo, les zones tactiles,
les diagonales, le multitouch, le masquage et la navigation. Les états sont
sauvegardés et chargés avec les véritables moteurs. La phase Android `overlays`
de `RetroAchievementsInstrumentation` vérifie chaque console en portrait et
paysage, avec des programmes originaux. Les ROM commerciales des captures de
référence ne sont pas des fixtures et ne sont pas distribuées.
