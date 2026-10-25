# Conventions CorvusRetro

- Android d'abord ; moteurs C++20 dans le sous-module `third_party/CorvusCore/`, interface Android Java/JNI dans `app/`. `core/` reste une démo C11.
- Écrire le code applicatif pour CorvusRetro. RetroArch sert de référence conceptuelle ; ne pas en copier le code ou les ressources.
- Tout fichier source original commence par les trois mentions suivantes, adaptées à la syntaxe des commentaires du langage :

```c
// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
```

- Conserver les licences et attributions des outils tiers, dont le wrapper Gradle et CorvusCore. Ne pas modifier les sources du sous-module.
- Vérifier le noyau avec `make test`, la frontière JNI Linux avec `JAVA_HOME=... make jni-test`, les vrais moteurs avec `make core-jni-test` (CMake 3.22.1+), Android avec `./gradlew :app:assembleDebug :app:lintDebug`.
- Version 0.3 : navigation MaterialUI originale et SDK CorvusCore 0.14.0 (GB/GBC/GBA expérimental). Sérialiser les appels par moteur ; sauvegarder la batterie avant de la déclarer acquittée. Distinguer tests hôtes, compilation APK, essais émulateur et appareil physique.
