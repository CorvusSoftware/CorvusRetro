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
./gradlew --no-daemon :app:testDebugUnitTest :app:assembleDebug :app:lintDebug :app:assembleDebugAndroidTest
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

Les phases `playback` et `core_removal` du même runner couvrent la continuité
avec le volet de notifications, la pause HOME et le retour à la surface EGL,
puis la confirmation, l'annulation, la suppression avec un jeu chargé et la
réinstallation. Construire le runner avec
`-PcorvusTestRunner=com.corvussoftware.corvusretro.RetroAchievementsInstrumentation`
et fournir `-e phase playback` ou `-e phase core_removal`, ainsi que
`-e fixture_dir` pointant vers les archives de test officielles déjà vérifiées.
Le journal de playback debug confirme des trames croissantes et AAudio actif
pendant la perte de focus. Cela ne constitue pas une écoute physique.

L'instrumentation utilise des programmes de test originaux et un transport
HTTPS déterministe. Un passage de ces tests ne prouve pas une connexion au
service public depuis un téléphone. Distinguer explicitement build, tests hôtes,
émulateur Android, exécution ARM64 et appareil physique.

### Parcours de la nouvelle interface

Pour les menus refondus, le runner de référence est désormais
`InterfaceRedesignInstrumentation`, et non les coordonnées de navigation de
l'ancienne interface dans `RetroAchievementsInstrumentation`.

```sh
./gradlew --no-daemon \
  -PcorvusTestRunner=com.corvussoftware.corvusretro.InterfaceRedesignInstrumentation \
  :app:assembleDebug :app:assembleDebugAndroidTest
adb install -r pkg/android/app/build/outputs/apk/debug/app-debug.apk
adb install -r pkg/android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w -e orientation portrait -e fixture_dir "$FIXTURE_DIR" \
  com.corvussoftware.corvusretro.test/com.corvussoftware.corvusretro.InterfaceRedesignInstrumentation
```

`FIXTURE_DIR` désigne un dossier Android contenant les trois archives officielles
GB/GBC/GBA x86_64 `0.15.0` déjà vérifiées. Exécuter aussi avec
`-e orientation landscape`. Les programmes originaux sont créés par le runner,
qui isole leurs identifiants et restaure les préférences modifiées. Le transport
de catalogue est contrôlé ; l'installateur conserve les contrôles de taille,
SHA-256, ZIP, ELF et permissions privées. Les captures restent dans les fichiers
privés de l'application de test et ne doivent pas être ajoutées au dépôt.

Le parcours couvre les cinq onglets, recherche et filtres, clavier réel et
tickets périmés, lancement avec cœur compatible, menu rapide, états réels et
actions, anciens états, réglages, trois thèmes, suppression et réinstallation
individuelle d'un cœur sans perte de jeux ou d'états.

`FrontendServicesInstrumentation` constitue un runner distinct pour les services
d'images et de partage, avec le même mécanisme `-PcorvusTestRunner`.

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
modules officiels. Aucun runtime ARM64,
appareil physique ou compte RetroAchievements personnel n'est revendiqué.

## Distribution indépendante des cœurs

