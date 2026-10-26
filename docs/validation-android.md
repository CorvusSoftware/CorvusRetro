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
python3 tools/validate_core_archives.py
python3 tools/test_core_archives.py
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
python3 tools/build_core_archives.py --variant debug --tag v0.13.0 \
  --output build/native-cores --catalog build/native-cores.json
```

La base `pkg/android/app/build/outputs/apk/debug/app-debug.apk` doit contenir
uniquement `libcorvusretro.so` pour ARM64 et x86_64. Les six bibliothèques du
répertoire `pkg/android/app/build/native-cores/Debug/` doivent exposer CorvusAPI,
correspondre à l'architecture annoncée et avoir des segments ELF de16Ko.
Chaque ZIP contient une seule bibliothèque.

## Qualification de l'application

Tester sur Android la base vide, le téléchargement GBA seul puis GB/GBC cumulés,
la liste immédiatement actualisée, le chargement et le déchargement, la reprise
après recréation pendant un transfert et les téléchargements invalides. Aucun
installateur de paquet ne doit être lancé pour un cœur.

Vérifier également portrait/paysage, marges système, commandes multitouch et
menu central, reprise des jeux existants, historique, thèmes et batterie GBA
historique suffixe-2. Un échec d'écriture de batterie conserve le moteur.

L'instrumentation utilise des programmes de test originaux et un transport
HTTPS déterministe. Un passage de ces tests ne prouve pas une connexion au
service public depuis un téléphone. Distinguer explicitement build, tests hôtes,
émulateur Android, exécution ARM64 et appareil physique.

## Publication

Le build signé doit employer le certificat durable de la base déjà distribuée.
Le publisher exige une CI réussie du commit exact, la provenance des dépendances,
les empreintes de tous les fichiers et le tag correspondant à la version APK.
Il réutilise ces fichiers qualifiés, publie les vraies archives puis prépare
leur catalogue pour revue. Vérifier les octets publics avant promotion du catalogue.
