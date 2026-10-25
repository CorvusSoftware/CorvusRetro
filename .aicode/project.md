# CorvusRetro

Android d'abord, confirmé par l'utilisateur. Code original C11 + Java/JNI.
Signature et AGPL-3.0-only définis dans AGENTS.md. Ne pas reprendre le code de RetroArch.
Le prototype affiche une démo interactive, sans émulation/ROM/BIOS/audio.
État C et pont JNI utilisés sur le thread UI ; pause Android retire les callbacks.
Commandes vérifiées : make test, make jni-test (JDK complet), ./gradlew :app:assembleDebug :app:lintDebug.
Pins : Gradle 8.13, AGP 8.9.1, SDK/target 35, min 26, NDK 27.2.12479018, Java 17 ; validation sous JDK 21.0.13.
Bibliothèques embarquées : ARM64/x86_64. Rotation : reset volontaire de la démo.
Tests hôtes/JNI et compilation Android ne prouvent pas les interactions sur téléphone.
