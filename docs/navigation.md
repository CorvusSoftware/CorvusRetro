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
| Accueil | Continuer ou importer, bibliothèque, cœurs, Saves, charger/décharger le cœur, Sous-système si un cœur est chargé |
| Jeux | Grille/liste, filtres Tous/GB/GBC/GBA, recherche, actions du jeu |
| Jeux > Menu | Importer un jeu, importer un dossier, historique, grille/liste |
| Saves | Partie courante et états par jeu |
| Saves > Jeu | Nouvel état, états nommés et actions ; états classiques et batterie pour la partie courante |
| Cœurs | Charger un cœur, fiche par cœur, téléchargement, suppression |
| Cœurs > Fiche | Informations réelles, installer ou charger/réinstaller/désinstaller |
| Menu rapide | Reprendre, Charger un état, Sauvegarder un état, Redémarrer, Options du jeu, Quitter le jeu, puis outils complémentaires en défilement |
| Menu rapide > Sauvegardes d'état | État manuel et dernier état automatique |
| Menu rapide > Sauvegardes du jeu | Importer `.sav` ou `.srm`, exporter `.sav` ou `.srm`, choix du joueur en câble link |
| Réglages > Affichage | Veille, présentation de l'image, filtrage |
| Réglages > Audio | Son et volume du jeu |
| Réglages > Commandes | Commandes tactiles, Manettes |
| Réglages > Système | Interface avec thèmes/notifications, sauvegardes, informations, import de contenu |
| Réglages > Sauvegardes | Copies de sécurité, État automatique |
| Réglages > RetroAchievements | Activation, Compte |
| Réglages > À propos | Application, cœur chargé, licence et crédits |

Le bouton Retour restaure la page précédente, sa sélection, son filtre et son
défilement. Les cinq onglets sont Accueil, Jeux, Saves, Cœurs et Réglages.
La géométrie des cartes et des panneaux est partagée entre dessin et toucher.
Le menu rapide suspend le jeu et affiche un panneau sur sa dernière image.
Fermer le contenu conserve le cœur chargé. La [référence de l'interface mobile](interface-redesign.md)
décrit la collection d'états nommés et l'adaptation de la maquette.
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

Le [guide des paramètres](settings-plan.md) décrit les catégories, leurs
sous-paramètres, les valeurs possibles et leur persistance.
