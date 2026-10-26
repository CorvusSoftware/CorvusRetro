# Corvus Core Downloader : APK hors Play Store

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

## CorvusRetro 0.10 : application de base et packs officiels

La nouvelle **APK de base 0.10.0** ne contient plus les modules
`libcorvus_gb.so`, `libcorvus_gbc.so` ou `libcorvus_gba.so`.
La liste des cœurs installés est vide au premier démarrage, mais
le téléchargeur est disponible.

Les trois moteurs restent développés dans CorvusCore. Une APK
optionnelle les intègre uniquement lorsque le build reçoit
`-PcorvusCores=gb,gbc,gba`. Le paquet contenant les trois
moteurs sera proposé dans le catalogue officiel **après** signature
release, création de la GitHub Release et validation de la fiche
`core-repository/index.json`.

Ce modèle est un **pack APK complet**, pas trois téléchargements
de `.so` indépendants et cumulables. Changer de pack remplace
la collection de moteurs embarqués dans l'APK. Tant qu'un format
de plug-in natif séparé n'a pas passé les tests Android, il ne faut
pas le présenter comme l'installation individuelle de cœurs de RetroArch.

Version de l'APK de base : **0.10.0 (versionCode 10)**.
VersionCode minimal du pack : **11** ; chaque publication ultérieure
doit employer une valeur supérieure à celles déjà distribuées.

```sh
# APK de base sans moteurs :
./gradlew :app:assembleRelease

# Pack officiel explicite (3 moteurs) :
./gradlew -PcorvusCores=gb,gbc,gba \
  -PcorvusVersionCode=11 \
  -PcorvusVersionName=0.10.0 :app:assembleRelease
```

## Publication depuis GitHub Actions

La procédure de publication officielle se trouve dans
`.github/workflows/publish-core-pack.yml`. Elle s'exécute manuellement
depuis `main` uniquement. Elle exige des secrets GitHub enregistrés
**par un responsable du dépôt**, et non dans un commit :

- `CORVUS_RELEASE_KEYSTORE_B64` : keystore signé durable, encodé en base64 ;
- `CORVUS_RELEASE_STORE_PASSWORD` : mot de passe du magasin ;
- `CORVUS_RELEASE_KEY_ALIAS` : alias de signature ;
- `CORVUS_RELEASE_KEY_PASSWORD` : mot de passe de la clé ;
- `CORVUS_RELEASE_CERT_SHA256` : empreinte publique du certificat
  de l'application déjà installée et approuvée.

Avec un tag de release neuf de forme `v0.10.0-cores.1`, un
`version_code` croissant et un `version_name` tel que `0.10.0`,
ce workflow construit le pack, vérifie sa signature et la présence
des cœurs pour les deux architectures, puis publie l'APK de pack
dans une GitHub Release. Il fournit ensuite un fichier de catalogue
prêt à être **soumis et relu**, mais ne l'enregistre pas automatiquement
sur `main`.

La mise à jour du catalogue n'est publique qu'une fois l'APK
réellement téléchargeable. Ne jamais ajouter de fausse URL de
release ni de faux SHA-256 pour simuler des cœurs disponibles.

## Etat du telechargeur

Le nouveau menu natif **Charger un cœur > Téléchargeur de cœurs**
permet de recuperer un catalogue HTTPS de CorvusSoftware.

Sur Android moderne (minSdk 26, targetSdk 35), nous choisissons
**des packs Android sous forme d'APK complets** contenant un frontend
CorvusRetro et une collection de cœurs CorvusAPI. Nous ne chargeons
pas automatiquement des modules `.so` depuis le dossier de stockage
modifiable par l'application. Les regles Android de protection de
l'execution native s'appliquent aussi aux APK installes hors Play Store.

Le catalogue officiel est
`core-repository/index.json`, heberge sur le depot CorvusRetro
a travers GitHub HTTPS. Il commence volontairement par une liste vide.

Le frontend natif C11 reste proprietaire de son interface, et Java
se limite aux fonctions de la plateforme Android : HTTP, calcul SHA-256,
autorisation de sources d'APK et PackageInstaller.

## Parcours utilisateur

1. L'utilisateur ouvre le telechargeur depuis "Charger un cœur".
2. Android recupere le catalogue HTTPS et n'affiche que les packs
   compatibles avec l'ABI du processus Android.
3. Une selection demande le telechargement du pack officiel.
   L'application impose une origine HTTPS GitHub de CorvusSoftware,
   un plafond de 300 Mio, une taille annoncee et un SHA-256 exact.