Le [workflow autonome CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository/actions/runs/40802383314)
a construit et publié le lot [v0.15.1](https://github.com/CorvusSoftware/CorvusRepository/releases/tag/v0.15.1),
contenant les six modules CorvusCore 0.15.0 depuis Core `063cf1f` et API
`39c927c`, sans construire l'application. Les anciennes archives restent accessibles.

Les six ZIP publics ont été téléchargés et vérifiés : tailles, SHA-256,
bibliothèque unique, architecture, segments ELF de 16 Ko, quatre exports et
dépendances système uniquement. Ils sont identiques aux fichiers standalone
utilisés pour les essais Android. Le catalogue a été promu après contrôle,
avec sa CI et le contenu `raw` de `main` vérifiés.

Sur Android API29 x86_64, les trois parcours paysage sur les nouveaux modules
publics passent : GB 127, GBC 127, GBA 131, soit 385 contrôles. Les empreintes
des modules installés concordent avec les archives ; 24 captures couvrent les
options, le reset, les deux joueurs et la fermeture conservant le cœur.
Le démarrage TCG a rencontré un ANR System UI avant les essais ; il a été
identifié et conservé, sans modification des permissions. Aucun crash applicatif
ni nouvel ANR pendant les essais.

La séparation côté frontend a passé huit tests de publication, Gradle
Debug/Release/lint/testAPK et les contrôles de base sans moteur. La CI
`40801908523` et la qualification signée APK seul `40802221654` sont réussies.
Ces preuves couvraient les archives installées directement par fixtures, pas
le parcours de lecture du catalogue officiel. Le filtre supplémentaire du
client 0.18.0 a ensuite été identifié comme incompatible avec les URL centrales.
Le flux historique est restauré ; 0.18.1 corrige ce filtre et découvre
`catalog/v1/`. Les régressions exécutent maintenant le vrai lecteur Java.
Aucun essai sur téléphone physique ni runtime ARM64 n'est revendiqué.

## Transferts de sauvegardes

`SaveFilesTest` vérifie les noms, les octets bruts, la taille limite, les
annulations et les fichiers temporaires exclusifs. Compiler le runner dédié :

```sh
./gradlew --no-daemon :app:assembleDebug :app:assembleDebugAndroidTest \
  -PcorvusTestRunner=com.corvussoftware.corvusretro.SaveTransfersInstrumentation
```

Après installation de la base debug et de l'APK de test, exécuter :

```sh
adb shell am instrument -w \
  com.corvussoftware.corvusretro.test/com.corvussoftware.corvusretro.SaveTransfersInstrumentation
```

Ce runner exerce le vrai ContentResolver avec un fournisseur limité à des
documents synthétiques dans l'APK de test. Il ne charge pas de cœur et ne
remplace pas la qualification du parcours complet depuis un jeu. Ce fournisseur
est absent des APK de l'application et ne doit jamais être ajouté au manifeste
principal. Les tests natifs vérifient séparément les moteurs réels et la
persistance avant et après import, y compris en câble link.

## Qualification 0.19.0

Source de l'APK : `dac599e42e1bfdd26a49d93e77dcf0d2141d8da2` (PR #17).
Les dépendances restent CorvusCore `063cf1f674788aa5913cebe184457a10b5946edd`,
CorvusAPI `39c927c5a1a55b78ab55758a41cd16b0971c4684` et rcheevos
`1433173220a7eaede6a9ed7a18e94117be1821e0`.

- 13 tests natifs puis 13 tests ASan/UBSan réussis, et 2 tests sans cœur.
  Les compléments link P1 passent également en normal et avec sanitizers.
- Tests d'import avec les vrais moteurs GB/GBC/GBA, solo et link P1/P2 :
  sauvegardes exactes, autre joueur inchangé, options conservées, backup de la
  SRAM live même sans fichier existant, rollback des erreurs d'écriture.
- Arrêts réels de processus via `fork` à chaque frontière `fsync` de l'import,
  puis récupération cohérente des deux joueurs et du backup. Ces tests ne
  simulent pas une panne matérielle ou une perte d'alimentation du stockage.
- 8 tests JVM, build debug/release et instrumentation, lint et contrôle de la
  base sans cœur réussis. Le fournisseur de documents synthétiques est absent
  du manifeste de la base distribuée.
- `SaveTransfersInstrumentation` : 38 contrôles API29 x86_64 réussis sur le vrai
  ContentResolver, dont octets `.sav`/`.srm`, taille/extension, annulations,
  destruction et réponses périmées. L'export vers un document existant plus long
  a été reproduit rouge puis corrigé avec le mode de troncature `wt`.
- Phase `savetransfer` de `RetroAchievementsInstrumentation` : 546 contrôles
  API29 x86_64 (GB portrait136/paysage136, GBC portrait137/paysage137).
  Elle vérifie le menu natif, JNI, les exports de SRAM, l'import confirmé et le
  rechargement réel des octets par le cœur, le backup, les annulations, le cœur
  conservé après fermeture et Sous-système retiré après déchargement.
  ActivityMonitor remplace uniquement la sélection dans DocumentsUI ; le moteur
  et les accès au fournisseur de documents sont réels.
- Catalogue : 40 contrôles des réponses publiques gelées, avec les trois cœurs
  affichés dans le téléchargeur. Cette preuve distingue le parseur et l'interface
  d'une connexion Internet depuis un téléphone.
- CI du commit fusionné : [41290648163](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41290648163).
  Build signé qualifié : [41290806524](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41290806524).
- Mise à jour signée 0.18.1/code23 vers 0.19.0/code25 sans désinstallation :
  même UID, dossier et date d'installation initiale, thème ASUS et historique
  Corvus-preserved.gba conservés. Les pixels de la ligne d'historique sont
  identiques ; aucun nouveau crash applicatif observé. Ce profil signé ne
  contient aucun cœur installé ; les essais avec cœur utilisent le profil debug.

Certificat durable SHA-256 :
`cb431e5abf3ae73ddd5c706a9c485ff6e627be2135bb2557b6191f04ac3a5211`.
APK qualifié SHA-256 :
`24d07ad24098a0f9d4ac697a93d9f52be57562ccc032d784d0f784f8f4bef063`.
Les sources et sommes de contrôle de l'artefact signé ont été vérifiées, ainsi
que les deux bibliothèques frontend, les segments ELF et l'alignement 16 Ko.

Aucune exécution ARM64 ou sur appareil physique n'a été réalisée pour ce lot.
Les épisodes SystemUI non réactif au démarrage des émulateurs ont été inspectés
puis traités via leur bouton Attendre avant de relancer les parcours inchangés.

La [release v0.19.0](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.19.0)
a été publiée par [41291144366](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41291144366).
L'APK a été retéléchargé depuis son URL publique et comparé octet par octet au
fichier signé qualifié : identité exacte, SHA-256 ci-dessus.

L'essai Internet direct avec l'APK signé dans cet émulateur n'est pas validé :
le catalogue reste indisponible, et une sonde utilisant la même connexion HTTPS
Android standard échoue avec `java.net.ConnectException` vers
`raw.githubusercontent.com/2606:50c0:8001::154:443`. Depuis le worker cloud,
les deux URL officielles répondent HTTP200 et le JSON officiel est identique
aux données du test gelé. Cette limite de connexion de l'émulateur reste
distincte de la correction du parseur. Aucune modification de TLS ou du
stockage de confiance n'a été faite pour cet essai. Le téléchargement Internet
depuis un téléphone reste à qualifier.

## Tests des paramètres

Compiler et installer la base debug ainsi que le runner dédié :

```sh
./gradlew --no-daemon :app:assembleDebug :app:assembleDebugAndroidTest \
  -PcorvusTestRunner=com.corvussoftware.corvusretro.SettingsInstrumentation
adb install -r pkg/android/app/build/outputs/apk/debug/app-debug.apk
adb install -r pkg/android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w -e orientation portrait \
  com.corvussoftware.corvusretro.test/com.corvussoftware.corvusretro.SettingsInstrumentation
adb shell am instrument -w -e orientation landscape \
  com.corvussoftware.corvusretro.test/com.corvussoftware.corvusretro.SettingsInstrumentation
```

Ce parcours ouvre les six catégories avec les événements Android réels, modifie
les valeurs, vérifie leur persistance et recrée l'activité pour vérifier que le
menu natif les recharge. Il restaure les seules préférences touchées. Aucun
cœur sélectionné ni compte réel n'est requis. Les images sont conservées sous
`files/test-settings-*.png` dans le profil de test.

La phase `settings_media` de `RetroAchievementsInstrumentation`, avec un
module GB installé séparément et un programme original de test, vérifie les
réglages appliqués à une partie. Elle mesure la vidéo entière en pixels
physiques et trouve le bouton dessiné dans la capture avant d'appuyer dessus :
son feedback doit s'éclaircir puis revenir à l'intensité de repos choisie.
L'attente bornée porte sur l'image effectivement présentée par l'émulateur.

## Qualification 0.20.0

Source APK : `1de61ddc6da4c30ffb7021c5abec93e6843d5fca` (PR #19).
CorvusCore, CorvusAPI et rcheevos restent aux mêmes révisions que la release0.19.0.

- 15 tests natifs et 15 tests ASan/UBSan réussis, avec détection des fuites ;
  frontend sans cœur, 8 tests JVM, builds debug/release/instrumentation et lint
  réussis. La base contient seulement les deux bibliothèques frontend ARM64/x86_64.
- Les tests média vérifient le gain PCM, les limites physiques de l'échelle
  entière, l'intensité au repos, les géométries agrandies/déplacées et les zones
  touchées. Les dimensions par défaut conservent la disposition existante.
- Les tests de menus vérifient les six catégories, les valeurs sauvegardées et
  rechargées, les défauts en cas de valeur invalide, les restaurations et les
  retours depuis le Menu rapide et depuis les réglages globaux.
- Les tests de sauvegardes GB/GBC/GBA couvrent rétention1/2/3, export des copies,
  journaux v1/v2, interruptions aux frontières fsync, états automatiques séparés
  et échec d'écriture conservant le jeu. La disponibilité d'un état automatique
  suit le même contrat de récupération `.bak` que son chargement.
- Les tests des manettes couvrent profils par défaut, touches réservées ou
  dupliquées, affectation, restauration, générations et déconnexion. Les noms
  réels proviennent d'InputManager ; aucune manette physique n'a été testée.
- 710 contrôles Android API29 x86_64 : 323 paramètres en portrait, 323 en paysage,
  puis 32 contrôles avec un jeu GB original dans chaque orientation. Ceux-ci
  observent une vidéo320x288 en pixels entiers et appuient sur le bouton A
  détecté dans la capture : feedback visible puis retour à l'intensité de repos.
  Les captures des catégories, des valeurs et des commandes ont été inspectées.
- La compilation GCC13/Fortify est vérifiée sans neutraliser les diagnostics.
  La lecture du journal reste bornée par la capacité réelle du record et valide
  séparément les tailles des formats244/252.
- Revue indépendante sans défaut bloquant restant.

CI du commit fusionné : [41315920611](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41315920611).
Build signé : [41316133776](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41316133776).
APK qualifié SHA-256 :
`18beba18ca7bf117680faa0143da8a84e348c43b5d7991e1037e05da0d764e53`.
Certificat durable : celui de la qualification0.19.0 ci-dessus. Provenance et
sommes vérifiées, alignement 16 Ko, zéro cœur, fournisseur synthétique absent.

Les tests d'écran utilisent l'émulateur et des programmes originaux. Ils ne
prouvent pas une exécution ARM64, un appareil physique, une manette Bluetooth/USB
ou le réseau du téléchargeur. Les alertes SystemUI de démarrage ont été inspectées
avant Attendre et reprise des tests, sans neutralisation des protections Android.

La mise à jour signée0.19.0/code25 vers0.20.0/code26 est acceptée sans
désinstallation : même UID10116, dossier et date d'installation initiale. Le
thème ASUS et l'historique Corvus-preserved.gba sont conservés ; aucun nouveau
crash applicatif observé. Le profil signé sans cœur montre les six catégories
des réglages après mise à jour. Les parcours de jeu utilisent le profil debug
séparé et les modules installés indépendamment.

La [release v0.20.0](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.20.0)
est publiée par [41316487558](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41316487558).
L'APK public retéléchargé est identique octet par octet au fichier qualifié,
avec le SHA-256 ci-dessus.

### Interface 0.21.0

La PR21 conserve les dimensions et la disposition de 0.20.0. Les icônes de ligne
restent à 32 dp, celles des onglets à 24 dp ; les tailles de texte restent
16/13/19 dp. L'en-tête de 56 dp, les lignes de 76 dp, les onglets de 72 dp en bas
avec leurs libellés et le pied de page conservent leur présentation précédente
dans les deux orientations. Aucun calcul DPI automatique, ligne extensible ou
menu latéral n'est ajouté.

Les 32 icônes originales, le classement des menus et le retrait du libellé
expérimental du GBA restent dans la PR. Le logo APK est conservé. CorvusCore,
CorvusAPI, jeux, sauvegardes et préférences ne changent pas. Les tests antérieurs
de navigation à coordonnées connues sont conservés ; les positions des nouvelles
actions rapides utilisent leur correspondance explicite.

Validation locale de la correction : 15 suites natives et 15 sous ASan/UBSan,
compilation Android debug pour les deux ABI, lint et tests JVM réussis. Le contrôle
APK confirme la base sans cœur et l'alignement 16 Ko. Sur émulateur x86_64,
323 contrôles Android passent en portrait et 323 en paysage, avec captures des
réglages et de leurs sous-menus. La revue indépendante ne relève aucun défaut.
Le dessin, les zones tactiles et le défilement ont aussi été comparés aux sources
de 0.20.0. Aucun test sur téléphone physique n'est revendiqué.

### Publication signée 0.21.0

La PR21 est fusionnée dans `main`. L'APK public provient exactement de
`e6f1157dc04736dcbb25f9e462c9f97d0cdc7e5c`, avec versionCode27. La CI de main
[41443902775](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41443902775),
la construction signée
[41444288324](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41444288324)
et la publication
[41445211694](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41445211694)
sont réussies. Le tag immuable `v0.21.0` désigne ce commit qualifié.

Le certificat conserve l'empreinte
`cb431e5abf3ae73ddd5c706a9c485ff6e627be2135bb2557b6191f04ac3a5211`.
L'APK téléchargé depuis la release publique est identique octet pour octet à
l'artefact signé testé ; SHA-256 :
`890a3912a4e77959e9892ce4a9732dc3d9179bb26c5d112dab376619fc94f0b1`.
Les deux frontends arm64-v8a/x86_64, l'alignement 16 Ko, l'absence de cœur et
l'absence du provider de tests ont été vérifiés. Le MP3 du succès conserve son
hash d'origine malgré le nom de ressource optimisé par AAPT dans l'APK release.

Sur émulateur Android29x86_64, la mise à jour signée0.20.0/code26 vers0.21.0/code27
est acceptée sans désinstallation. UID10116, répertoire de données et première
date d'installation restent identiques. Le thème ASUS, l'entrée d'historique
`Corvus-preserved.gba`, ses textes et leurs positions sont conservés. Les images
d'icônes changent comme prévu. Le menu sans cœur propose toujours le téléchargeur.
Aucun nouveau crash de l'application n'apparaît dans le buffer observé.

Les fonctionnalités de la PR ont aussi été contrôlées sur l'APK debug :
100 contrôles de rasterisation d'icônes,646 de réglages portrait/paysage,
269 des commandes et réglages média avec jeux synthétiques, puis75 du son et
du déblocage réel d'un succès. Les étapes correspondantes et leurs sources sont
décrites dans la mémoire de tâche. La CI finale exécute15 suites natives et
15 sous ASan/UBSan. Aucun test sur téléphone physique, manette physique ou
runtime ARM64 n'est revendiqué.

## Continuité audio et suppression des cœurs, préparation 0.22

Sur l'émulateur Android 10 API29 x86_64, les phases `playback` portrait et paysage passent
chacune 41 contrôles. Le volet de notifications retire le focus sans interrompre
les trames ni l'état AAudio actif. HOME suspend effectivement les trames et le
retour reconstruit la surface avec reprise audio. Le lanceur ramène le retour
HOME en portrait ; seule la phase sous le volet et les commandes ont été
vérifiées en paysage. La phase `core_removal` passe 91 contrôles : annulation,
suppression d'un autre cœur gardant GBA chargé, suppression du cœur GBA chargé,
préservation des empreintes ROM et batterie, historique conservé, réinstallation
et relance immédiate. Les captures portrait et confirmation ont été inspectées.

Les tests hôtes ajoutent la panne de stockage batterie, les trous du registre,
les fichiers partagés, les liens symboliques, le rollback des préférences et les
callbacks audio arrivant après fermeture ou destruction du propriétaire.
Les 17 suites natives passent en mode strict et ASan/UBSan ; les 19 tests JVM,
le frontend sans cœur, le lint, les builds Android debug/release et le contrôle
du paquet debug deux ABI, signature et alignement 16 Ko passent également.
La revue indépendante n'a plus de finding concret après correction des entrées
maintenues lors de la perte EGL et de la durée de vie des callbacks d'erreur.

L'émulateur utilise une sortie audio sans écoute physique. L'état AAudio et
les trames sont observés, pas la qualité sonore sur haut-parleur. La déconnexion
audio est injectée dans le test du pilote hôte ; aucune campagne sur téléphone
physique, Bluetooth ou runtime ARM64 n'est revendiquée. Android reste responsable
de l'atténuation audio pendant les appels et de ses politiques système.

### Publication signée 0.22.0

La [release v0.22.0](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.22.0)
provient du commit de fusion de PR #24
`c97ccaf61563ecc7a6ced6421424a28b572031aa`, versionCode28. Son arbre est identique
à la tête de PR `751c1a7`, dont les parcours fonctionnels sont décrits ci-dessus.
La CI de main
[41493981097](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41493981097),
le build signé
[41494442678](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41494442678)
et la publication
[41495125139](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41495125139)
sont réussis sur ce commit exact.

Le certificat conserve l'empreinte
`cb431e5abf3ae73ddd5c706a9c485ff6e627be2135bb2557b6191f04ac3a5211`.
L'APK public a été retéléchargé et comparé octet pour octet à l'artefact signé
qualifié. SHA-256 :
`9adf6208b05349671855f7b1e53dfb0873d01dcc0b7135d6c7ec9217f57aa4e1`.
Version 0.22.0/code28, frontends arm64-v8a/x86_64, alignement 16 Ko, absence de
cœur et de provider de tests, provenance des trois sous-modules et empreinte du
MP3 fourni ont été vérifiés. Aucune révision de moteur ou d'API n'a changé.

Sur le profil signé Android 10/API29 x86_64 existant, la mise à jour
0.21.0/code27 vers 0.22.0/code28 est acceptée sans désinstallation. UID10116,
dossier de données et première date d'installation restent identiques. Le thème
ASUS et la ligne d'historique `Corvus-preserved.gba` sont conservés ; la région
de cette ligne est identique en pixels. Les captures après mise à jour ont été
inspectées et la liste sans cœur propose toujours le téléchargeur. Aucun nouveau
crash de l'application n'a été observé dans le buffer consulté.

Les contrôles fonctionnels précédents restent ceux de l'émulateur API29 et des
tests hôtes. Aucune écoute physique, connexion Bluetooth ou exécution sur
appareil ARM64 n'est revendiquée. Le tag `v0.22.0` et son APK restent immuables ;
la présente documentation ne modifie pas la source du binaire publié.

## Progression répétée et amorce audio, préparation 0.22.1

Une régression native avec rcheevos réel reproduit le retour temporaire d'un
compteur à zéro puis à sa valeur précédente. Avant correction, la même
progression est réémise et le popup peut revenir après expiration. Le test
vérifie maintenant 240 frames sans réapparition, les historiques indépendants
de deux succès, les véritables augmentations et diminutions, puis les resets,
chargements d'état et rechargements de jeu. Les 321 contrôles du client passent.

Le pilote AAudio est testé avec un calendrier GBA exact de 59,727 Hz présenté
à 60 Hz. La nouvelle régression échoue contre le pilote de v0.22.0, puis passe
avec l'amorce de deux lots PCM. Cinq simulations de 60 s vérifient l'absence de
silence ajouté et l'ordre exact de la stéréo sur toute la plage 16 bits. Les
18 372 contrôles incluent les gains 0/1/37/50/99/100 %, les tailles 548/549 dans
les deux ordres, les limites de file et les reprises. Les compilations API26
ARM64/x86_64, les modes strict et ASan/UBSan passent. Voir
[la référence audio et vidéo](av-retroarch-reference.md).

Sur émulateur Android 10/API29 x86_64, 68 contrôles de la phase `popups` passent
avec un programme GB original : A donne 33/66, trois nouvelles tentatives A
restent sans badge aux captures, Select donne une nouvelle valeur 49/66, puis
B débloque un seul succès avec un seul son. La progression reste silencieuse.
Les captures fraîches ont été inspectées ; le buffer crash est vide avant et
après. Les captures négatives après une attente de heartbeat lente ne suffisent
pas seules à exclure un flash déjà expiré : la preuve de non-répétition vient
du test natif déterministe, complété par la frontière Android.

Le runner collecte ses deux snapshots d'animation avant d'encoder les PNG.
L'encodage mesuré pouvait durer plus de 2 s et laisser expirer une carte pourtant
vue au premier snapshot. Les captures positives suivent directement l'appui ;
les contrôles négatifs et les relâchements attendent une frame supplémentaire.
Les durées et assertions des widgets restent identiques.

Les 17 suites natives strictes et ASan/UBSan, les 19 tests JVM, le frontend sans
cœur, le lint, les builds debug/release et le paquet debug deux ABI/16 Ko passent.
La ROM Pokémon Émeraude n'était pas disponible. Ces essais ne remplacent pas
une écoute sur téléphone ; une dérive prolongée entre horloges ou un frontend
trop lent reste un cas distinct de l'amorce corrigée. Aucune qualification
Bluetooth ou runtime ARM64 n'est revendiquée.

### Publication signée 0.22.1

La [release v0.22.1](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.22.1)
provient du commit de fusion de PR #26 `ea05c867eddc6258999724435023212a0390e2d9`, versionCode29.
Son arbre est identique à la tête de PR `0b03d92`, déjà revue et qualifiée.
La [CI main 41543067388](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41543067388),
le [build signé 41543475863](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41543475863)
et la [publication 41544028480](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41544028480)
sont réussis sur ce commit exact.

Certificat durable inchangé : `cb431e5abf3ae73ddd5c706a9c485ff6e627be2135bb2557b6191f04ac3a5211`.
L'APK public a été retéléchargé et comparé octet pour octet à l'artefact signé
testé. SHA-256 : `c7ca0e7e00646d37ba6a743ce460c942d181033d5d94de90fda1fd994ec93c0e`.
Version0.22.1/code29, frontends arm64-v8a/x86_64, alignement16 Ko, absence de
cœur et de provider de tests, provenance des trois sous-modules et MP3 original
ont été vérifiés. Aucune révision de moteur ou d'API n'a changé.

Sur le profil signé Android10/API29 x86_64, la mise à jour0.22.0/code28 vers
0.22.1/code29 est acceptée sans désinstallation. UID10116, dossier de données
et première date d'installation sont identiques. Le thème ASUS et l'historique
`Corvus-preserved.gba` sont conservés ; la ligne d'historique est identique en
pixels. Les captures ont été inspectées et le menu sans cœur propose toujours
le téléchargeur. Aucun nouveau crash de l'application n'a été observé.

Les régressions progression et amorce audio, 17 suites natives strictes et
ASan/UBSan, 19 tests JVM et 68 contrôles Android sont décrits ci-dessus.
Le test PCM ne remplace pas une écoute sur téléphone ; aucune ROM Pokémon,
écoute physique, qualification Bluetooth ou exécution ARM64 n'est revendiquée.
Le tag et les octets de v0.22.1 restent immuables ; cette documentation ne
modifie pas le binaire publié.

## Dérive audio et sauvegarde périodique hors rendu

Le correctif suivant conserve les moteurs et le PCM, ajuste légèrement les
échéances d'émulation à partir de la file audio et déporte les écritures batterie
périodiques sur un travailleur. Les détails et limites numériques sont décrits
dans [la référence audio](av-retroarch-reference.md#dérive-des-horloges-et-écritures-pendant-le-jeu).

Validation hôte : 19 suites natives strictes et 19 suites ASan/UBSan passent,
ainsi que le frontend sans cœur et les huit tests de publication. La nouvelle
régression audio couvre 38 scénarios contrôlés de dérive prolongée et quatre
coûts CPU ; celle de batterie comprend 321 contrôles avec véritable cœur GBA,
blocage de `fsync`, erreurs disque, générations récentes, restauration, reset,
importation, fermeture et deux joueurs. La revue indépendante du contrôleur,
de la boucle, du worker et de ces tests n'a relevé aucun défaut concret.

La compilation debug et de son instrumentation, ainsi que le lint Android,
passent. Le contrôle de l'APK confirme un frontend pour arm64-v8a et x86_64,
l'alignement 16 Ko, une signature debug et aucun cœur embarqué.
APK debug testé : SHA-256
`b973c0afb49d3c904f769c457ec815db0eba55327413dac8f440a0a58ae2d635`.
APK de tests : SHA-256
`5dbb961f8fe9fecfb049d2f41bb840784719e42fd7530bd0f27916c4b7770cf0`.

L'instrumentation `RetroAchievementsInstrumentation`, phase `playback`, accepte
maintenant `-e system 0`, `1` ou `2`. Sur Android10/API29 x86_64 logiciel,
`-e system 2` passe 48 contrôles en portrait et 48 en paysage :

- Le programme ARM original dessine une image et écrit `0x5a` en SRAM. Son
  fichier de sauvegarde est effacé avant lancement ; un fichier de 32 Ko
  contenant cet octet apparaît pendant le jeu, avant toute pause ou menu.
- Le volet de notifications laisse avancer les trames avec AAudio actif.
- Menu rapide, HOME, retour à l'activité et fermeture passent ; HOME suspend
  effectivement l'émulation et le retour rétablit EGL et AAudio.
- Les marqueurs debug montrent au plus trois trames par rendu et une correction
  bornée à 5 000 ppm. Ils attestent l'exécution du nouveau pilotage, sans prouver
  une sortie sonore continue sur un téléphone.

Aucun nouveau crash n'a été observé. L'émulateur fonctionne avec `-no-audio` et
le programme original n'est pas une ROM Pokémon. Aucune écoute physique,
exécution ARM64, qualification du casque filaire ou du Redmi Note 13 5G n'est
revendiquée. Cette préparation ne modifie ni le tag ni l'APK public de v0.22.1.

### Publication signée 0.22.2

La [release v0.22.2](https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.22.2)
provient de la fusion de PR #28, commit `f83ecd694f5cf373873a011febc910a9a658a2ad`, versionCode 30.
Son arbre est identique à la tête de PR `09e3026`, dont la CI est réussie.
La [CI main 41597282038](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41597282038),
le [build signé 41597582130](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41597582130)
et la [publication 41598126420](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41598126420)
sont réussis pour cette source exacte.

L'APK public retéléchargé est identique octet pour octet à l'artefact signé testé.
SHA-256 : `2d9cf2198f33263e0b9c132bf13d35f9f92207363b0b721d6311164f8b3cec88`.
Le certificat durable reste `cb431e5abf3ae73ddd5c706a9c485ff6e627be2135bb2557b6191f04ac3a5211`.
Version 0.22.2/code 30, Android 8.0+/target 35, frontends arm64-v8a/x86_64,
alignement 16 Ko, zéro cœur embarqué, absence du provider de tests, provenance
des sous-modules et son MP3 original ont été vérifiés. Le texte À propos natif
contient 0.22.2 dans les deux bibliothèques. Les moteurs, l'API et les pins restent
identiques à 0.22.1.

La mise à jour signée 0.22.1/code 29 vers 0.22.2/code 30 a été installée sans
désinstallation sur Android 10/API29 x86_64. UID 10116, dossier de données et
première date d'installation sont conservés, ainsi que le thème ASUS et
l'historique `Corvus-preserved.gba`. La ligne d'historique est identique en
pixels ; les captures avant/après ont été inspectées et le téléchargeur reste
présent sans cœur installé. Aucun nouveau crash de l'application observé.

Les preuves de comportement restent celles décrites ci-dessus : 19 suites
natives strictes et ASan/UBSan, tests JVM, 38 scénarios de dérive contrôlée,
321 contrôles de sauvegarde et 96 contrôles Android GBA. Une écoute sur le
Redmi Note 13 5G avec casque filaire reste à faire ; aucune ROM Pokémon,
écoute physique, campagne Bluetooth ou exécution ARM64 n'est revendiquée.
Le tag et les octets publics de 0.22.2 restent immuables.

## Qualification de la refonte mobile

La refonte de [PR #30](https://github.com/CorvusSoftware/CorvusRetro/pull/30)
couvre les menus de la maquette et le menu rapide. Le rendu du jeu, les profils
tactiles par console, les pilotes audio et les moteurs sont conservés.

- 21 suites natives strictes et 21 suites ASan/UBSan passent, avec les
  régressions historiques et les nouveaux parcours d'interface et d'états.
- 25 tests JVM, compilation de la base et des tests, et lint Android passent.
  Le contrôle APK vérifie zéro cœur, les deux ABI du frontend, l'alignement
  16 Ko et la signature debug.
- `FrontendServicesInstrumentation` passe 31 contrôles sur les vrais services
  Android d'images et de partage privé en lecture seule.
- `InterfaceRedesignInstrumentation` passe ses parcours complets en portrait
  et paysage sur Android 10/API29 x86_64. Ils exercent Entrée, Retour,
  IME_DONE, les anciens tickets rejetés, les filtres, un vrai framebuffer GBA,
  le menu rapide, la création de trois états, renommage, duplication, partage,
  suppression confirmée, restauration, état classique, réglages, trois thèmes
  et installation/réinstallation d'un cœur avec conservation des données.
- Les captures des deux orientations ont été inspectées, notamment les marges
  système, la grille à quatre colonnes en paysage et les panneaux défilants.
  Aucun crash de l'application n'est enregistré sur ces parcours.
- La revue indépendante a été suivie de corrections et d'une nouvelle revue
  sans finding : prévalidation des états avant remplacement de partie,
  générations de saisie et rafraîchissement après une erreur finale d'IO.

La base debug testée a pour SHA-256
`ee15f9cf49893132af016b67637f279d5ddd0f65f8f3f360f321028a6b8a2442`.
Le test paysage a d'abord révélé des coordonnées de test calculées pour deux
colonnes ; elles ont été corrigées avant le parcours réussi, sans changement de
la base APK. Les compteurs de contrôles incluent des boucles de pixels et de
lecture shell ; ils ne représentent pas des milliers de scénarios indépendants.

Ces parcours utilisent des programmes originaux, un transport de catalogue et
d'archives contrôlé, et un émulateur logiciel. Ils ne qualifient ni un appareil
physique, ni l'exécution ARM64, ni le réseau de production, ni une écoute audio.
Aucun changement de version, fusion ou publication d'APK n'est effectué pour
cette préparation de refonte. La release publique 0.22.2 reste inchangée.

## Découpage des modules de menu

L'extraction issue de `e4061e8` sépare la construction des entrées dans
`menu_displaylist.c`, les préférences dans `menu_setting.c` et les opérations
des actions dans `cbs/menu_cbs_ok.c`. Le pilote conserve navigation, entrées,
cycle de vie et coordination du dessin. L'API publique et la structure `cr_ui`
ne changent pas ; CMake partage une seule liste de modules entre le frontend
et le test de registre.

Une comparaison ponctuelle avant/après confirme les corps des 93 fonctions
d'origine, après renommage des symboles privés et extraction exacte de la
lecture initiale des préférences. L'ordre des enums, les spécifications et
tables, ainsi que le compteur unique des transferts, sont conservés. La revue
indépendante des frontières et des déclarations n'a relevé aucun finding.

Les 21 suites natives strictes, les 21 suites ASan/UBSan, le frontend sans cœur,
le build debug et de son instrumentation, et le lint Android passent sur ce
découpage. L'APK contient les deux frontends alignés à 16 Ko et aucun cœur.
La base debug a pour SHA-256
`27e7c0cab1357a0fea454b211cdfbc50846e211e8e46330a37a3eabd81b676ba`.

Les parcours Android complets portrait et paysage passent sur cette base,
avec captures inspectées et aucun crash enregistré. Les limites du
dispositif de validation restent celles de la refonte : émulateur API29 x86_64,
programmes originaux et transport d'archives contrôlé, sans appareil physique,
exécution ARM64, réseau public ou écoute audio qualifiés.

## Proportions de l'accueil

Les anciennes hauteurs fixes laissaient environ 146 dp sous la carte du cœur
dans le viewport 384 x 696 dp. La composition partage désormais sa géométrie
adaptative entre dessin, toucher, focus et défilement. Les régressions vérifient
360 x 640, 384 x 696 et 392 x 824 dp : cinq cartes visibles, marge basse bornée,
cibles exactes et accès au sous-système supplémentaire par défilement.

Les 21 suites natives strictes et les 21 suites ASan/UBSan passent, ainsi que
build/lint Android et les contrôles APK sans cœur, deux ABI et alignement 16 Ko.
Le parcours portrait complet passe avec la nouvelle composition. Après le
dernier ajustement de cadrage, un contrôle ciblé sur l'APK final vérifie les
trois raccourcis, le cœur, le sous-système défilant et la rotation en paysage.
Les captures finales sont inspectées et aucun crash n'est enregistré.
SHA-256 de cette base debug :
`8cbc1fe1da503758fb37d65b121b509439eee3a2d8745a7735eb47c03c58bdc0`.

## Nouveau jeu d'icônes

Les 42 ressources vectorielles sont rasterisées par Android dans l'atlas réel,
sans croissance du bitmap. `IconAtlasInstrumentation` passe 172 contrôles :
formes distinctes, contenu visible, absence de débordement et lisibilité à
24 pixels. La planche de contrôle affiche les trois palettes. Les anciens
32 identifiants restent stables, avec dix nouveaux pictogrammes dédiés aux actions.

Les 21 suites strictes et les 21 suites ASan/UBSan, build debug/test, lint et
contrôles APK passent. Sur la même base, le parcours Android portrait complet
exerce accueil, bibliothèque, cœurs, états et leurs actions, réglages et les trois
thèmes. Les captures sont inspectées et aucun crash n'est enregistré. La revue
indépendante confirme la correspondance C/Java, les UV, le marqueur blanc et
les bornes du cache de caractères avant la base464.

SHA-256 de la base debug qualifiée :
`e6f55d97afd477f5221b104d541cc0219f411348522621b24d157e27b00e95e2`.
Il s'agit d'essais sur émulateur API29 x86_64 avec programmes originaux et
transport contrôlé, sans nouvelle qualification de téléphone physique ou ARM64.

## Correspondance des icônes et des actions

Les 48 pictogrammes sont associés aux objets et actions décrits dans
[le guide des icônes](icons.md). Les 42 IDs existants et l'allocation de l'atlas
restent stables. Les tests natifs contrôlent aussi les symboles réellement
dessinés pour les catégories vides et le déchargement du cœur.

Les 21 suites strictes et les 21 suites ASan/UBSan passent, ainsi que build/lint,
base sans cœur, deux ABI et alignement 16 Ko. L'atlas Android passe 196 contrôles
de formes, de lisibilité et de marges ; sa planche est inspectée. Le parcours
portrait complet passe sur cette même base, avec captures inspectées pour
import/export, états classiques, actions d'états, compte, Système et éjection
du cœur. Aucun crash n'est enregistré. La revue indépendante ne relève aucun
problème bloquant.

SHA-256 de cette base debug :
`9215deb7964fd043ea3038a182c9c446b6e36cd397973dfd06d1c5234c485b85`.
Essais sur émulateur API29 x86_64 avec programmes originaux et transport
d'archives contrôlé ; sans nouvelle qualification physique, ARM64 ou audio.

## Livraison 0.23.0

PR #30 fusionnée sur autorisation du mainteneur. Source de l'APK :
`3b5cc44189f0cb346b17a604769d5ddd80cddbbb`, tag `v0.23.0`, versionCode31.
La CI [PR](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41845581313),
la CI [main](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41846084262),
le build [signé](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41846654391)
et la [publication](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41848226713) passent.

SHA-256 de l'APK public :
`ce0646e9a585c7ddf8fd517e2839c4c1b1dd8374db8443c86ded132a8ceb0ddd`.
Les octets publics sont identiques à l'artefact signé qualifié. Certificat durable
`cb431e5abf3ae73ddd5c706a9c485ff6e627be2135bb2557b6191f04ac3a5211` inchangé.
Provenance, deux ABI, alignement 16 Ko, absence de cœur et MP3 original vérifiés.

Mise à jour signée 0.22.2/code30 vers 0.23.0/code31 sur émulateur API29 x86_64 :
UID, dossier de données, première installation, thème ASUS et jeu historique
conservés. Les captures de l'accueil, de la bibliothèque, des sauvegardes, des
cœurs, des réglages et d'À propos sont inspectées ; aucun nouveau crash de
l'application observé. Un dialogue System UI au démarrage de l'émulateur a été
résolu avant la qualification, sans changer l'APK.

Les validations de la refonte et des 48 icônes restent applicables ; seuls le
numéro de version, À propos et les liens README ont changé depuis leur contrôle.
Pas de nouvelle qualification physique, ARM64, réseau public ou écoute audio.

## Accueil, classement du menu rapide et identité de l'organisation

Le bouton Réglages du titre est absent du dessin et du ciblage ; son onglet
reste accessible. Les tests natifs vérifient l'ordre de toutes les actions et
leurs cibles après défilement, en portrait/paysage et pour un/deux joueurs.
Les 21 suites strictes et 21 ASan/UBSan passent, ainsi que build/lint Android et
contrôles de base sans cœur, deux ABI et alignement 16 Ko.

L'atlas Android passe196 contrôles, avec planche inspectée dans les trois
palettes. Le fichier du lanceur est identique à l'avatar officiel récupéré :
SHA256 `896f74c6c33739cf4a267eb068dc496b5e006b0aefb905fc91bacad2e82f8881`. Les petits glyphes sont des déclinaisons vectorielles
du même modèle.

Les parcours Android ciblés portrait et paysage passent : ancien bouton sans
action, navigation, création et chargement d'état avec aperçu du framebuffer,
options du cœur, défilement jusqu'à Quitter et fermeture avec cœur conservé.
Les captures sont inspectées et aucun crash n'est enregistré. Le dialogue
System UI du démarrage a été fermé avant reprise des essais.

Base debug qualifiée : `edee3f440048ca6c7e635fbcdb22203c8311673d263395d075542a4aae570338`.
Émulateur API29 x86_64, programmes originaux et transport d'archives contrôlé ;
pas de nouvelle qualification physique, ARM64, réseau public ou audio.

## Livraison corrective 0.23.1

PR #31 fusionnée sur autorisation du mainteneur. Source de l'APK :
`29b091945b4213e3a72ddcd83fecf1fa3656709b`, tag `v0.23.1`, versionCode32.
CI PR[41878516891](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41878516891),
main[41882858763](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41882858763),
build signé[41883818990](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41883818990)
et publication[41884748183](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41884748183) réussis.

APK public SHA256 : `b1a4fb6f7322c6a016694e1802472f1955544d044be5e181971f360cbadb4cb9`.
Les octets publics sont identiques à la base signée qualifiée. Certificat
durable, provenance, deux ABI, alignement16Ko, absence de cœur et MP3 original
vérifiés. Les deux bibliothèques natives contiennent la version À propos0.23.1.

La publication utilise explicitement le build signé qualifié, via source_run_id.

Upgrade signé31->32 sur émulateur API29x86_64 : UID, données, première
installation et thème ASUS conservés, texte du jeu historique identique en
pixels. Les captures sont contrôlées automatiquement : ancien bouton absent
et géométrie du logo conforme à la capture debug déjà inspectée. Pas de nouvelle
inspection manuelle des captures signées. Aucun nouveau crash applicatif observé.
Les essais fonctionnels ciblés portrait/paysage de la correction restent
qualifiés ; seuls version, À propos et README ont changé depuis ces essais.
Pas de nouvelle qualification physique, ARM64, réseau public ou audio.

## Livraison 0.24.0 et cœurs 0.16.0

PR32 et PR33 fusionnées sur autorisation du mainteneur. Source de l'APK :
`826bbf86d7d682772e801690d86eca1d7a87bdc8`, tag `v0.24.0`, versionCode33.
CI de préparation[41932276773](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41932276773),
main[41932715340](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41932715340),
build signé[41933214823](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/41933214823)
et publication[42017261496](https://github.com/CorvusSoftware/CorvusRetro/actions/runs/42017261496) réussis.
APK public SHA256 : `04b2b93d70456c0921e1f1e8732d2b2c9ddf5eda20058d50d9c3e2084a2868dd`.
Les octets publics sont identiques à la base signée qualifiée : certificat durable,
provenance, version0.24.0, deux ABI, alignement16Ko, zéro cœur et MP3 original vérifiés.

Mise à jour signée0.23.1/code32 vers0.24.0/code33 sur API29x86_64 : même UID,
répertoire de données et première installation ; historique et thème ASUS conservés.
Captures signées inspectées, À propos indique0.24.0 et Core0.16.0 ; aucun nouveau
crash applicatif. Un défaut visuel intermittent de la barre de navigation reste
observé sur cet émulateur à rendu logiciel : il existe aussi en version signée0.23.1,
et le pied de page À propos est identique pixel pour pixel. Aucune correction
ni qualification physique de ce défaut n'est revendiquée ici.

Core0.16.0 : source`cebbc10de76e319ce5deb613a9df750804608174`, API
`588326f289f60f2961351e32730e3ecddd8ebe72`, CI source41931403008 réussie.
[Lot CorvusRepositoryv0.16.0](https://github.com/CorvusSoftware/CorvusRepository/releases/tag/v0.16.0)
construit et publié par41932376500 : six ZIP GB/GBC/GBA pour ARM64 et x86_64,
provenance, SHA, tailles, membre unique, ABI, ELF16Ko et exports triche vérifiés
sur les vrais fichiers publics. Trois modules x86_64 installés/découverts ;
parcours GBA triche/IME/JNI/SAF réussi en portrait4099 et paysage4512 vérifications.
Pas de nouvelle exécution physique ARM64 ; cette architecture est compilée et contrôlée structurellement.

PR catalogue8 fusionnée après qualification, sourceRepository
`676e6aaf980fc5324546493e1bf77c2f40d3d165`, CI41933537606 réussie.
Le catalogue public v1 contient les six nouvelles références exactes ; le flux
historique0.18.0 reste inchangé. Les sauvegardes batterie.sav/.srm gardent leur format ;
les anciens états0.15.0 restent conservés et exigent cette version du cœur.

L'événement push du tag a publié l'APK qualifié. Le dispatch explicite42017261434,
concurrent, a ensuite refusé la création car cette release existait déjà. Aucun
octet n'a été remplacé. La publication effective42017261496 est verte et le
fichier public correspond exactement à la base testée ; le workflow reste actif.
Preuves locales : `/workspace/setup/release0240`,
`/workspace/setup/cheats-public-0.16.0` et `/workspace/setup/core-distribution-0.16.0`.
