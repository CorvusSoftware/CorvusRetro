# CorvusRetro

Version 0.8 : frontend original C11 avec NativeActivity. L'utilisateur exige l'approche native C/C++ de RetroArch ; ne pas réintroduire de menus Java.

- `frontend.c` : menus, thèmes, navigation et association fichier/système. `play_layout.c/h` : géométrie unique du lecteur, des commandes et du toucher ; portrait vidéo séparée, paysage plein viewport, menu entre Select/Start, croix 8 directions, pas de titre en jeu.
- `native_main.c` : Looper, cycle Android et cadence ; `android_backend.c` : JNI des services, EGL, GLES2, atlas et cache de glyphes.
- `audio_android.c` : AAudio et tampon PCM ; `engine.c` : SDK C, contenu, états et batterie transactionnels.
- CorvusCore C++20 figé au commit `38829d4935bbdcf4b13261714352362065b78551`, API C version 0 vérifiée, GB/GBC/GBA expérimental.
- Java limité à MainActivity/NativeActivity et GameLibrary : SAF, stockage/index/préférences compatibles 0.4, ressources et polices Android. Aucun menu, widget applicatif ou moteur Java.
- JNI import enregistré par System.loadLibrary ; événement gardé avant publication du Looper. `finishImport(ZI)String` commit après validation C ; le suffixe d'ID suit le système résolu en C.
- Batterie : .new/fsync/rename puis génération acquittée. Échec disque : conserver le moteur pour une nouvelle tentative. Sauvegarder avant de relire la batterie lors d'une réouverture du même jeu.
- Catalogue C theme.c/h : IDs stables0Shield/2Corvus/3ASUS, affichage Corvus/Shield/ASUS ; ID1 retiré. Corvus par défaut si préférence absente/inconnue ; ancien dark migré vers corvus, autres choix conservés. Palettes en-tête, nav, contrôle et texte séparées pour ASUS clair. Java applique seulement la couleur ARGB de barre décidée en C.
- Thème Corvus bleu nuit/violet inspiré du logo fourni ; ASUS noir/blanc du logotype (référence Simple Icons14.0.0, siteASUS bloqué par le réseau). Icônes originales à tête de corbeau. Aucun tiret Unicode long dans les fichiers du dépôt parent ; sous-module intact et crédits CorvusCore/NDK/Gradle conservés.
- Tests : `make test CMAKE=...` et `SANITIZE=ON` (notre C portable instrumenté, SDK non instrumenté), Gradle assembleDebug/lintDebug. CTest native_engine/native_frontend/native_viewport/native_controls, aucune JVM pour les tests hôtes.
- Pins : Gradle 8.13, AGP 8.9.1, SDK 35, min 26, NDK 27.2.12479018, CMake 3.22.1, Java17/JDK21. ARM64 et x86_64.
- Preuves et limites dans docs/validation-android.md. Menus OpenGL sans arbre TalkBack ; émulateurs Android10 et15 exercés, pas de téléphone physique ou d'exécution ARM64 validée.

Version0.7 : NativeActivity en coordonnées fenêtre entière, marges WindowInsets système/cutout publiées par snapshot immuable Java vers C ; viewport.c valide dimensions et origine commune OpenGL/tactile. Thèmes actifs IDs0Shield/2Corvus/3ASUS, ID1 retiré ; clé dark migrée vers corvus.
