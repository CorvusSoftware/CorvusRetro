# Proposition : sous-systèmes et câble link dans CorvusAPI

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

Statut : proposition à soumettre au dépôt CorvusAPI. Aucun symbole décrit ici
n'est disponible dans l'ABI actuelle. Ce document ne constitue pas une nouvelle
API privée de CorvusRetro et n'active pas le câble link dans l'application.

## Point de reprise

CorvusRetro `54cb1b7` utilise CorvusAPI 0, via CorvusCore `8433980` et
CorvusAPI `188a1a5`. Le contrat public ne contient que `load_game` pour un
contenu et un callback vidéo. Il ne déclare aucun sous-système ni fonction
de liaison. Les adaptateurs officiels déclarent `max_players = 1` et
ignorent les commandes des autres joueurs.

Le SDK interne de CorvusCore possède déjà une liaison, utilisée par son
adaptateur libretro pour « Deux joueurs (câble link) ». Le frontend ne peut
pas appeler ce SDK directement : cela contournerait CorvusAPI et ne
fonctionnerait pas avec les autres fournisseurs de cœurs.

## Contrat à proposer dans CorvusAPI

Conserver le point d'entrée et les structures de l'ABI 0. Négocier une
extension optionnelle versionnée, par exemple `corvus_get_subsystem_api`.
L'absence du symbole ou le refus de sa version signifie : aucun sous-système.
L'extension doit être définie, relue et testée dans CorvusAPI avant son
implémentation dans les adaptateurs ou le frontend.

L'extension devra fournir les opérations suivantes sous forme d'un groupe
complet de pointeurs C, avec `struct_size`, version et limites explicites :

| Opération | Contrat proposé |
| --- | --- |
| Énumérer | Retourner le nombre et les descriptions des sous-systèmes d'un module, avant de charger du contenu. |
| Décrire | Identifiant stable du sous-système, libellé UTF-8, nombre de joueurs, liste ordonnée des emplacements ROM, libellé et extensions de chaque emplacement, caractère obligatoire. Chaînes valides jusqu'au déchargement du module. |
| Charger | Charger un sous-système identifié et sa liste de `corvus_game` dans une instance candidate neuve. Copier toutes les données conservées avant le retour. Un échec ne modifie jamais l'instance active. |
| Batterie par emplacement | Taille, dirty, copie avec génération, acquittement après écriture atomique, séparément pour chaque joueur. Aucun acquittement si l'écriture échoue. Le groupe historique ne doit pas prétendre sauvegarder une session multiple. |

La création, destruction, entrée, exécution d'une frame et interrogation audio/
vidéo restent celles de l'API principale sur la même instance opaque.
`set_button(instance, player, ...)` utilise des joueurs de zéro à N-1.
Une frame avance toutes les consoles liées de manière synchronisée, puis
produit une image composite à géométrie annoncée et un flux audio stéréo.
Le fournisseur compose les écrans et choisit le mixage ; le frontend n'accède
jamais aux machines ni au hub internes. Une future sortie multi-écrans
devra faire l'objet d'une extension distincte si nécessaire.

La disponibilité des états et du reset doit être interrogée pour la session
chargée. Un état annoncé doit contenir toutes les machines ET la liaison.
À défaut, l'action est absente du menu. Le reset doit réinitialiser toute la
session ; un reset partiel d'un seul joueur ne satisfait pas ce contrat.
Les callbacks restent synchrones et appartiennent au thread frontend.

## Adaptateur CorvusCore et parcours CorvusRetro

1. CorvusCore expose le sous-système via l'extension publique acceptée, avec
   deux emplacements obligatoires et l'identifiant stable de son fournisseur.
   Il encapsule son hub, ses deux machines et leur destruction ordonnée.
2. CorvusRetro affiche « Configuration du sous-système » dès qu'un cœur est
   chargé, à la demande de l'utilisateur. Sans cœur, l'entrée disparaît.
   Dans la version0.17.1, cette page indique l'indisponibilité de la fonction.
   Après ajout du contrat public, seuls les sous-systèmes réellement déclarés
   et acceptés pourront être configurés. `max_players` ou le nom du cœur ne
   remplacent jamais cette déclaration.
3. « Deux joueurs (câble link) » ouvre la sélection de Jeu 1, Jeu 2 et Démarrer.
   Une annulation conserve la session active. Une sélection est invalidée si
   le cœur est déchargé ou remplacé, y compris pendant le sélecteur Android.
4. Le démarrage valide les deux contenus et persiste la session précédente
   avant remplacement. Les identités durables de contenu et de cœur sont
   conservées ; aucun indice de catalogue n'entre dans un nouveau chemin.
5. Les batteries restent séparées même si les deux joueurs ouvrent la même
   ROM : la sauvegarde historique du joueur 1 conserve son chemin ; la copie
   du joueur 2 possède un suffixe stable distinct, à spécifier avec les tests
   de migration. Un échec de sauvegarde conserve toute la session liée.
6. Les manettes sont associées à des joueurs distincts. Le tactile peut
   sélectionner son joueur depuis le menu rapide. Une déconnexion relâche
   uniquement les touches de la manette concernée.
7. « Décharger le cœur » persiste les deux batteries, ferme la liaison et
   revient à « Charger un cœur ». L'entrée de sous-système disparaît.

## Validation requise avant activation

- Négociation : ancien module sans extension, version inconnue, groupe incomplet,
  métadonnées invalides et module à zéro sous-système.
- Fournisseur : transfert série GB/GBC vérifié par les données échangées,
  liaison GBA, deux entrées indépendantes, reset de la paire et ordre de destruction.
- Persistance : même ROM deux fois, batteries distinctes, échec disque sans
  perte de session, nouvelle tentative et rechargement.
- Frontend : apparition/disparition conditionnelle, sélection de deux jeux,
  échec du second chargement, annulation et résultat SAF devenu obsolète.
- Android : APK de base toujours sans cœur, pack explicite, portrait/paysage,
  retour arrière, pause/reprise et manettes. Distinguer l'émulateur du matériel réel.
