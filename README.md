<h1 align="center">CorvusRetro</h1>

<p align="center">
  <strong>Un frontend Android natif, conçu pour les consoles Nintendo classiques.</strong>
</p>

<p align="center">
  Game Boy &nbsp;·&nbsp; Game Boy Color &nbsp;·&nbsp; Game Boy Advance
</p>

<p align="center">
  <a href="https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.15.1"><img alt="Version 0.15.1" src="https://img.shields.io/badge/version-0.15.1-7057c9?style=flat-square"></a>
  <a href="https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml"><img alt="Android CI" src="https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml/badge.svg"></a>
  <a href="LICENSE"><img alt="Licence AGPL-3.0-only" src="https://img.shields.io/badge/licence-AGPL--3.0--only-555555?style=flat-square"></a>
</p>

<p align="center">
  <strong><a href="https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.15.1/CorvusRetro-0.15.1-base-release.apk">Télécharger l'APK</a></strong>
  &nbsp;·&nbsp;
  <a href="https://github.com/CorvusSoftware/CorvusRetro/releases">Toutes les versions</a>
  &nbsp;·&nbsp;
  <a href="docs/navigation.md">Documentation</a>
</p>

---

**CorvusRetro** est un frontend Android développé par **CorvusSoftware**. Son interface est écrite en **C11** et s'appuie sur des moteurs natifs indépendants via [CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI).

L'application propose une bibliothèque de jeux, des commandes tactiles, plusieurs thèmes, des sauvegardes et l'intégration RetroAchievements. **L'APK de base ne contient aucun moteur ni aucun jeu.**

> **Version actuelle : 0.15.1 (préversion).** Les cœurs officiels sont distribués séparément en version 0.14.1. L'émulation GBA et l'ABI CorvusAPI 0 restent expérimentales.

## Installation

1. [Télécharger CorvusRetro v0.15.1](https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.15.1/CorvusRetro-0.15.1-base-release.apk) et installer l'APK sur un appareil compatible **Android 8.0 ou plus**.
2. Dans l'application, ouvrir **Charger un cœur > Téléchargeur de cœurs**.
3. Télécharger le moteur souhaité, puis le sélectionner dans **Charger un cœur**.
4. Importer et ouvrir un jeu que vous possédez légalement.

Le téléchargement installe un module natif vérifié dans le stockage privé de l'application. **Aucun autre APK et aucun redémarrage ne sont nécessaires.** Un cœur déchargé reste installé pour une prochaine utilisation.

### Consoles prises en charge

| Moteur | Console | État |
| :-- | :-- | :-- |
| **CorvusGB** | Game Boy | Moteur natif |
| **CorvusGBC** | Game Boy Color | Moteur natif |
| **CorvusGBA** | Game Boy Advance | Expérimental |

Les modules officiels sont disponibles pour **arm64-v8a** et **x86_64**.

[Guide du téléchargeur](docs/individual-cores.md) · [Gestion des cœurs](docs/community-cores.md)

## Fonctionnalités

- **Interface entièrement native :** menus et navigation en C11, sans interface d'émulation Java.
- **Commandes tactiles :** disposition adaptée au portrait et au paysage, avec multitouch et menu rapide.
- **Bibliothèque et sauvegardes :** jeux importés, historique, sauvegardes batterie et états instantanés selon le moteur.
- **Thèmes :** CorvusRetro, NVIDIA Shield et ASUS.
- **Cœurs téléchargeables :** installation et mise à jour indépendantes de l'application.
- **RetroAchievements :** identification des jeux et suivi des succès dans le mode normal.
- **Notifications natives :** icône du jeu au lancement, badges des succès et progression mesurée, avec chargement des images en arrière-plan.

Le **mode Hardcore** de RetroAchievements n'est pas encore pris en charge. Les jetons de connexion et les succès en attente sont protégés par Android Keystore ; les mots de passe ne sont pas conservés.

