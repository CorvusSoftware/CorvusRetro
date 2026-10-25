# Conventions CorvusRetro

- Android d'abord ; noyau C11 portable dans `core/`, interface Android Java/JNI dans `app/`.
- Écrire le code applicatif pour CorvusRetro. RetroArch sert de référence conceptuelle ; ne pas en copier le code ou les ressources.
- Tout fichier source original commence par les trois mentions suivantes, adaptées à la syntaxe des commentaires du langage :

```c
// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
```

- Conserver les licences et attributions des outils tiers, dont le wrapper Gradle.
- Vérifier le noyau avec `make test`, la frontière JNI Linux avec `JAVA_HOME=... make jni-test`, Android avec `./gradlew :app:assembleDebug :app:lintDebug`.
- La version initiale est une démo de rendu et d'entrée, pas encore un émulateur. Distinguer les tests hôtes, la compilation APK et les essais sur appareil.
