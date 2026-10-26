# Organisation des composants

Référence examinée : [RetroArch dda31fef](https://github.com/libretro/RetroArch/tree/dda31fef21aeb472be9de7ee53eb386cc8db1417).
L'organisation suit ses composants, en gardant uniquement les plateformes et
services implémentés dans CorvusRetro.

| CorvusRetro | Référence dans RetroArch | Rôle |
| --- | --- | --- |
| `menu/menu_driver.c` | `menu/` | Navigation et état des menus |
| `frontend/drivers/` | `frontend/drivers/` | Cycle de vie et services Android |
| `gfx/` | `gfx/` | Rendu et viewport |
| `audio/drivers/aaudio.c` | `audio/drivers/aaudio.c` | Sortie audio native |
| `input/play_layout.c` | `input/` | Géométrie des contrôles |
| `cores/` | `cores/` et chargement dynamique | Registre, moteurs et contenu |
| `deps/` | `deps/` | Bibliothèques externes versionnées |
| `pkg/android/` | `pkg/android/` | Assemblage de l'application Android |
| `tools/` | `tools/` | Validation et distribution |

Le CMake portable est à la racine. Gradle se trouve dans `pkg/android/` ; le
wrapper racine délègue à cet emplacement pour conserver la commande habituelle.
Les en-têtes restent proches de leur composant. Le code d'émulation demeure
fourni par CorvusCore via CorvusAPI, avec un sous-module identique avant et après
le déplacement vers `deps/CorvusCore`.

Le build Android produit directement des `.so` pour les archives. Il ne construit
plus d'APK de cœur. La base exclut tous les moteurs et conserve uniquement son
frontend. Le catalogue des moteurs est désormais hébergé dans
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository),
à l'emplacement `catalog/corvuscore.json`. Les anciens JSON ont été
supprimés de CorvusRetro. Les anciens scripts de packs et projets
dynamic-feature ont également été retirés.

Les noms de composants servent à retrouver les responsabilités réelles. Les
plateformes absentes ne reçoivent pas de répertoires vides.

Les intégrations de succès sont dans `cheevos/`, les réponses asynchrones dans `tasks/`. Le rendu EGL/OpenGL ES2 est séparé de JNI dans `gfx/drivers/gl2.c`.
