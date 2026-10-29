# Identités de contenu et choix du cœur

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

## Objectif

CorvusRetro distingue désormais les quatre concepts :
- **Contenu** : empreinte SHA-256 du fichier importé ;
- **Système** : identifiant stable de machine (par exemple
  `nintendo.game_boy_color`) ;
- **Cœur** : identifiant public retourné par `corvus_core_info.id`
  (par exemple `corvus.gbc`) ;
- **Clé de stockage** : nom opaque utilisé pour conserver ROM, sauvegarde
  batterie et état de jeu.

Contrairement à la position du module dans la liste affichée, le champ
`coreId` ne change pas lors d'une réorganisation du catalogue. Le
`systemId` est conservé séparément pour préparer plusieurs cœurs
pour une même machine. Le modèle suit le principe de séparation
frontend / core / content de RetroArch et libretro, mais reste conçu
pour CorvusAPI.

## Mise à niveau sans perte

Les enregistrements Android historiques `id/title/system/played`
restent lisibles. Lors de la lecture, les valeurs 0, 1 et 2 sont
reliées respectivement aux identifiants permanents
`corvus.gb`, `corvus.gbc` et `corvus.gba`.
Leurs identifiants de fichiers `SHA256-0`, `SHA256-1` et
`SHA256-2` sont **inchangés**. Aucun fichier `.rom`, `.sav`
ou état instantané n'est renommé ou effacé par cette migration.

Les nouveaux enregistrements Android sauvegardent explicitement
`coreId` et `systemId`. Le champ `system` reste présent pour la
compatibilité avec les anciennes versions et n'est plus utilisé
comme identité durable du cœur lors de l'affichage.

Lorsqu'un cœur communautaire n'est plus installé, son jeu reste
dans l'index et dans l'historique. Son ouverture affiche
un message demandant le retour du cœur, sans effacer les données.
Les anciens contenus communautaires sans `coreId` fiable sont
considérés comme non résolus, plutôt que réassociés au hasard
à un moteur occupant désormais la même position.

## Identifiants des nouveaux cœurs

Les nouveaux jeux importés avec les trois moteurs officiels
conservent l'ancien suffixe numérique pour ne pas perturber les
sauvegardes des utilisateurs actuels.

Les nouveaux jeux destinés aux cœurs communautaires utilisent
`<sha256-du-contenu>-c<empreinte-FNV64-de-coreId>`.
Le SHA-256 identifie le contenu ; FNV-1a 64 bits crée un suffixe
de classement déterministe, **pas** une preuve cryptographique
d'authenticité du cœur. La validation vérifie en plus que le
suffixe correspond au cœur sélectionné. Le même calcul est
appliqué par la couche Java et par le frontend C.

Les indices ne déterminent plus l'association des jeux
communautaires une fois leur `coreId` enregistré.

## Travaux suivants

L'ABI CorvusAPI version 0 ne fournit actuellement qu'un
`system_name` descriptif. Pour un catalogue multi-émulateurs
universel, il reste à normaliser un `systemId` machine dans
les métadonnées déclarées par le cœur ou son manifeste.
Les trois machines officielles utilisent dès maintenant
des identifiants canonique propres à CorvusSoftware.
Pour les cœurs tiers sans manifeste, un identifiant
provisoire issu du `coreId` évite toute association ambiguë.

La bibliothèque pourra alors regrouper plusieurs cœurs autour
d'un système et offrir un cœur préféré par jeu. Les menus et
le contrat public de CorvusAPI restent indépendants.

Les fournisseurs communautaires sont revus dans
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository/blob/main/CONTRIBUTING.md).
Leurs modules compatibles sont distribués par les catalogues approuvés,
indépendamment de l'APK. Le téléchargeur valide l'origine, l'ABI,
la taille, le SHA-256 et la bibliothèque avant installation privée.
Cette ouverture du catalogue ne change ni les identifiants de contenu
ni les sauvegardes existantes.
