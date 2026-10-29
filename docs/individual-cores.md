# Cœurs natifs CorvusRetro

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

## Utilisation

Ouvrir **Charger un cœur > Téléchargeur de cœurs**, choisir CorvusGB, CorvusGBC ou
CorvusGBA et attendre la fin du téléchargement. Revenir à **Charger un cœur** : le
cœur est immédiatement disponible. Android ne demande pas d'installer un APK.
**Décharger le cœur** libère son instance en mémoire ; le fichier téléchargé reste
installé pour la prochaine utilisation. Ajouter un cœur conserve les autres.

Sélectionner explicitement un cœur avant **Charger du contenu** ou avant de
lancer un jeu de **Listes de lecture > Historique**. Le cœur déjà chargé doit
correspondre au jeu ; l'application ne change pas de cœur automatiquement.
Les listes de lecture contiennent uniquement **Historique**.

**Menu rapide > Fermer le contenu** sauvegarde le jeu et le ferme en conservant
le cœur chargé. Le menu principal permet ensuite d'ouvrir un autre jeu avec ce
cœur, ou de choisir **Décharger le cœur**. Une erreur de sauvegarde garde le jeu
et le cœur actifs pour permettre une nouvelle tentative.

Les jeux, l'historique, les réglages et sauvegardes sont conservés. Une base0.11
mise à jour peut perdre ses anciens splits Android : retélécharger les moteurs
souhaités suffit, les contenus restent présents. Les futurs modules natifs ne
sont plus liés au versionCode de l'application.

## Référence RetroArch réellement examinée

Révision [dda31fef](https://github.com/libretro/RetroArch/tree/dda31fef21aeb472be9de7ee53eb386cc8db1417),
comparée à [1.22.2](https://github.com/libretro/RetroArch/tree/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576).

- [menu/cbs/menu_cbs_ok.c](https://github.com/libretro/RetroArch/blob/dda31fef21aeb472be9de7ee53eb386cc8db1417/menu/cbs/menu_cbs_ok.c)
  déclenche les tâches du téléchargeur depuis la sélection d'une entrée.
- [tasks/task_core_updater.c](https://github.com/libretro/RetroArch/blob/dda31fef21aeb472be9de7ee53eb386cc8db1417/tasks/task_core_updater.c)
  télécharge l'archive, attend la décompression, prépare le remplacement et actualise
  les informations. La variante Play Feature Delivery suit un autre chemin.
- [frontend/drivers/platform_unix.c](https://github.com/libretro/RetroArch/blob/dda31fef21aeb472be9de7ee53eb386cc8db1417/frontend/drivers/platform_unix.c)
  définit le dossier Android privé des cœurs.
- [libretro-common/dynamic/dylib.c](https://github.com/libretro/RetroArch/blob/dda31fef21aeb472be9de7ee53eb386cc8db1417/libretro-common/dynamic/dylib.c)
  charge les bibliothèques avec dlopen.

CorvusRetro adapte ce parcours à CorvusAPI0. Aucun code ni ABI libretro n'est copié.
L'ancien choix APK confondait des restrictions execve avec une interdiction de tout
chargement natif privé. Le chargement d'un vrai CorvusGB privé en lecture seule a
été vérifié sur Android15, cible35, sans changer les protections Android.

## Implémentation CorvusRetro

CoreRepository récupère le format3 par HTTPS et filtre l'architecture et l'ABI
CorvusAPI0. Il vérifie l'origine, la taille exacte et le SHA-256. CoreStore impose
une archive à une seule entrée connue, borne l'extraction et contrôle l'en-tête ELF.
Le module temporaire est marqué en lecture seule avant écriture via le descripteur
ouvert, synchronisé puis renommé atomiquement dans le stockage privé files/cores.
Son nom inclut le SHA-256 du module. Son empreinte et ses permissions sont revérifiées
au démarrage avant toute découverte native. Les fichiers du cache ne sont pas chargés.

Java publie les chemins au mailbox natif. Le thread frontend négocie CorvusAPI,
actualise les descripteurs en préservant leurs indices et recharge la bibliothèque
de jeux. Un moteur déjà actif garde son ancien handle et son ancien fichier ; une
mise à jour ne remplace jamais cet inode. Les sauvegardes gardent leurs identités.
Un échec conserve les anciens cœurs. Aucun installateur de paquets n'intervient.

## Publication

Les modules et leurs catalogues sont publiés indépendamment dans
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository).
Son workflow construit les trois moteurs et les deux ABIs depuis une révision
qualifiée de CorvusCore, sans construire CorvusRetro ni utiliser sa signature APK.
Les ZIP sont publiés dans les releases de CorvusRepository ; leurs tailles et
empreintes sont vérifiées sur les fichiers publics avant promotion du catalogue.

CorvusRetro construit, qualifie et publie uniquement sa base signée. Son
sous-module CorvusCore reste une référence pour les en-têtes CorvusAPI et les
tests hôtes avec de vrais moteurs, sans verrouiller la version des cœurs
installés. Les APK 0.18.0 acceptent déjà les archives de CorvusRepository.

Le catalogue actif reste `CorvusRepository/catalog/corvuscore.json`. Les anciens
assets CorvusRetro sont conservés avec leurs URL et empreintes historiques ;
aucune réinstallation de l'application n'est nécessaire pour changer d'hébergeur.
