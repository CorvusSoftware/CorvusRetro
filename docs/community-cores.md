# Gestion des cœurs CorvusAPI

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

CorvusAPI est le contrat natif commun. CorvusCore fournit les moteurs officiels ;
le frontend conserve ses propres menus, stockage, graphismes et entrées.
Le registre négocie l'ABI0, vérifie les fonctions obligatoires et les groupes
optionnels, puis lit les métadonnées fournies par chaque module.

Sur Android, les modules téléchargés proviennent du catalogue officiel. Ils sont
vérifiés, extraits dans un emplacement privé en lecture seule puis présentés au
registre. Les cœurs ne sont pas inclus dans la base. Aucun fichier arbitraire du
stockage externe n'est chargé comme bibliothèque native.

## Développement de modules

Les tests hôtes peuvent ajouter des sources CorvusAPI via les options CMake
`CORVUS_EXTRA_CORE_DIRECTORIES` et `CORVUS_EXTRA_CORE_TARGETS`. Chaque cible doit
être une bibliothèque partagée exportant `corvus_get_api`. Le catalogue généré
permet de découvrir ses métadonnées sans modifier les tableaux des menus.

Un fournisseur peut contribuer un catalogue `catalog/fournisseur.json` dans
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository), avec ses
propres identifiants permanents `fournisseur.identifiant`. Les archives sont
centralisées dans les releases de ce dépôt. L'ajout du fournisseur, de ses
sources, de sa licence et de ses modules est revu avant publication ; le client
ne charge pas des URL exécutables arbitraires soumises depuis l'application.

Chaque entrée précise le cœur, sa version, l'ABI Android, CorvusAPI0, la
bibliothèque attendue, la taille et le SHA-256. Le workflow du distributeur
vérifie l'archive à une seule bibliothèque, son format ELF, ses exports et ses
segments de 16 Ko avant de préparer la promotion du catalogue. Une empreinte
valide ne remplace pas la revue d'un nouveau fournisseur.

CorvusRetro découvre automatiquement chaque catalogue approuvé. Aucune liste
de fournisseurs, modification du menu ou nouvelle version APK n'est nécessaire.
Voir le [guide de contribution](https://github.com/CorvusSoftware/CorvusRepository/blob/main/CONTRIBUTING.md).
L'extension CMake des tests hôtes ne contourne pas les contrôles de distribution.

## Identités et sauvegardes

Le registre est limité à32cœurs. Les indices peuvent différer selon les modules
présents ; ils ne servent jamais d'identité de stockage. Les identifiants publics
`corvus.gb`, `corvus.gbc` et `corvus.gba` conservent les suffixes historiques0/1/2.
Les autres cœurs utilisent un identifiant permanent et un suffixe de stockage
indépendant de leur position. Voir [les identités de contenu](stable-content-identities.md).

Les groupes de sérialisation et de batterie sont optionnels et cohérents : tout
le groupe est présent ou absent. L'absence d'une capacité ne rend pas les autres
fonctions du cœur invalides. CorvusAPI0 demeure une ABI expérimentale.
