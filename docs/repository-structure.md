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
