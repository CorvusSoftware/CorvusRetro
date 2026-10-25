# CorvusRetro

Android d'abord. Application originale Java/JNI, signature AGPL-3.0-only définie dans AGENTS.md.
Version 0.3 : navigation inspirée de MaterialUI réellement étudié (listes, trois onglets, menu rapide).
MainActivity : import SAF privé, sélection de cœur, listes par console, historique, réglages.
PlayerActivity/EmulationSession : moteurs sur thread FIFO dédié, rendu ARGB UI, AudioTrack PCM16,
commandes tactiles/clavier/manette, pause Android, états et batterie AtomicFile avec acquittement.
CorvusCore 0.14.0, sous-module third_party/CorvusCore au commit 38829d4935bbdcf4b13261714352362065b78551.
SDK C API version 0 préliminaire vérifiée au JNI, C++20, GB/GBC et GBA expérimental. Ne pas modifier ses sources.
Notices CorvusCore embarquées. Aucun code/graphisme RetroArch copié.
Ancienne démo C11 dans core/ et GameActivity, accessible dans les réglages.
Validation : make test, make jni-test, make core-jni-test, assembleDebug, lintDebug.
Pins : Gradle 8.13, AGP 8.9.1, SDK/target 35, min 26, NDK 27.2.12479018, CMake 3.22.1, Java17 ; JDK21 utilisé.
ABIs ARM64/x86_64. Rotation du vrai lecteur préserve son moteur (configChanges) ; la démo peut se réinitialiser.
Preuves et limites des essais Android dans docs/validation-android.md ; pas de téléphone physique testé.
