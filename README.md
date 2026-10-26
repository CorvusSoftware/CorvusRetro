# CorvusRetro

Frontend Android original de CorvusSoftware, sous AGPL-3.0-only.
Copyright (C) 2026 CorvusSoftware.

Les menus, commandes, thèmes et pilotes sont écrits en C11. Android héberge une
NativeActivity avec EGL/OpenGL ES2 et AAudio. Java fournit les services Android :
stockage, sélection de fichiers, polices et réseau. Les moteurs CorvusCore C++20
communiquent avec le frontend par l'interface publique CorvusAPI.

## Utilisation

[Télécharger l’APK signé 0.13.0](https://github.com/CorvusSoftware/CorvusRetro/releases/download/v0.13.0/CorvusRetro-0.13.0-base-release.apk).

La base ne contient aucun moteur. Ouvrir **Charger un cœur > Téléchargeur de cœurs**
et choisir CorvusGB, CorvusGBC ou CorvusGBA. L'archive native vérifiée est enregistrée
puis le cœur devient immédiatement disponible dans **Charger un cœur**.
**Décharger le cœur** libère son instance en mémoire et conserve son fichier.
Le téléchargement ne lance pas d'installateur APK et ne redémarre pas l'application.

Les thèmes sont CorvusRetro, NVIDIA Shield et ASUS. Les contenus, l'historique,
les réglages et les sauvegardes conservent leurs identifiants lors des mises à jour.
Aucun jeu commercial ni BIOS constructeur n'est distribué.

## RetroAchievements

Dans **Réglages > RetroAchievements**, saisir l’identifiant et le mot de passe,
puis choisir **Se connecter**. Le compte utilise le mode normal ; les états
instantanés restent disponibles. **Menu rapide > Succès** affiche les succès du jeu.
Les cœurs officiels doivent être à jour en version 0.14.1.

La bibliothèque officielle rcheevos identifie le jeu et évalue ses succès depuis
la mémoire exposée par CorvusAPI. Le mot de passe n'est jamais enregistré ; le
jeton et les succès en attente d'envoi sont chiffrés avec Android Keystore.
Les envois interrompus reprennent après retour au premier plan, même si le jeu a
été fermé. La déconnexion efface la session et les envois restants.
Voir [l'intégration et ses validations](docs/retroachievements.md).

## Structure du dépôt

L'organisation reprend les frontières des composants de RetroArch. Les composants
propres à CorvusRetro sont originaux ; les bibliothèques externes gardent leurs licences.

| Dossier | Responsabilité |
| --- | --- |
| `menu/` | Menus, navigation, thèmes et état de l'interface |
| `frontend/` | Contrat des services et pilotes de plateforme |
| `gfx/` | Rendu graphique et géométrie du viewport |
| `audio/` | Pilotes de sortie audio |
| `input/` | Disposition des commandes et géométrie tactile |
| `cheevos/` | Client RetroAchievements natif, succès et protocole officiel |
| `tasks/` | Échanges asynchrones entre services Android et thread natif |
| `cores/` | Registre CorvusAPI, cycle de vie, états et sauvegardes |
| `deps/` | Dépendances versionnées, avec leurs notices originales |
| `pkg/android/` | Projet Gradle, manifeste, ressources et services Android |
| `core-repository/` | Métadonnées des téléchargements publics |
| `tools/` | Validation et publication des artefacts |
| `tests/` | Régressions natives et programmes de test originaux |
| `docs/` | Contrats, navigation et procédures de validation |

Voir [la structure et ses références](docs/repository-structure.md),
[les commandes](docs/controls.md), [la navigation](docs/navigation.md),
[les thèmes](docs/themes.md) et [le téléchargeur](docs/individual-cores.md).

## Construire et vérifier

Prérequis : JDK17 ou supérieur, Android SDK35, NDK27.2.12479018 et CMake3.22.1.
Le wrapper Gradle8.13 utilise Android Gradle Plugin8.9.1.

```sh
git submodule update --init --recursive
./gradlew --no-daemon :app:assembleDebug :app:lintDebug :app:assembleDebugAndroidTest
make test
make test SANITIZE=ON
python3 tools/build_core_archives.py --variant debug --tag v0.13.0 \
  --output build/native-cores --catalog build/native-cores.json
python3 tools/validate_core_archives.py
python3 tools/test_core_archives.py
python3 tools/test_release_workflows.py
```

Le wrapper racine délègue au projet `pkg/android/`. L'APK est produit dans
`pkg/android/app/build/outputs/apk/debug/app-debug.apk`. Les six bibliothèques
individuelles sont construites directement dans
`pkg/android/app/build/native-cores/Debug/<ABI>/`, puis empaquetées en ZIP.
Elles sont explicitement exclues de la base Android. Les anciens projets de
split APK et leurs outils de publication ont été retirés.

La CI vérifie le frontend sans moteur, les régressions natives, les catalogues,
le lint Android, l'APK et les six entrées natives. La publication réutilise un
build signé du commit exact ayant passé la CI, vérifie sa provenance et prépare
le catalogue sur une branche de revue. Les empreintes et URLs proviennent des
fichiers réellement publiés.

Le catalogue courant est `core-repository/cores-v3.json`. Les deux anciens
catalogues restent servis pour les applications déjà publiées ; ils ne sont
plus utilisés pour construire de nouvelles distributions.

Les moteurs actuels utilisent CorvusCore 0.14.1 et le header public unique `corvusapi.h`, avec l’extension mémoire RetroAchievements facultative.
