# Organisation des composants

Référence examinée : [RetroArch dda31fef](https://github.com/libretro/RetroArch/tree/dda31fef21aeb472be9de7ee53eb386cc8db1417).
L'organisation suit ses composants, en gardant uniquement les plateformes et
services implémentés dans CorvusRetro.

| CorvusRetro | Référence dans RetroArch | Rôle |
| --- | --- | --- |
| `menu/menu_driver.c` | `menu/menu_driver.c`, `menu/menu_displaylist.c`, `menu/cbs/` | Navigation, modèle des entrées et actions |
| `menu/interface_layout.[ch]` | Géométrie des pilotes `menu/drivers/` | Rectangles communs au dessin, toucher, focus et défilement |
| `menu/interface_draw.[ch]` | `menu/drivers/` | Composition des écrans et panneaux |
| `menu/widgets.[ch]`, `menu/theme.[ch]` | Primitives visuelles et palettes des pilotes | Cartes, formes, consoles et couleurs |
| `frontend/drivers/` | `frontend/drivers/` | Cycle de vie et services Android |
| `gfx/` | `gfx/` | Rendu et viewport |
| `audio/drivers/aaudio.c` | `audio/drivers/aaudio.c` | Sortie audio native |
| `input/play_layout.c` | `input/` | Géométrie des contrôles |
| `cores/` | `cores/` et chargement dynamique | Registre, moteurs et contenu |
| `cores/state_store.[ch]` | Services de sauvegarde séparés des pilotes de menu | Stockage transactionnel des états nommés et aperçus |
| `gfx/widgets/notifications.[ch]` | `gfx/widgets/` | Animation et affichage des notifications |
| `deps/` | `deps/` | Bibliothèques externes versionnées |
| `pkg/android/` | `pkg/android/` | Assemblage de l'application Android |
| `tools/` | `tools/` | Validation et distribution |

Le CMake portable est à la racine. Gradle se trouve dans `pkg/android/` ; le
wrapper racine délègue à cet emplacement pour conserver la commande habituelle.
Les en-têtes restent proches de leur composant. Le code d'émulation demeure
fourni par CorvusCore via CorvusAPI, avec un sous-module identique avant et après
le déplacement vers `deps/CorvusCore`.

Le build Android compile seulement le frontend `libcorvusretro.so` pour les
deux ABIs. La base ne contient aucun moteur. La construction des modules,
les archives ZIP, leurs releases et les catalogues par fournisseur appartiennent
à [CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository),
indépendamment du build et de la signature APK. Une mise à jour de cœur
n'exige aucune nouvelle release de l'application.

Le sous-module CorvusCore reste épinglé pour les en-têtes CorvusAPI et les tests
hôtes avec les moteurs réels. Le catalogue distribué est découvert dans
`CorvusRepository/catalog/v1/*.json` ; aucun JSON de catalogue, ancien pack APK
ou projet dynamic-feature n'est maintenu dans le frontend.

Les noms de composants servent à retrouver les responsabilités réelles. Les
plateformes absentes ne reçoivent pas de répertoires vides.

Les intégrations de succès sont dans `cheevos/`, les réponses asynchrones dans `tasks/`. Le rendu EGL/OpenGL ES2 est séparé de JNI dans `gfx/drivers/gl2.c`.

## Découpage RetroArch revérifié pour la refonte

Lecture du code à la révision
[`168c451a6bf436e97aa5561093ae46b5d1fe2e73`](https://github.com/libretro/RetroArch/tree/168c451a6bf436e97aa5561093ae46b5d1fe2e73),
le 4 novembre 2026 :

- [`menu/menu_driver.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/menu/menu_driver.c)
  coordonne les pilotes, entrées et sélections.
- [`menu/menu_displaylist.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/menu/menu_displaylist.c)
  construit les listes ;
  [`menu/menu_setting.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/menu/menu_setting.c)
  décrit les réglages et leurs opérations.
- [`menu/cbs/menu_cbs_ok.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/menu/cbs/menu_cbs_ok.c)
  associe les validations aux actions.
- [`menu/drivers/materialui.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/menu/drivers/materialui.c)
  porte la présentation du pilote, sa disposition et ses interactions visuelles.
- [`gfx/gfx_widgets.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/gfx/gfx_widgets.c)
  coordonne les widgets, avec un fichier spécialisé tel que
  [`gfx_widget_achievement_popup.c`](https://github.com/libretro/RetroArch/blob/168c451a6bf436e97aa5561093ae46b5d1fe2e73/gfx/widgets/gfx_widget_achievement_popup.c)
  pour le popup des succès.

Cette séparation porte sur les responsabilités, sans plafond uniforme de lignes.
La refonte CorvusRetro applique ce principe à son volume actuel :
`menu_driver.c` possède l'état et les actions, `interface_layout.c` la géométrie,
`interface_draw.c` la composition et `widgets.c` les primitives réutilisées.
Le dessinateur reçoit un lecteur d'entrées et un `cr_painter` ; il n'installe
aucun cœur et n'écrit aucune sauvegarde. Les popups restent dans `gfx/widgets`,
et les opérations de fichiers dans `cores/state_store.c`.

Les classes Android suivent les mêmes frontières : `FrontendImages` prépare
les images avec `FrontendImageFiles`, et `StateShares` délègue les fichiers à
`StateShareFiles` et leur exposition privée à `StateShareProvider`. Elles ne
dessinent pas de menus Java. Chaque nouveau composant garde son en-tête proche,
sa responsabilité explicite et ses tests ; les sources sont déclarées dans
CMake. Aucun fichier source RetroArch n'est ajouté au dépôt.

Les sources, guides et tests sont versionnés ; les builds, APK, bibliothèques,
captures et journaux de qualification restent hors Git. Les dépendances restent
dans `deps/`, sans duplication de leurs sources dans les composants maison.
