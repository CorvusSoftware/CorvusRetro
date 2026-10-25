# Gestion des cœurs CorvusAPI

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

## Architecture

CorvusRetro utilise CorvusAPI comme **contrat natif commun**. CorvusCore
reste l'implementation des cœurs officiels Game Boy, Game Boy Color et
Game Boy Advance. L'interface, les moteurs et l'application sont des
projets independants : leur implementation n'est pas confondue.

En debut d'execution, CorvusRetro examine uniquement la liste de modules
**integres dans sa propre compilation**. Pour chaque module, il charge la
bibliotheque native, demande l'ABI `corvus_get_api`, verifie les fonctions
obligatoires et lit `get_info`. Seuls les modules compatibles et aux
identifiants uniques apparaissent dans le gestionnaire de cœurs.

Les noms de modules ne sont plus stockes dans `engine.c` et les noms
de cœurs, versions, formats et systemes sont fournis par leurs propres
metadonnees. Les ecrans sont construits depuis ce registre.

## Integration d'un cœur communautaire

Le distributeur de l'application peut ajouter un repertoire source de
module CorvusAPI avec l'option CMake
`CORVUS_EXTRA_CORE_DIRECTORIES`, et nommer ses cibles de bibliotheques
natives dans `CORVUS_EXTRA_CORE_TARGETS`. Les deux options sont des
listes CMake, et chacune des cibles doit etre une bibliotheque
`SHARED_LIBRARY` exportant `corvus_get_api`.

CMake cree alors un catalogue des bibliotheques produites, et
CorvusRetro les detecte a partir de leurs metadonnees **sans ajouter
de ligne dans les tableaux des menus ou de `engine.c`**.

Le frontend ne telecharge pas d'executables natifs et ne charge pas
les fichiers `.so` arbitraires presents sur le telephone.

Le catalogue est limite a 32 cœurs au total. Les trois moteurs
officiels conservent leurs positions historiques 0, 1 et 2 pour la
compatibilite des enregistrements existants.

## Identifiants et sauvegardes

Les identifiants de jeux historiques se terminant par `-0`, `-1`
et `-2` sont conserves. Des indices decimaux `-3` a `-31` sont
acceptes pour les paquets et les cœurs additionnels. Le code
de persistance atomique de CorvusRetro reste inchange.

**Limitation connue** : dans l'ABI et la bibliotheque de jeux actuelles,
l'identifiant de sauvegarde repose encore sur l'index du cœur dans
le paquet. Un distributeur doit maintenir le meme ordre de cœurs
d'une version de son application a la suivante, faute de quoi les
associations des jeux communautaires peuvent changer. Une migration
future vers des identifiants de cœurs permanents permettra de lever
cette limite avant la generalisation de la distribution externe.

## Fonctions optionnelles

Les fonctions de serialisation et de sauvegarde batterie sont des
groupes optionnels tout-ou-rien, conformement a CorvusAPI. Un module
n'est pas rejete parce qu'il ne propose pas de sauvegardes batterie.
Les commandes indisponibles doivent etre gerees sans dereferencement
de pointeurs `NULL`.

## Stabilisation

CorvusAPI est toujours en **ABI version 0**. Avant de publier une
version 1.0 stable, il faudra normaliser les extensions de type
capteurs, manettes, vibration, BIOS et cheats, traiter l'identification
permanente des jeux communautaires et valider la compatibilite ABI
C/C++ sur les plateformes cibles. L'existence d'un registre dynamique
ne garantit pas a elle seule une ABI stable.

Les adaptateurs CorvusCore conservent AGPL-3.0-only, l'application
CorvusRetro reste sous AGPL-3.0-only et le contrat CorvusAPI demeure MIT.
