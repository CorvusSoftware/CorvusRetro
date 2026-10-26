# Appuis et organisation des menus

## Références étudiées

- [MaterialUI](https://github.com/libretro/RetroArch/blob/master/menu/drivers/materialui.c) : `materialui_render_touch_feedback`, `materialui_pointer_up`, navigation principale et retour de sélection.
- [menu_input.h](https://github.com/libretro/RetroArch/blob/master/menu/menu_input.h) : seuils de maintien court de 200 ms et d'appui long de 1 seconde.
- [input_overlay_alpha.c](https://github.com/libretro/RetroArch/blob/master/input/input_overlay_alpha.c) : renforcement de l'opacité des images touchées par `alpha_mod`.
- [menu_displaylist.c](https://github.com/libretro/RetroArch/blob/master/menu/menu_displaylist.c) : catégories de réglages et liste d'informations.

Le code CorvusRetro reste original, en C11, avec ses propres palettes et ressources.
Seules les catégories disposant de fonctions effectives sont proposées.

## Rangement

| Chemin | Contenu |
| --- | --- |
| Menu principal | Menu rapide si un contenu est chargé, charger/décharger le cœur, charger du contenu, Informations |
| Menu principal > Informations | À propos, informations du cœur chargé, licence et crédits |
| Listes de lecture | Historique, Game Boy, Game Boy Color, Game Boy Advance |
| Réglages > Interface utilisateur | Thème de l'interface |
| Réglages > Vidéo | Garder l'écran allumé pendant le jeu |
| Réglages > Audio | Activer le son |
| Réglages > Affichage à l'écran > Surimpression tactile | Afficher les appuis |

Le bouton Retour restaure la page précédente, sa sélection, son filtre et son
défilement. Les trois onglets restent Menu principal, Listes de lecture, Réglages,
dans l'ordre de la barre de navigation MaterialUI. Le thème sombre supprimé reste
retiré ; le nom MaterialUI désigne ici la référence de navigation.

## Réaction au toucher

- Commandes de jeu : entrée dès l'appui, opacité renforcée sans déplacer la cible tactile ni changer la couleur du glyphe. Après relâchement, le retour au repos dure environ 100 ms.
- Un appui très bref reste visible pendant au moins une frame même si le moteur a déjà consommé l'entrée ; le feedback visuel ne maintient aucune touche du jeu.
- Menus : fondu de sélection sur environ 200 ms tant que le doigt reste immobile sur la ligne.
- Tap inférieur à 200 ms : action au relâchement sur la même cible.
- Maintien de 200 ms à moins d'une seconde : sélection sans validation.
- Appui d'au moins une seconde sur Audio, Garder l'écran allumé ou Afficher les appuis : restauration de la valeur par défaut activée. Les entrées de navigation restent en sélection.
- Glissement : défilement, sans valider de ligne. Annulation, perte de focus ou rotation : entrées et feedback réinitialisés.

Les menus utilisent un doigt ; les commandes de jeu conservent le multitouch.
La durée des gestes repose sur les timestamps Android des événements et reste
indépendante de la cadence du rendu. Le bouton central de menu conserve le geste
de la version 0.8 : ouverture au relâchement, glissement ou annulation sans action.

L'option **Afficher les appuis** conserve les entrées du moteur et leur réactivité ;
elle règle uniquement leur affichage. Le choix est sauvegardé dans les préférences.

## Vérification

`native_menu` teste l'arbre des menus, le retour avec sélection conservée, les
informations d'un vrai cœur, les gestes tap/maintien/reset, une frame lente, le
menu à un doigt, l'opacité dessinée d'un tap bref, son fondu et la désactivation
du feedback sans perte d'entrée. Voir [la validation Android](validation-android.md)
pour les essais et captures sur émulateur.
