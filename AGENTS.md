# Conventions CorvusRetro

- Android d'abord, moteurs C++20 dans `third_party/CorvusCore/`, interface Java et pont JNI dans `app/`.
- Écrire le code applicatif pour CorvusRetro. Étudier RetroArch comme référence et dessiner les ressources originales.
- Charger et décharger les handles uniquement sur la file FIFO CoreManager ; un seul lecteur propriétaire à la fois. Persister la batterie avant libération.
- Tous les fichiers source originaux commencent par les mentions suivantes, adaptées aux commentaires du langage :

```c
// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
```

- Les tirets Unicode longs sont interdits dans les fichiers du dépôt parent. Utiliser la ponctuation française ou le tiret ASCII. La CI contrôle cette règle.
- Préserver les licences et attributions de CorvusCore et du wrapper Gradle. Ne pas modifier les sources du sous-module.
- Validation : `JAVA_HOME=... make test CMAKE=...` pour les vrais moteurs, puis `./gradlew :app:assembleDebug :app:lintDebug`.
- Version 0.4 : menus MaterialUI originaux, thèmes NVIDIA Shield et sombre, CorvusCore 0.14.0 GB/GBC/GBA expérimental. Distinguer tests hôtes, compilation, émulateur et téléphone physique.
