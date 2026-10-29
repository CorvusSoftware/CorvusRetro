# Validation Android et native

Le build et les tests sont indépendants de l'agencement du projet Gradle.
Le code portable est construit par le CMake racine ; le wrapper `./gradlew`
exécute le projet de `pkg/android/`.

## Tests hôtes

```sh
make test
make test SANITIZE=ON
cmake -S . -B build/corefree -DCORVUS_OFFICIAL_CORES=
cmake --build build/corefree --target native_nocore_test
(cd build/corefree && ./native_nocore_test)
python3 tools/test_release_workflows.py
```

Vérifier les générations de batterie, les identifiants historiques GB/GBC/GBA,
les ajouts de moteurs, le remplacement d'un module pendant un jeu et le registre
vide. Les tests de mémoire instrumentent le code portable original.

## Build Android

```sh
./gradlew --no-daemon :app:assembleDebug :app:lintDebug :app:assembleDebugAndroidTest
python3 tools/check_core_apks.py \
  --apksigner "$ANDROID_HOME/build-tools/35.0.0/apksigner" \
  --aapt "$ANDROID_HOME/build-tools/35.0.0/aapt" \
  --zipalign "$ANDROID_HOME/build-tools/35.0.0/zipalign"
```

La base `pkg/android/app/build/outputs/apk/debug/app-debug.apk` doit contenir
uniquement `libcorvusretro.so` pour ARM64 et x86_64, avec des segments ELF
de 16 Ko. Le build Gradle cible seulement `corvusretro` et ne produit pas
les archives de cœurs.

La compilation autonome, la validation des bibliothèques et des ZIP,
les contrôles des fournisseurs et la promotion des catalogues sont dans
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository/blob/main/docs/publication.md).
Ils n'utilisent ni le projet Gradle ni la signature de l'application.

## Qualification de l'application

Tester sur Android la base vide, le téléchargement GBA seul puis GB/GBC cumulés,
la liste immédiatement actualisée, le chargement et le déchargement, la reprise
après recréation pendant un transfert et les téléchargements invalides. Aucun
installateur de paquet ne doit être lancé pour un cœur.

Vérifier également portrait/paysage, marges système, commandes multitouch et
menu central, reprise des jeux existants, historique, thèmes et batterie GBA
historique suffixe-2. Un échec d'écriture de batterie conserve le moteur.

La phase `navigation` de `RetroAchievementsInstrumentation` vérifie sur un vrai
cœur GB le lancement refusé sans cœur, la fermeture du jeu gardant le cœur,
la réouverture puis le déchargement explicite. Elle capture l'icône adaptative,
l'historique seul et les interrupteurs ON/OFF dans les trois thèmes. Utiliser
`-e orientation landscape` pour le paysage. Les tests attendent le focus de la
fenêtre avant d'injecter une commande après une rotation et lisent les réglages
via le service Android, qui conserve les types historiques des préférences.

L'instrumentation utilise des programmes de test originaux et un transport
HTTPS déterministe. Un passage de ces tests ne prouve pas une connexion au
service public depuis un téléphone. Distinguer explicitement build, tests hôtes,
émulateur Android, exécution ARM64 et appareil physique.

## Publication

Le build signé doit employer le certificat durable de la base déjà distribuée.
Le publisher exige une CI réussie du commit exact, la provenance des dépendances,
les empreintes de tous les fichiers et le tag correspondant à la version APK.
Il publie l'APK qualifié sans reconstruire ou publier de cœurs.
CorvusRepository publie ses propres archives et prépare leur catalogue
pour revue après vérification des octets publics. Les numéros de version
des moteurs et de l'application sont indépendants.

## Qualification 0.18.0

La [release v0.18.0](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.18.0)
correspond au frontend `67385be702dbf146f18b4e529b8559c9e38d0b32`, à CorvusCore
`063cf1f674788aa5913cebe184457a10b5946edd` et à CorvusAPI
`39c927c5a1a55b78ab55758a41cd16b0971c4684`.

- 12 tests natifs, puis 12 tests ASan/UBSan réussis, et frontend sans cœur.
- Build debug/release, lint et APK d'instrumentation réussis pour ARM64 et x86_64.
- Six parcours `configuration` sur émulateur Android 10/API29 x86_64 :
  GB/GBC/GBA en portrait et paysage, 782 contrôles. Ils couvrent les deux jeux,
  le lancement lié, les options mémorisées, le reset, les états désactivés,
  le joueur contrôlé et la fermeture gardant le cœur.
- Mise à jour signée 0.17.1/code21 vers 0.18.0/code22 acceptée sans
  désinstallation : même UID, thème ASUS et historique conservés, sans crash
  applicatif observé. Ce profil signé ne contient aucun cœur installé.
- APK public et six ZIP téléchargés et comparés aux fichiers qualifiés :
  signature durable, version, empreintes, bibliothèques, ABIs et segments de
  16 Ko contrôlés. La base contient uniquement le frontend.
- Les trois ZIP x86_64 publics ont aussi été installés par le service de test ;
  le parcours GBA en paysage a été rejoué avec ces modules de production :
  134 contrôles supplémentaires réussis.

CI exacte [40740763674](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/40740763674),
build signé [40741288414](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/40741288414),
publication [40742906577](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/40742906577).
APK public : 1 302 756 octets ; SHA-256 :

```text
ea0d3a5f7b0b027cc99b65c5d17df6ba9d723a6914df68b36063b9e950477821
```

Construire l'instrumentation avec
`-PcorvusTestRunner=com.corvussoftware.corvusretro.RetroAchievementsInstrumentation`,
installer la base et son APK de test, puis fournir les trois ZIP x86_64 dans
le dossier de fixtures du terminal :

```sh
adb shell am instrument -w -e phase configuration -e system 0 \
  -e orientation portrait -e fixture_dir /chemin/fixtures \
  com.corvussoftware.corvusretro.test/com.corvussoftware.corvusretro.RetroAchievementsInstrumentation
```

Répéter avec `system` 1 et 2, puis `orientation landscape`. Les fixtures sont
des programmes originaux ; aucun jeu commercial ni BIOS n'est distribué.
Les alertes ANR de démarrage de System UI/Quickstep ont été identifiées et
capturées avant reprise des essais. Le refus SELinux de `link()` lors d'une
édition d'options a été reproduit et corrigé par une copie synchronisée puis
un renommage atomique. Les tests reproduisent cette interdiction et les
erreurs de synchronisation, y compris après publication du fichier.

Les états groupés et les succès pendant le link sont indisponibles dans les
modules officiels. Le link GBA reste expérimental. Aucun runtime ARM64,
appareil physique ou compte RetroAchievements personnel n'est revendiqué.
