# Menus du cœur

CorvusRetro conserve deux entrées distinctes :

- **Menu principal > Configuration du sous-système** apparaît dès qu'un cœur
  est chargé, même avant de charger un jeu. Fermer le contenu garde cette
  entrée, puisque le cœur reste chargé. Décharger le cœur la retire.
- **Menu rapide > Options du cœur** se trouve entre les sauvegardes d'état
  et les commandes. Le jeu reste en pause dans cette page ; Retour restaure
  la sélection et le défilement du menu rapide.

Les modules CorvusCore 0.15 proposent des réglages réels : palette GB,
couleurs de compatibilité GB dans GBC, type de sauvegarde et horloge GBA,
ainsi que la source audio du câble link. Ouvrir une option affiche ses valeurs
et la sélection courante. Les sélections sont mémorisées par cœur dans le
stockage privé. Les réglages GBA signalés comme différés s'appliquent au
prochain chargement ou redémarrage ; les palettes et le choix audio sont immédiats.
Les paramètres de surimpression restent dans **Commandes**.

## Deux joueurs (câble link)

1. Charger un cœur, puis ouvrir **Configuration du sous-système**.
2. Choisir **Deux joueurs (câble link)**.
3. Choisir ou importer un jeu compatible pour **Joueur 1**, puis **Joueur 2**.
4. Choisir **Démarrer**. Cette action reste désactivée tant qu'un jeu manque.

Les deux consoles s'affichent côte à côte. Le menu rapide permet de choisir
le joueur des commandes tactiles et du clavier. Les deux premières manettes
connectées commandent leurs ports respectifs ; une déconnexion relâche ses
touches. Une importation annulée conserve la sélection et la partie existantes.

Chaque joueur possède sa batterie : le joueur 1 conserve le chemin historique,
le joueur 2 utilise un suffixe `-link-p2.sav`, même avec la même ROM deux fois.
Une erreur d'écriture empêche le remplacement de la session active.
Fermer le contenu conserve le cœur, ses options et l'entrée de configuration.

Les modules officiels ne proposent pas d'états instantanés groupés ni de
lecture mémoire RetroAchievements pendant le link. Ces actions sont désactivées ;
le compte et les succès en attente sont conservés. Le link GBA reste expérimental :
les transferts synthétiques sont testés, sans garantie de compatibilité de chaque jeu.

Les extensions publiques optionnelles version 1 sont définies dans
[CorvusAPI/CONFIGURATION.md](https://github.com/CorvusSoftware/CorvusAPI/blob/main/docs/CONFIGURATION.md).
Les anciens modules sans extension restent chargeables et affichent un état
indisponible. Le frontend n'accède pas au SDK privé du fournisseur.

## Références RetroArch consultées

Lecture de `menu/menu_displaylist.c` à la révision
[`dda31fef21aeb472be9de7ee53eb386cc8db1417`](https://github.com/libretro/RetroArch/blob/dda31fef21aeb472be9de7ee53eb386cc8db1417/menu/menu_displaylist.c) :

- `menu_displaylist_has_subsystems` et le menu principal utilisent les
  déclarations de sous-systèmes du cœur pour décider de leur présence.
- Le menu rapide insère les options du cœur avant les commandes.
- `DISPLAYLIST_CORE_OPTIONS` affiche un état vide si aucune option n'existe.

CorvusRetro suit l'emplacement et la navigation des options avec du code
original C11. Pour les sous-systèmes, la présence de l'entrée suit la demande
de l'utilisateur : cœur chargé, plutôt que déclaration d'un sous-système.
Elle affiche un état indisponible si le module ne déclare aucun sous-système.

## Vérification

Les tests natifs couvrent la négociation, les valeurs réelles, la persistance
et ses erreurs, les deux jeux, les imports annulés ou périmés, les batteries
distinctes, les capacités des sessions et le retour en pause. Le parcours Android
`configuration` de `RetroAchievementsInstrumentation` vérifie ces menus avec
les modules natifs installés séparément, en portrait et paysage.