[Interface et navigation](docs/navigation.md) · [Commandes](docs/controls.md) · [Thèmes](docs/themes.md) · [RetroAchievements](docs/retroachievements.md)

## Architecture

CorvusRetro reste indépendant des moteurs d'émulation. Cette séparation permet de faire évoluer l'application et les cœurs sans les fusionner dans un même projet.

| Composant | Responsabilité |
| :-- | :-- |
| **CorvusRetro** | Interface C11, rendu OpenGL ES, audio AAudio, commandes et services Android |
| **[CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI)** | Contrat C public entre le frontend et les modules |
| **[CorvusCore](https://github.com/CorvusSoftware/CorvusCore)** | Moteurs d'émulation GB, GBC et GBA en C++20 |

Le code est organisé par responsabilités : `menu/`, `frontend/`, `gfx/`, `audio/`, `input/`, `cores/`, `cheevos/` et `tasks/`. Le projet Android est dans `pkg/android/` ; les dépendances externes sont dans `deps/`.

CorvusRetro s'inspire de certains principes de navigation de RetroArch, mais **n'utilise pas l'ABI libretro** et possède son propre code frontend.

[Découvrir la structure du dépôt](docs/repository-structure.md)

## Compilation

**Prérequis :** JDK 17, Android SDK 35, NDK 27.2.12479018, CMake 3.22.1 et Git. Le wrapper fournit Gradle 8.13 avec Android Gradle Plugin 8.9.1.

```bash
git clone --recurse-submodules https://github.com/CorvusSoftware/CorvusRetro.git
cd CorvusRetro

./gradlew --no-daemon :app:assembleDebug
```

L'APK de développement se trouve dans :

```text
pkg/android/app/build/outputs/apk/debug/app-debug.apk
```

Les modules natifs sont compilés séparément pour préparer les archives téléchargeables. **Ils ne sont pas embarqués dans l'APK de base.**

### Tests

```bash
make test
make test SANITIZE=ON
./gradlew --no-daemon :app:lintDebug :app:assembleDebugAndroidTest
```

Ces commandes couvrent les tests natifs, les vérifications mémoire et le projet Android. Pour la validation des archives, des téléchargements et des builds de publication, voir le [guide de validation](docs/validation-android.md).

## Documentation

| Guide | Sujet |
| :-- | :-- |
| [Prise en main et navigation](docs/navigation.md) | Menus et réglages |
| [Commandes tactiles](docs/controls.md) | Boutons, gestes et orientation |
| [Téléchargement des cœurs](docs/individual-cores.md) | Modules natifs et installation |
| [RetroAchievements](docs/retroachievements.md) | Compte, succès et limitations |
| [Architecture](docs/repository-structure.md) | Organisation des composants |
| [Validation Android](docs/validation-android.md) | Compilation, tests et contrôles |

Les catalogues sont maintenus dans **[CorvusRepository/catalog](https://github.com/CorvusSoftware/CorvusRepository/tree/main/catalog)**. CorvusRetro découvre automatiquement chaque JSON publié dans ce dossier et affiche ses cœurs compatibles, après validation stricte. Aucune liste de fichiers n'est intégrée dans le frontend.

> **Compatibilité :** les anciens catalogues ont été supprimés. Les APK qui utilisaient leurs anciennes URL doivent être mis à jour. La version 0.15.1 découvre les catalogues de fournisseurs dans CorvusRepository.

---

### Licence

CorvusRetro est distribué sous licence **[AGPL-3.0-only](LICENSE)**, © 2026 CorvusSoftware.

[CorvusCore](https://github.com/CorvusSoftware/CorvusCore) conserve sa licence AGPL-3.0-only. [CorvusAPI](https://github.com/CorvusSoftware/CorvusAPI) et [rcheevos](https://github.com/RetroAchievements/rcheevos) conservent leurs licences MIT respectives.

*Aucun jeu ni BIOS commercial n'est distribué. CorvusRetro n'est affilié ni à Nintendo, ni à RetroAchievements, ni à RetroArch.*
