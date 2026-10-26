# CorvusRetro

**Un frontend Android natif pour jouer avec les moteurs Game Boy, Game Boy Color et Game Boy Advance de CorvusSoftware.**

CorvusRetro propose une interface originale en **C11**, des commandes tactiles, un téléchargeur de cœurs et la prise en charge de RetroAchievements. Les moteurs fonctionnent séparément grâce à **[CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI)** : l'application n'embarque aucun émulateur dans son APK de base.

[![Android CI](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml/badge.svg)](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml)
[![Version](https://img.shields.io/badge/version-0.13.0-6C5CE7)](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.13.0)
[![Licence](https://img.shields.io/badge/licence-AGPL--3.0--only-555555)](LICENSE)

**[Télécharger l'APK v0.13.0](https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.13.0/CorvusRetro-0.13.0-base-release.apk)** · [Toutes les versions](https://github.com/CorvusSoftware/CorvusRetro/releases) · [Documentation](docs/navigation.md)

> **État du projet :** la version 0.13.0 est une **préversion**. Les moteurs officiels sont disponibles séparément en version 0.14.1. CorvusGBA et l'ABI CorvusAPI 0 restent expérimentaux.

## Commencer à jouer

1. **Installer CorvusRetro** avec l'APK disponible ci-dessus (Android 8.0 ou plus, appareils compatibles).
2. Ouvrir **Charger un cœur > Téléchargeur de cœurs**.
3. Choisir **CorvusGB**, **CorvusGBC** ou **CorvusGBA** et télécharger le module correspondant.
4. Revenir à **Charger un cœur**, sélectionner le moteur, puis ouvrir un jeu obtenu légalement.

Les cœurs téléchargés sont vérifiés et installés dans le stockage privé de l'application. **Aucun APK supplémentaire ni redémarrage n'est nécessaire.** Décharger un cœur ferme son instance, mais conserve son module et les données de jeu.

| Cœur officiel | Console | Statut |
| :--- | :--- | :--- |
| **CorvusGB** | Game Boy | Moteur natif |
| **CorvusGBC** | Game Boy Color | Moteur natif |
| **CorvusGBA** | Game Boy Advance | Expérimental |

Le catalogue officiel distribue les modules CorvusAPI sous forme d'archives natives distinctes pour **ARM64** et **x86_64**. Les jeux, BIOS et fichiers commerciaux ne sont pas fournis.

[En savoir plus sur le téléchargeur](docs/individual-cores.md) · [Cœurs et compatibilité](docs/community-cores.md)

## Fonctionnalités

- **Interface tactile originale :** menus, navigation, commandes virtuelles et menu rapide.
- **Portrait et paysage :** image adaptée à l'écran, boutons tactiles et prise en charge du multitouch.
- **Thèmes :** CorvusRetro, NVIDIA Shield et ASUS.
- **Bibliothèque :** jeux importés, historique et identifiants de contenu conservés entre les versions.
- **Sauvegardes :** données batterie et états instantanés selon les capacités du cœur.
- **Cœurs indépendants :** téléchargement, vérification, chargement et déchargement sans installation d'APK supplémentaire.
- **RetroAchievements :** connexion au compte, consultation et suivi des succès des jeux reconnus.

[Commandes tactiles](docs/controls.md) · [Navigation](docs/navigation.md) · [Thèmes](docs/themes.md) · [Conservation des données](docs/stable-content-identities.md)

### RetroAchievements

Depuis **Réglages > RetroAchievements**, connectez votre compte ; les succès du jeu sont ensuite accessibles dans **Menu rapide > Succès**.

CorvusRetro utilise la bibliothèque officielle **rcheevos** et l'extension mémoire facultative de CorvusAPI. Le **mode normal** est pris en charge, mais **pas encore le mode Hardcore**. Le mot de passe n'est pas conservé ; les jetons de session et les succès en attente d'envoi sont protégés par Android Keystore.

Un moteur sans extension mémoire reste jouable, mais ne peut pas fournir les succès.

[Documentation RetroAchievements](docs/retroachievements.md)

## Architecture

CorvusRetro sépare le frontend Android des moteurs d'émulation. L'interface, les entrées, l'affichage et l'audio sont assurés par le frontend ; les moteurs sont chargés comme bibliothèques natives avec le contrat C public CorvusAPI.

```text
CorvusRetro (frontend Android)
  |-- Interface, commandes, audio, vidéo      C11
  |-- Services Android, stockage, réseau      Java
  |-- Succès                                 rcheevos
  |
  +-- CorvusAPI (ABI C expérimentale, v0)
        |-- CorvusGB    (CorvusCore)
        |-- CorvusGBC   (CorvusCore)
        +-- CorvusGBA   (CorvusCore)
```

| Répertoire | Rôle |
| :--- | :--- |
| [`menu/`](menu/) | Menus, navigation et thèmes |
| [`frontend/`](frontend/) | Cycle de vie de l'application et services de plateforme |
| [`gfx/`](gfx/), [`audio/`](audio/), [`input/`](input/) | Graphismes, sortie audio et commandes |
| [`cores/`](cores/) | Chargement CorvusAPI, registre, états et sauvegardes |
| [`cheevos/`](cheevos/), [`tasks/`](tasks/) | Succès et échanges asynchrones |
| [`pkg/android/`](pkg/android/) | Projet Android, Gradle, manifeste et services Java |
| [`deps/`](deps/) | Sous-modules Git : CorvusCore et rcheevos |
| [`core-repository/`](core-repository/) | Catalogue officiel des cœurs téléchargeables |
| [`tests/`](tests/), [`tools/`](tools/) | Tests natifs, validation et outils de publication |

CorvusRetro est **inspiré de certains principes d'organisation et de navigation de RetroArch**, mais il utilise son propre frontend et **n'est pas compatible avec l'ABI libretro**.

[Organisation détaillée](docs/repository-structure.md) · [Contrat CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI)

## Développement

### Prérequis

- **JDK 17** ou supérieur ;
- **Android SDK 35** et **NDK 27.2.12479018** ;
- **CMake 3.22.1** et un compilateur C/C++ approprié ;
- **Git**, pour initialiser les sous-modules.

Le projet utilise **Gradle 8.13** et **Android Gradle Plugin 8.9.1**. Les scripts `gradlew` à la racine redirigent vers le projet situé dans `pkg/android/`.

### Compiler l'application Android

```sh
git clone --recurse-submodules https://github.com/CorvusSoftware/CorvusRetro.git
cd CorvusRetro

./gradlew --no-daemon :app:assembleDebug
```

**APK de développement :** `pkg/android/app/build/outputs/apk/debug/app-debug.apk`.

La compilation produit aussi les **six bibliothèques natives** des trois moteurs pour ARM64 et x86_64, sous `pkg/android/app/build/native-cores/Debug/`. Ces bibliothèques **sont exclues de l'APK de base** et servent à préparer les archives téléchargeables.

### Exécuter les tests

```sh
# Tests natifs (frontend et moteurs de test)
make test

# Vérifications mémoire avec ASan / UBSan
make test SANITIZE=ON

# Build Android, lint et APK de tests
./gradlew --no-daemon :app:assembleDebug :app:lintDebug :app:assembleDebugAndroidTest

# Validation des catalogues et archives natives
python3 tools/validate_core_archives.py
python3 tools/test_core_archives.py
```

Les tests natifs activent volontairement les moteurs officiels ; **cela ne change pas l'APK Android sans cœurs**. La CI contrôle également la compilation, les APK et la distribution des modules. Les tests automatisés ne remplacent pas une validation sur appareil physique.

[Validation Android](docs/validation-android.md) · [CI GitHub Actions](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml)

## Cœurs et publications

Le catalogue actuel est **[`core-repository/cores-v3.json`](core-repository/cores-v3.json)**. Il contient les métadonnées de distribution des archives natives officielles. Les anciens catalogues sont conservés pour les versions déjà diffusées.

Les releases et les modules sont publiés depuis des artefacts vérifiés. Les fichiers téléchargés sont contrôlés avant leur chargement ; aucun module arbitraire du stockage externe n'est exécuté.

[Téléchargements officiels](https://github.com/CorvusSoftware/CorvusRetro/releases) · [Documentation des archives](docs/individual-cores.md)

## Licence et crédits

**CorvusRetro** est développé par **CorvusSoftware** et distribué sous licence **[AGPL-3.0-only](LICENSE)**. **[CorvusCore](https://github.com/CorvusSoftware/CorvusCore)** utilise également l'AGPL-3.0-only ; **[CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI)** et **[rcheevos](https://github.com/RetroAchievements/rcheevos)** conservent leurs licences MIT respectives.

Les noms, marques et consoles cités appartiennent à leurs ayants droit. CorvusRetro n'est affilié ni à Nintendo, ni à RetroAchievements, ni à RetroArch.

**Copyright © 2026 CorvusSoftware.**
