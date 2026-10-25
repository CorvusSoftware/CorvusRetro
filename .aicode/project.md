# CorvusRetro

Version 0.5 : frontend original C11 avec NativeActivity. L'utilisateur exige l'approche native C/C++ de RetroArch ; ne pas réintroduire de menus Java.

- `frontend.c` : menus, thèmes, navigation, association fichier/système et commandes.
- `native_main.c` : Looper, cycle Android et cadence ; `android_backend.c` : JNI des services, EGL, GLES2, atlas et cache de glyphes.
- `audio_android.c` : AAudio et tampon PCM ; `engine.c` : SDK C, contenu, états et batterie transactionnels.
- CorvusCore C++20 figé au commit `38829d4935bbdcf4b13261714352362065b78551`, API C version 0 vérifiée, GB/GBC/GBA expérimental.
- Java limité à MainActivity/NativeActivity et GameLibrary : SAF, stockage/index/préférences compatibles 0.4, ressources et polices Android. Aucun menu, widget applicatif ou moteur Java.
- JNI import enregistré par System.loadLibrary ; événement gardé avant publication du Looper. `finishImport(ZI)String` commit après validation C ; le suffixe d'ID suit le système résolu en C.
- Batterie : .new/fsync/rename puis génération acquittée. Échec disque : conserver le moteur pour une nouvelle tentative. Sauvegarder avant de relire la batterie lors d'une réouverture du même jeu.
- Thèmes Shield et bleu, icônes originales à tête de corbeau. Aucun tiret Unicode long dans les fichiers du dépôt parent ; sous-module intact et crédits CorvusCore/NDK/Gradle conservés.
- Tests : `make test CMAKE=...` et `SANITIZE=ON` (notre C portable instrumenté, SDK non instrumenté), Gradle assembleDebug/lintDebug. CTest native_engine/native_frontend, aucune JVM pour les tests hôtes.
- Pins : Gradle 8.13, AGP 8.9.1, SDK 35, min 26, NDK 27.2.12479018, CMake 3.22.1, Java17/JDK21. ARM64 et x86_64.
- Preuves et limites dans docs/validation-android.md. Menus OpenGL sans arbre TalkBack ; pas de téléphone physique ou d'Android récent validé.
