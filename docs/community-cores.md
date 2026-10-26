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

Le catalogue de distribution Android accepte actuellement les trois identités
officielles. L'ajout d'un autre fournisseur implique une revue de son origine,
de sa licence, de ses métadonnées et de la compatibilité de ses bibliothèques.
L'existence du mécanisme d'extension CMake ne contourne pas ces contrôles.

## Identités et sauvegardes

Le registre est limité à32cœurs. Les indices peuvent différer selon les modules
présents ; ils ne servent jamais d'identité de stockage. Les identifiants publics
`corvus.gb`, `corvus.gbc` et `corvus.gba` conservent les suffixes historiques0/1/2.
Les autres cœurs utilisent un identifiant permanent et un suffixe de stockage
indépendant de leur position. Voir [les identités de contenu](stable-content-identities.md).

Les groupes de sérialisation et de batterie sont optionnels et cohérents : tout
le groupe est présent ou absent. L'absence d'une capacité ne rend pas les autres
fonctions du cœur invalides. CorvusAPI0 demeure une ABI expérimentale.
