# CorvusRetro

Application Android originale de **CorvusSoftware**, avec navigation inspirée de
**MaterialUI de RetroArch** et les moteurs maison
[CorvusCore](https://github.com/CorvusSoftware/CorvusCore).
Code applicatif sous **AGPL-3.0-only**, Copyright (C) 2026 CorvusSoftware.

## Version 0.4

- Listes tactiles et onglets Menu principal, Listes de lecture et Réglages.
- **Charger un cœur** crée un moteur natif, même sans contenu ; **Décharger le cœur** le libère et revient à **Aucun cœur**.
- Cœurs intégrés : **CorvusGB**, **CorvusGBC**, **CorvusGBA (expérimental)**, version 0.14.0.
- Thème **NVIDIA Shield** par défaut : fond noir, en-tête anthracite, icônes et sélection vertes. Thème **MaterialUI sombre** bleu disponible dans les réglages.
- Icônes vectorielles originales de style MaterialUI ; module de cœur avec une tête de corbeau découpée, également utilisée comme identité de l'application.
- Import local, listes par console, historique, audio stéréo et commandes tactiles.
- Menu rapide : reprendre, redémarrer, sauvegarder ou charger un état et fermer le contenu.
- Sauvegardes batterie automatiques et suspension de l'émulation en arrière-plan.
- APK debug Android 8.0+, ARM64 et x86_64.

Captures réelles sur émulateur Android 10 :

<p>
  <img src="docs/screenshots/menu.png" width="240" alt="Cœur chargé et thème NVIDIA Shield" />
  <img src="docs/screenshots/cores.png" width="240" alt="Sélection des cœurs CorvusCore" />
  <img src="docs/screenshots/settings.png" width="240" alt="Réglages du thème et du lecteur" />
</p>

Autres captures : [lecteur](docs/screenshots/player.png),
[liste de contenus](docs/screenshots/playlist.png), [menu rapide](docs/screenshots/quick-menu.png),
[aucun cœur](docs/screenshots/menu-empty.png), [thème sombre bleu](docs/screenshots/theme-dark.png).

## Charger et décharger

L'application démarre avec **Aucun cœur**. Choisir **Charger un cœur** initialise
le moteur sélectionné. Son nom apparaît dans la barre d'état. Il peut être
remplacé par un autre cœur ou libéré avec **Décharger le cœur**.
Ce choix est un état de la session : après arrêt du processus, aucun cœur n'est chargé.

**Charger du contenu** ouvre le sélecteur Android. Les extensions `.gbc` et `.gba`
sélectionnent le système correspondant ; `.gb` utilise le cœur GB/GBC choisi,
ou GB si GBA était sélectionné. Sans cœur chargé, les contenus sont associés
à leur console. Les fichiers doivent être décompressés et limités à 32 Mio.

Les listes et l'historique peuvent lancer directement un contenu, avec chargement
automatique de son moteur. Le menu rapide s'ouvre par son icône ou Retour Android.
**Fermer le contenu** enregistre la batterie, libère le moteur et revient aux menus.
Un changement de cœur demandé depuis une autre activité est refusé si un lecteur
en est encore propriétaire : fermer d'abord le contenu.

Les fichiers importés, la batterie et les états résident dans le stockage privé.
La batterie est enregistrée périodiquement, à la pause et à la fermeture.
Un emplacement d'état par contenu est associé à la version de CorvusCore.
Une mise à jour avec la même signature conserve les données ; la désinstallation
les supprime. L'export de sauvegardes reste à ajouter.

Commandes tactiles : directions, A/B, Start/Select et L/R sur GBA.
Événements clavier/manette pris en charge : flèches, Z/X, Entrée/Espace,
A/B, Start/Select et L1/R1. Le son et la veille se règlent avant le lancement.
Le thème se choisit dans **Réglages → Thème de l'interface** et reste mémorisé.

La compatibilité dépend de CorvusCore. GBA reste expérimental.
Aucun jeu commercial ou BIOS constructeur n'est fourni.

## Construire et tester

Prérequis : JDK 17 ou 21, SDK Android 35, Build Tools 35.0.0,
NDK 27.2.12479018 et CMake 3.22.1. Gradle 8.13 et AGP 8.9.1 sont fixés.

```sh
git clone --recurse-submodules https://github.com/CorvusSoftware/CorvusRetro.git
cd CorvusRetro
# Pour un clone existant :
git submodule update --init --recursive
export ANDROID_HOME=/chemin/vers/android-sdk
./gradlew :app:assembleDebug :app:lintDebug
# Tests natifs sur Linux, avec JAVA_HOME vers un JDK complet :
make test CMAKE="$ANDROID_HOME/cmake/3.22.1/bin/cmake"
```

APK : `app/build/outputs/apk/debug/app-debug.apk`, signé avec la clé Android debug.
Android Studio peut ouvrir la racine du projet avec ces versions du SDK/NDK/CMake.
Les preuves d'exécution et leurs limites sont dans [la validation Android](docs/validation-android.md).

## APK automatique

Le workflow [Android APK](https://github.com/CorvusSoftware/CorvusRetro/actions/workflows/android.yml)
s'exécute à chaque push sur `main`, sur les pull requests et manuellement.
Il initialise CorvusCore, contrôle la ponctuation du dépôt, teste les vrais moteurs
via JNI, construit l'application et exécute lint, puis vérifie signature et alignement 16 Ko.

Téléchargement : **Actions → Android APK → exécution réussie → Artifacts →
CorvusRetro-debug-…**, puis extraire l'APK du ZIP. Conservation : 14 jours.

## Architecture

```text
app/                      Menus Java, lecteur, audio et pont JNI C++
third_party/CorvusCore/    Moteurs C++20 et SDK C, sous-module figé
tests/                    Tests Java/JNI et génération de contenus synthétiques
```

`CoreManager` possède le cœur sans contenu et transfère son handle à un seul
`EmulationSession`. Création, émulation, sauvegardes et destruction passent par
une même file FIFO. `MainActivity` observe l'état chargé ; `PlayerActivity`
présente les images et commandes. Les fichiers et préférences restent dans `GameLibrary`.

CorvusCore 0.14.0 est figé au commit `38829d4935bbdcf4b13261714352362065b78551`.
Le pont vérifie son API C préliminaire version 0. Toute mise à jour exige une nouvelle
validation. Les sources du sous-module restent intactes.

## Inspiration et conventions

Le pilote MaterialUI, son thème Shield et les ressources GLUI ont été étudiés
pour la navigation, les couleurs et les métaphores d'icônes. Les icônes et le
code applicatif sont originaux. Voir [les références précises](docs/retroarch-reference.md).

Tout fichier source original porte :

```c
// CorvusRetro - CorvusSoftware
// Copyright (C) 2026 CorvusSoftware
// SPDX-License-Identifier: AGPL-3.0-only
```

Les tirets Unicode longs sont interdits dans les fichiers du dépôt parent.
Les licences et crédits tiers restent conservés, notamment ceux de CorvusCore
et du wrapper Gradle. Les notices embarquées utilisent le tiret ASCII pour les
plages d'années, sans modification des crédits. Elles sont consultables hors ligne
via **Réglages → Licence et crédits**.
