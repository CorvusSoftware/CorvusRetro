# Cœurs individuels CorvusRetro 0.11

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

## Utilisation

L'APK de base ne contient aucun émulateur. Ouvrir **Charger un cœur >
Téléchargeur de cœurs**, puis choisir **CorvusGB**, **CorvusGBC** ou **CorvusGBA**.
Le téléchargement correspond au cœur choisi, pas à une nouvelle application
contenant les trois moteurs. Android demande la confirmation d'installation.
Au démarrage suivant, **Charger un cœur** propose les modules installés.

Ajouter CorvusGB conserve un CorvusGBA déjà installé. Les jeux, leur historique
et leurs sauvegardes restent dans le stockage privé de CorvusRetro. Le fichier
d'une sauvegarde GBA conserve son identité même si GBA est le premier ou le seul
cœur dans la liste. **Décharger le cœur** libère l'instance en mémoire ; il ne
désinstalle pas le module Android.

Le catalogue ne propose que les modules correspondant à l'architecture du
processus et au `versionCode` de la base. Une nouvelle version de l'application
peut nécessiter de retélécharger les modules correspondants. Les anciens jeux
ne sont pas supprimés si leur cœur n'est plus disponible.

## Distribution Android

Chaque moteur est une fonctionnalité Android native séparée : `coregb`,
`coregbc` ou `coregba`. Les quatre APK partagent le package
`com.corvussoftware.corvusretro`, le même `versionCode` et le même certificat.
Le service utilise `PackageInstaller.MODE_INHERIT_EXISTING`, ce qui ajoute le
module choisi aux modules présents. Android valide le package, le nom du split,
la version et le certificat avant de rendre le code disponible.

L'APK téléchargé est vérifié par origine HTTPS, taille et SHA-256. Les
bibliothèques sont extraites par Android dans `nativeLibraryDir`, non modifiable
par l'application. Le frontend charge uniquement les bibliothèques de ses
propres modules installés et valide CorvusAPI 0. Aucun `.so` du cache ou des
téléchargements n'est chargé directement.

Avant une installation, le frontend enregistre la batterie puis suspend
l'émulation jusqu'à un résultat Android terminal. La suspension doit survivre
à la recréation de l'activité pendant la confirmation système. Une erreur de
sauvegarde empêche le lancement de l'installation.

## Catalogue

`core-repository/cores-v2.json` utilise le format2. Chaque entrée porte :

| Champ | Rôle |
| --- | --- |
| `id` | Identité CorvusAPI stable : corvus.gb, corvus.gbc ou corvus.gba |
| `name`, `description` | Nom et description du cœur |
| `version` | Version du moteur, actuellement 0.14.0 |
| `abi` | arm64-v8a ou x86_64 |
| `sha256`, `size`, `apk_url` | Fichier APK signé réellement publié |
| `split_name` | coregb, coregbc ou coregba |
| `base_version_code` | Version exacte de la base compatible |

L'unicité porte sur identité du cœur, ABI et version de base. Les entrées des
anciennes bases peuvent être conservées lors de la publication d'une nouvelle
base. Le format1 `index.json` reste réservé aux clients 0.10 et à leurs packs
complets ; il n'est pas transformé en faux catalogue de cœurs individuels.

La génération utilise `tools/register_core_split.py` avec la base signée,
l'APK du cœur et l'empreinte du certificat déjà approuvé. Le certificat du
candidat téléchargé ne devient jamais son propre ancrage de confiance.
`tools/validate_core_splits.py` et `tools/test_core_splits.py` contrôlent le
schéma et la conservation des entrées. Aucun catalogue réel ne contient
d'URL, de taille ou de SHA-256 fictifs.

## Construire et publier

```sh
./gradlew :app:assembleDebug :app:lintDebug \
  :features:coregb:assembleDebug :features:coregbc:assembleDebug \
  :features:coregba:assembleDebug :app:assembleDebugAndroidTest
python3 tools/check_core_apks.py --apksigner "$ANDROID_HOME/build-tools/35.0.0/apksigner" \
  --aapt "$ANDROID_HOME/build-tools/35.0.0/aapt" \
  --zipalign "$ANDROID_HOME/build-tools/35.0.0/zipalign"
```

Le workflow Android produit une base et trois APK de cœur debug. Le workflow
de preview signé reprend un commit main validé, réutilise les secrets de
signature durables et produit les quatre APK signés sans publier de Release.
Le publisher de cœurs reprend ces APK du même commit, contrôle leur certificat
et leurs versions, puis publie la base et les trois modules sous le tag choisi.
Il ne reconstruit pas les APK et ne remplace aucun tag ou asset existant.

Le catalogue généré est proposé dans une branche candidate pour revue. Seules
les métadonnées vérifiées des assets réellement accessibles sont ensuite
intégrées à `main`, afin de rendre les trois téléchargements visibles.

## Essais sur appareil

Les tests hôtes couvrent le registre vide, GBA seul, l'ajout des autres cœurs,
les identités historiques et l'échec batterie avant installation. Les tests
Android couvrent le format du catalogue, le filtrage ABI/version et les
confirmations système. Distinguer les essais sur émulateur d'une qualification
sur téléphone physique ; voir [les preuves exécutées](validation-android.md).
