# CorvusRetro

Version 0.10 : frontend original C11 avec NativeActivity. L'utilisateur exige l'approche native C/C++ de RetroArch ; ne pas réintroduire de menus Java.

- `frontend.c` : menus, thèmes, navigation et association fichier/système. `play_layout.c/h` : géométrie unique du lecteur, des commandes et du toucher ; portrait vidéo séparée, paysage plein viewport, menu entre Select/Start, croix 8 directions, pas de titre en jeu.
- `native_main.c` : Looper, cycle Android et cadence ; `android_backend.c` : JNI des services, EGL, GLES2, atlas et cache de glyphes.
- `audio_android.c` : AAudio et tampon PCM ; `engine.c` : CorvusAPI, contenu, états et batterie transactionnels.
- CorvusCore C++20 figé au commit `8433980530425232568f35085dc1b7cc3b12ab00`, CorvusAPI MIT ABI0 au commit `188a1a50d83e965f1bdc7a958b18c3e90c6432f4`. Ne pas revenir au SDK interne dans le frontend.
- `core_registry.c` valide les modules et identités durables. APK de base sans cœurs ; pack explicite `-PcorvusCores=gb,gbc,gba`. Lire docs/core-downloader.md et docs/stable-content-identities.md avant toute modification des cœurs.
- Java limité à MainActivity/NativeActivity et GameLibrary : SAF, stockage/index/préférences compatibles 0.4, ressources et polices Android. Aucun menu, widget applicatif ou moteur Java.
- JNI import enregistré par System.loadLibrary ; événement gardé avant publication du Looper. `finishImport` commit après validation C avec identifiants durables du cœur/système ; suffixes historiques GB/GBC/GBA préservés. Le catalogue peut être vide et les jeux dont le cœur manque sont conservés.
- Batterie : .new/fsync/rename puis génération acquittée. Échec disque : conserver le moteur pour une nouvelle tentative. Sauvegarder avant de relire la batterie lors d'une réouverture du même jeu.
- Catalogue C theme.c/h : IDs stables0Shield/2Corvus/3ASUS, affichage Corvus/Shield/ASUS ; ID1 retiré. Corvus par défaut si préférence absente/inconnue ; ancien dark migré vers corvus, autres choix conservés. Palettes en-tête, nav, contrôle et texte séparées pour ASUS clair. Java applique seulement la couleur ARGB de barre décidée en C.
- Thème Corvus bleu nuit/violet inspiré du logo fourni ; ASUS noir/blanc du logotype (référence Simple Icons14.0.0, siteASUS bloqué par le réseau). Icônes originales à tête de corbeau. Aucun tiret Unicode long dans les fichiers du dépôt parent ; sous-module intact et crédits CorvusCore/NDK/Gradle conservés.
- Tests : `make test CMAKE=...` et `SANITIZE=ON` (notre C portable instrumenté, SDK non instrumenté), Gradle assembleDebug/lintDebug. CTest native_engine/native_frontend/native_viewport/native_controls/native_menu, aucune JVM pour les tests hôtes.
- Pins : Gradle 8.13, AGP 8.9.1, SDK 35, min 26, NDK 27.2.12479018, CMake 3.22.1, Java17/JDK21. ARM64 et x86_64.
- Preuves et limites dans docs/validation-android.md. Menus OpenGL sans arbre TalkBack ; émulateurs Android10 et15 exercés, pas de téléphone physique ou d'exécution ARM64 validée.

Version0.7 : NativeActivity en coordonnées fenêtre entière, marges WindowInsets système/cutout publiées par snapshot immuable Java vers C ; viewport.c valide dimensions et origine commune OpenGL/tactile. Thèmes actifs IDs0Shield/2Corvus/3ASUS, ID1 retiré ; clé dark migrée vers corvus.

Version0.9 : catégories UI/Vidéo/Audio/OSD, historique dans listes, informations/licences au menu principal. Pile bornée8vues (page/focus/filter/scroll) et tab calculé par page. Gestes menus aux timestamps Android : tap<200ms, maintien sélection, long>=1s restaure les booléens pris en charge. Une seule touche dans menus, multitouch en jeu. Feedback alpha séparé des pulses moteur ; feedback_pulses garde le premier flash pendant une frame puis fondu100ms. show_input (int Java générique) sauvegarde le choix visuel.

Reprise0.10 : une seule ligne cœur dans le menu principal, Charger sans handle et Décharger avec handle ; Menu rapide précède cette ligne si contenu actif. Un échec batterie conserve le cœur et l’action Décharger. ABI0 ne déclare aucun sous-système et les adaptateurs officiels n’exposent qu’un joueur. Le câble link exige d’abord un contrat public dans CorvusAPI ; proposition dans docs/subsystems-corvusapi-proposal.md. Ne pas réappliquer l’ancien prototype direct sur cette migration.
