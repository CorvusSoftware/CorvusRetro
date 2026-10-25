# Conventions CorvusRetro

- Frontend natif original en C11 : menus, navigation, input, boucle Android, EGL/OpenGL ES2, audio AAudio, états et batterie.
- Moteurs C++20 dans `third_party/CorvusCore/`, SDK C figé ; Java limité aux services Android NativeActivity, SAF, index/préférences, glyphes et ressources. Aucun menu, widget applicatif ou moteur piloté par Java.
- Un seul thread frontend possède les handles. Le callback audio ne lit qu'un tampon PCM protégé. Persister la batterie avant remplacement ou libération ; conserver le moteur si une écriture demandée échoue.
- Toutes les sources originales portent les trois mentions, adaptées aux commentaires du langage :

```c
// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
```

- Aucun tiret Unicode long dans les fichiers du dépôt parent ; règle vérifiée par CI.
- Préserver les licences et crédits de CorvusCore, du NDK et du wrapper Gradle. Ne pas modifier les sources du sous-module.
- Références RetroArch pour l'architecture et la navigation ; écrire notre code et nos ressources.
- Validation : `make test CMAKE=...`, `make test SANITIZE=ON CMAKE=...`, puis `./gradlew :app:assembleDebug :app:lintDebug`.
- Distinguer les preuves hôtes, la compilation APK, l'émulateur et le téléphone physique. La migration conserve les données et chemins 0.4.