4. La validation du paquet **et de la signature APK** est deleguee
   a Android PackageInstaller. Android demandera la permission
   d'installer des APK et une confirmation de mise a jour.
5. Android refuse un APK qui ne peut pas mettre a jour CorvusRetro
   avec l'identifiant de paquet et le certificat de signature deja
   installes. Les jeux et leurs sauvegardes sont conserves lors
   d'une mise a jour valide.

Installer un pack modifie donc **l'APK de CorvusRetro**. Ce n'est pas un
telechargement autonome et immediat de bibliotheque `.so` comme avec
certains formats de distribution de RetroArch. L'application doit
redemarrer apres son remplacement pour charger les nouveaux modules.

## Comment publier le premier pack

Il faut **une seule cle de signature release durable** pour toute la
vie de ce canal de distribution. Stocke la cle et les secrets hors git,
dans un coffre fiable. Ne publie jamais le keystore ni les mots de passe.

Exemple local apres avoir configure les variables d'environnement :

```sh
export CORVUS_RELEASE_KEYSTORE=/chemin/prive/corvus-release.jks
export CORVUS_RELEASE_STORE_PASSWORD=...
export CORVUS_RELEASE_KEY_ALIAS=...
export CORVUS_RELEASE_KEY_PASSWORD=...
./gradlew :app:assembleRelease
```

Controle le certificat et le manifeste avec le SDK Android :

```sh
apksigner verify --print-certs app/build/outputs/apk/release/app-release.apk
```

Il faut ensuite :
- ajuster le `versionCode` de l'application pour chaque publication ;
- verifier que l'APK inclut tous les cœurs annonces et les ABIs cibles ;
- tester sur un vrai appareil une mise a jour depuis un APK **signe
  avec le meme certificat**, puis verifier sauvegardes et rechargement ;
- publier l'APK signe sur une Release du depot
  `CorvusSoftware/CorvusRetro`, sous un tag immuable ;
- relever le digest SHA-256 du certificat de l'**application deja
  installee et de confiance**, et non celui declare par un fichier
  telecharge depuis Internet ;
- preparer un changement du catalogue avec l'outil de validation.

```sh
python3 tools/register_core_pack.py \
  --apk app/build/outputs/apk/release/app-release.apk \
  --abi arm64-v8a --id corvus-core-pack --name "Corvus Core Pack" \
  --description "Cœurs officiels GB, GBC et GBA" \
  --version 1.0.0 --core-id corvus.gb --core-library libcorvus_gb.so \
  --url https://github.com/CorvusSoftware/CorvusRetro/releases/download/v1.0.0/app-release.apk \
  --expected-cert-sha256 <empreinte-publique-64-caracteres>
python3 tools/validate_core_catalog.py
python3 tools/test_core_catalog.py
```

Executer ensuite la meme commande pour `--abi x86_64` sur l'APK
universel si ses bibliotheques x86_64 sont egalement presentes.
Le schema et l'outil imposent l'origine officielle, les champs,
la signature verifiee, le SHA-256 et les limites de taille.

La fusion du nouveau `index.json` sur `main` rendra le pack visible
dans le telechargeur apres actualisation. Aucun secret n'est necessaire
pour lire le catalogue ; la signature release, elle, est une condition
prerequise pour publier de nouveaux packs.

**Attention pour les versions de developpement :** si le telephone
possede un APK debug signe avec un certificat different, Android ne peut
pas le remplacer automatiquement par le premier APK de release. Sauvegarde
les donnees et les sauvegardes utilisateur avant tout changement de
certificat et prevois une migration accompagnee. L'installation d'un
APK release sur une application deja signee avec le meme certificat
ne doit pas necessiter l'effacement de ses donnees.

## Securite et licences

Le catalogue est servi en HTTPS depuis le depot controle par
CorvusSoftware. Son contenu n'est pas encore signe independamment
de HTTPS : SHA-256 protege l'integrite de l'APK reference, et le
systeme Android impose la compatibilite des signatures des APK
installes. Une signature detachee du catalogue et un vrai
gestionnaire de versions / restauration pourront venir ensuite.

N'installe pas des APK tiers arbitraires. L'ABI CorvusAPI est toujours
experimentale (version 0). Le frontend conserve AGPL-3.0-only, CorvusCore
reste sous AGPL-3.0-only et la specification CorvusAPI reste MIT.

Une validation sur **telephone physique** reste indispensable avant
de publier un pack en ligne. Les tests CI n'executent pas
PackageInstaller sur un appareil reel.
