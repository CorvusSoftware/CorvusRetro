# Appuis et organisation des menus

## Références étudiées

- [MaterialUI](https://github.com/libretro/RetroArch/blob/master/menu/drivers/materialui.c) : `materialui_render_touch_feedback`, `materialui_pointer_up`, navigation principale et retour de sélection.
- [menu_input.h](https://github.com/libretro/RetroArch/blob/master/menu/menu_input.h) : seuils de maintien court de 200 ms et d'appui long de 1 seconde.
- [input_overlay_alpha.c](https://github.com/libretro/RetroArch/blob/master/input/input_overlay_alpha.c) : renforcement de l'opacité des images touchées par `alpha_mod`.
- [menu_displaylist.c](https://github.com/libretro/RetroArch/blob/master/menu/menu_displaylist.c) : catégories de réglages et liste d'informations.

Le code CorvusRetro reste original, en C11, avec ses propres palettes et ressources.
Les pages sans capacité déclarée par le cœur affichent leur état indisponible.

## Rangement

| Chemin | Contenu |
| --- | --- |
| Menu principal | Menu rapide si un contenu est chargé, charger/décharger le cœur, charger du contenu, configuration du sous-système si un cœur est chargé, Informations |
| Menu principal > Informations | À propos, informations du cœur chargé, licence et crédits |
| Listes de lecture | Historique uniquement |
| Menu rapide | Reprendre, Redémarrer, Fermer, Sauvegardes d'état, Options du cœur, Commandes, Succès, Informations sur le cœur, Menu principal |
| Menu rapide > Sauvegardes d'état | Sauvegarder l'état, Charger l'état |
| Réglages > Interface utilisateur | Thème de l'interface |
| Réglages > Vidéo | Garder l'écran allumé pendant le jeu |
| Réglages > Audio | Activer le son |
| Réglages > Affichage à l'écran > Surimpression tactile | Afficher les commandes |

Le bouton Retour restaure la page précédente, sa sélection, son filtre et son
défilement. Les trois onglets restent Menu principal, Listes de lecture, Réglages,
avec les icônes de maison, de liste et d'engrenage. Le menu rapide suspend le jeu
et occupe tout l'écran. Fermer le contenu conserve le cœur chargé.
Les [menus du cœur](core-menus.md) décrivent les options persistantes,
la sélection de deux jeux pour le câble link et les capacités des sessions.

## Réaction au toucher

- Commandes de jeu : entrée dès l'appui, opacité renforcée sans déplacer la cible tactile ni changer la couleur du glyphe. Après relâchement, le retour au repos dure environ 100 ms.
- Un appui très bref reste visible pendant au moins une frame même si le moteur a déjà consommé l'entrée ; le feedback visuel ne maintient aucune touche du jeu.
- Menus : fondu de sélection sur environ 200 ms tant que le doigt reste immobile sur la ligne.
- Tap inférieur à 200 ms : action au relâchement sur la même cible.
- Maintien de 200 ms à moins d'une seconde : sélection sans validation.
- Appui d'au moins une seconde sur Audio, Garder l'écran allumé ou Afficher les commandes : restauration de la valeur par défaut activée. Les entrées de navigation restent en sélection.
- Glissement : défilement, sans valider de ligne. Annulation, perte de focus ou rotation : entrées et feedback réinitialisés.

Les menus utilisent un doigt ; les commandes de jeu conservent le multitouch.
La durée des gestes repose sur les timestamps Android des événements et reste
indépendante de la cadence du rendu. Le bouton de menu s'ouvre au relâchement ;
un glissement ou une annulation ne valide aucune action.

L'option **Afficher les commandes** masque ou rétablit la surimpression et relâche
les entrées en cours. Les commandes de jeu tactiles sont inactives lorsqu'elles
sont masquées ; le menu et la flèche restent accessibles. Le clavier et la
manette restent utilisables. Le choix est sauvegardé dans les préférences.

## Vérification

`native_menu` teste l'arbre des menus, le retour avec sélection conservée, les
informations d'un vrai cœur, les gestes tap/maintien/reset, une frame lente, le
menu à un doigt, l'opacité dessinée d'un tap bref, son fondu et la désactivation
des commandes tactiles. Voir [l'interface de jeu](game-interface.md) et [la validation Android](validation-android.md)
pour les essais et captures sur émulateur.
