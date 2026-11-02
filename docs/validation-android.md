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
que les deux bibliothèques frontend, les segments ELF et l'alignement16Ko.

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
sommes vérifiées, alignement16Ko, zéro cœur, fournisseur synthétique absent.

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
Les deux frontends arm64-v8a/x86_64, l'alignement16Ko, l'absence de cœur et
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
