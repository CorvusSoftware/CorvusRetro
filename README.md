<div align="center">

# CorvusRetro

**L'émulation rétro sur Android, avec des cœurs indépendants.**

Un frontend natif, une interface tactile originale et des moteurs téléchargeables à la demande.

[![Build Android](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml/badge.svg)](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml)
[![Release](https://img.shields.io/badge/release-v0.13.0-7965C1?style=flat-square)](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.13.0)
[![Android](https://img.shields.io/badge/Android-8.0%2B-3DDC84?style=flat-square&logo=android&logoColor=white)](https://github.com/CorvusSoftware/CorvusRetro/releases)
[![Licence](https://img.shields.io/badge/licence-AGPL--3.0--only-555555?style=flat-square)](LICENSE)

### [Télécharger CorvusRetro pour Android](https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.13.0/CorvusRetro-0.13.0-base-release.apk)

[Voir la version v0.13.0](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.13.0) · [Guide d'utilisation](#premiers-pas) · [Documentation](#documentation) · [Compiler](#compilation)

<sub>Version 0.13.0 en préversion · CorvusCore 0.14.1 · Aucun jeu ni BIOS fourni</sub>

</div>

---

## Présentation

**CorvusRetro** est un frontend Android développé par [CorvusSoftware](https://github.com/CorvusSoftware), conçu autour d'une idée simple : **l'application et les moteurs d'émulation sont séparés**.

L'APK de base contient l'interface, les commandes et les services Android, **mais aucun moteur d'émulation**. Les cœurs sont téléchargés individuellement depuis le catalogue officiel, vérifiés puis chargés via [CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI), sans installer un second APK.

### Moteurs disponibles

| Cœur | Console | État |
| :-- | :-- | :-- |
| **CorvusGB** | Game Boy | Disponible |
| **CorvusGBC** | Game Boy Color | Disponible |
| **CorvusGBA** | Game Boy Advance | Expérimental |

Les moteurs officiels sont fournis par [CorvusCore](https://github.com/CorvusSoftware/CorvusCore). Les archives natives sont distribuées séparément pour **ARM64** et **x86_64**.

## Premiers pas

1. Téléchargez et installez **[l'APK CorvusRetro](https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.13.0/CorvusRetro-0.13.0-base-release.apk)** sur un appareil compatible.
2. Dans l'application, ouvrez **Charger un cœur → Téléchargeur de cœurs**.
3. Téléchargez **CorvusGB**, **CorvusGBC** ou **CorvusGBA**.
4. Sélectionnez le cœur dans **Charger un cœur**, puis ouvrez votre jeu.

Le moteur devient disponible dès la fin du téléchargement. **Pas de second APK, pas de redémarrage.** Le déchargement d'un cœur n'efface ni son fichier ni vos sauvegardes.

[Guide du téléchargeur de cœurs](docs/individual-cores.md)

## Fonctionnalités

| | |
| :-- | :-- |
| **Interface native** | Menus en C11, navigation tactile, menu rapide et commandes virtuelles |
| **Affichage** | Modes portrait et paysage, disposition adaptée et multitouch |
| **Personnalisation** | Thèmes CorvusRetro, NVIDIA Shield et ASUS |
| **Bibliothèque** | Importation des jeux, historique et préservation des identifiants de contenu |
| **Sauvegardes** | Batterie et états instantanés selon les capacités de chaque cœur |
| **RetroAchievements** | Connexion, consultation et progression des succès compatibles |

### RetroAchievements

Les succès sont accessibles depuis **Réglages → RetroAchievements**, puis **Menu rapide → Succès** pendant le jeu.

L'intégration repose sur **rcheevos** et l'extension mémoire optionnelle de CorvusAPI. Le **mode normal** est disponible ; **Hardcore n'est pas encore pris en charge**. Le mot de passe n'est pas stocké et les données de session persistantes sont protégées avec Android Keystore.

[Lire la documentation des succès](docs/retroachievements.md)

## Architecture

```text
CorvusRetro
├── Interface et menus         C11
├── Rendu et audio             OpenGL ES 2 / AAudio
├── Services Android           Java
├── RetroAchievements          rcheevos
└── Chargement des cœurs       CorvusAPI
    ├── CorvusGB               CorvusCore
    ├── CorvusGBC              CorvusCore
    └── CorvusGBA              CorvusCore
```

Le code natif du frontend est organisé dans `menu/`, `frontend/`, `gfx/`, `audio/`, `input/` et `cores/`. Le projet Android réside dans `pkg/android/` ; les dépendances versionnées dans `deps/`.

CorvusRetro s'inspire de certains principes d'interface de RetroArch, mais utilise son propre code et **n'implémente pas l'ABI libretro**.

[Explorer l'organisation du dépôt](docs/repository-structure.md)

## Compilation

<details>
<summary><strong>Prérequis et compilation Android</strong></summary>

<br>

- JDK **17** ou supérieur
- Android SDK **35** et NDK **27.2.12479018**
- CMake **3.22.1**, Git et un environnement Android de compilation
- Gradle **8.13** et Android Gradle Plugin **8.9.1** (via le wrapper fourni)

```sh
git clone --recurse-submodules https://github.com/CorvusSoftware/CorvusRetro.git
cd CorvusRetro
./gradlew --no-daemon :app:assembleDebug
```

**APK généré :** `pkg/android/app/build/outputs/apk/debug/app-debug.apk`

Le build produit aussi six bibliothèques natives des moteurs officiels dans `pkg/android/app/build/native-cores/Debug/`. Elles sont **exclues de l'APK de base** et destinées aux archives téléchargeables.

</details>

<details>
<summary><strong>Tests et vérifications</strong></summary>

<br>

```sh
# Régressions natives
make test

# Vérifications mémoire
make test SANITIZE=ON

# Android et lint
./gradlew --no-daemon :app:assembleDebug :app:lintDebug

# Vérification des archives natives
python3 tools/validate_core_archives.py
python3 tools/test_core_archives.py
```

La [CI Android](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml) vérifie également la structure de l'APK et les modules distribuables. Ces tests ne remplacent pas les vérifications sur appareil physique.

[Guide de validation](docs/validation-android.md)

</details>

## Documentation

| Guide | Contenu |
| :-- | :-- |
| [Navigation](docs/navigation.md) | Menus et paramètres |
| [Commandes](docs/controls.md) | Contrôles tactiles et dispositions |
| [Thèmes](docs/themes.md) | Personnalisation de l'interface |
| [Téléchargement des cœurs](docs/individual-cores.md) | Installation et mise à jour |
| [RetroAchievements](docs/retroachievements.md) | Succès et sécurité |
| [Identifiants et sauvegardes](docs/stable-content-identities.md) | Compatibilité des données |
| [Catalogue officiel](core-repository/README.md) | Distribution des archives natives |

## Licence

CorvusRetro est développé par **CorvusSoftware** sous licence **[AGPL-3.0-only](LICENSE)**. Les moteurs [CorvusCore](https://github.com/CorvusSoftware/CorvusCore) conservent leur licence AGPL-3.0-only ; [CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI) et [rcheevos](https://github.com/RetroAchievements/rcheevos) conservent leurs licences MIT.

Aucune ROM ni aucun BIOS commercial n'est distribué. CorvusRetro n'est affilié ni à Nintendo, ni à RetroAchievements, ni à RetroArch.

<div align="center">

<sub>© 2026 CorvusSoftware · [Code source](https://github.com/CorvusSoftware/CorvusRetro) · [Releases](https://github.com/CorvusSoftware/CorvusRetro/releases)</sub>

</div>
