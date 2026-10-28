# Menus du cœur

CorvusRetro conserve deux entrées distinctes :

- **Menu principal > Configuration du sous-système** apparaît dès qu'un cœur
  est chargé, même avant de charger un jeu. Fermer le contenu garde cette
  entrée, puisque le cœur reste chargé. Décharger le cœur la retire.
- **Menu rapide > Options du cœur** se trouve entre les sauvegardes d'état
  et les commandes. Le jeu reste en pause dans cette page ; Retour restaure
  la sélection et le défilement du menu rapide.

Les cœurs CorvusAPI actuellement distribués ne déclarent aucun sous-système
ni option modifiable. Les pages l'indiquent explicitement, sans réglage fictif.
Les paramètres de surimpression restent dans **Commandes**, séparément des
options du moteur.

Le câble link n'est pas activé par ces entrées. Il requiert une extension
publique de CorvusAPI, puis sa prise en charge par les cœurs et le frontend.
Voir la [proposition de contrat](subsystems-corvusapi-proposal.md).
Le SDK interne d'un fournisseur ne constitue pas un contrat frontend.

## Références RetroArch consultées

Lecture de `menu/menu_displaylist.c` à la révision
[`af622105435d776c1ce203c28387cbc27e0c7925`](https://github.com/libretro/RetroArch/blob/af622105435d776c1ce203c28387cbc27e0c7925/menu/menu_displaylist.c) :

- `menu_displaylist_has_subsystems` et le menu principal utilisent les
  déclarations de sous-systèmes du cœur pour décider de leur présence.
- Le menu rapide insère les options du cœur avant les commandes.
- `DISPLAYLIST_CORE_OPTIONS` affiche un état vide si aucune option n'existe.

CorvusRetro suit l'emplacement et la navigation des options avec du code
original C11. Pour les sous-systèmes, la présence de l'entrée suit la demande
de l'utilisateur : cœur chargé, plutôt que déclaration d'un sous-système.
Elle affiche donc un état indisponible avec les modules actuels.

## Vérification

`native_menu` couvre l'entrée avant chargement du contenu, sa disparition au
déchargement, son maintien à la fermeture du jeu, les états vides, le retour
avec sélection et défilement conservés et la pause avec un vrai moteur GBA.
Les parcours Android `overlays` visitent les deux pages en portrait et paysage.
