# Plan des réglages

Ce document propose le rangement des préférences de CorvusRetro sur Android.
Il ne décrit pas une nouvelle interface déjà livrée. L'état de référence est
le code de `menu/menu_driver.c`, les préférences Android et les documents
[navigation](navigation.md), [interface de jeu](game-interface.md),
[menus du cœur](core-menus.md), [thèmes](themes.md) et
[RetroAchievements](retroachievements.md).

## Principes

- **Existant** : comportement ou préférence effectivement présent dans le code.
- **Déplacement proposé** : fonction existante, avec un nouvel emplacement.
- **À développer** : fonction absente, à afficher seulement après réalisation
  complète de son action, de sa persistance et de ses erreurs.
- Les réglages globaux définissent les préférences de l'application. Le Menu
  rapide contient les actions et informations du jeu ou de la session en cours.
- Une catégorie ne doit pas être créée vide pour annoncer des fonctions futures.
  Une page d'information sur une capacité absente du cœur reste possible ; elle
  ne doit pas présenter de faux interrupteur.
- Conserver les trois onglets Menu principal, Listes de lecture et Réglages,
  ainsi que le retour avec sélection, filtre et défilement restaurés.

## Arborescence cible

Ordre proposé : apparence, image, son, commandes, sauvegardes, compte. Les
sous-pages ne sont utiles que lorsqu'elles regroupent plusieurs contrôles.
Une page contenant un seul interrupteur reste une liste courte, sans étage
supplémentaire.

| Ordre | Catégorie | Sous-réglages cibles | État |
| --- | --- | --- | --- |
| 1 | Interface utilisateur | Thèmes ; Notifications | Thèmes existants ; notifications configurables à développer |
| 2 | Vidéo | Garder l'écran allumé ; présentation de l'image | Veille existante ; présentation configurable à développer |
| 3 | Audio | Activer le son ; volume du jeu | Son existant ; volume à développer |
| 4 | Commandes | Commandes tactiles ; Manettes | Visibilité tactile existante à déplacer ; configuration des manettes à développer |
| 5 | Sauvegardes | Copies de sécurité ; sauvegardes d'état automatiques | Catégorie et préférences à développer ; ne pas l'afficher avant une première fonction réelle |
| 6 | RetroAchievements | Activation ; Compte | Existants ; séparer la liste des succès du jeu lors du rangement |

Au premier rangement, cinq catégories sont donc suffisantes : Interface
utilisateur, Vidéo, Audio, Commandes et RetroAchievements. Sauvegardes rejoint
la liste lorsqu'une préférence globale utile est effectivement disponible.

### Interface utilisateur

| Chemin cible | Contenu et comportement | État |
| --- | --- | --- |
| Interface utilisateur > Thèmes | CorvusRetro, NVIDIA Shield, ASUS ; conserver les choix et leur ordre | Existant ; le libellé actuel est « Thème de l'interface » |
| Interface utilisateur > Notifications > Cartes de jeu et de succès | Afficher les cartes déjà produites lors de l'identification et des déblocages | Affichage existant ; interrupteur à développer |
| Interface utilisateur > Notifications > Progression des succès | Afficher l'indicateur de progression mesurée | Affichage existant ; interrupteur à développer |

Les notifications concernées sont dessinées dans le jeu par le frontend.
Ne pas les confondre avec les notifications du système Android. Les réglages
d'affichage doivent laisser fonctionner l'évaluation et l'envoi des succès.
Conserver les erreurs de sauvegarde, de connexion et de chargement visibles,
même si les cartes de succès sont masquées. Ne pas ajouter de réglage de son
de notification tant qu'un tel son n'existe pas.

### Vidéo

| Chemin cible | Contenu et comportement | État |
| --- | --- | --- |
| Vidéo > Garder l'écran allumé | Empêcher la veille pendant le jeu | Existant, activé par défaut |
| Vidéo > Présentation de l'image | Taille adaptée au ratio natif ; option de mise à l'échelle entière si elle tient dans la fenêtre | À développer ; le ratio natif est déjà le comportement automatique |
| Vidéo > Filtrage | Choix explicite entre pixels nets et interpolation, après prise en charge dans le rendu | À développer |

Le portrait, le paysage et les fenêtres étroites doivent continuer de réserver
l'espace nécessaire aux commandes et aux marges Android. La palette d'un jeu,
ses couleurs de compatibilité et les autres traitements déclarés par le cœur
restent dans Options du cœur. Pas de sélecteur de pilote graphique, de shaders
ou de fréquence d'écran sans prise en charge effective.

### Audio

| Chemin cible | Contenu et comportement | État |
| --- | --- | --- |
| Audio > Activer le son | Activer la sortie audio du jeu | Existant, activé par défaut |
| Audio > Volume du jeu | Régler le gain du frontend, sans changer le volume système Android | À développer |

La source audio d'une session câble link reste une option du cœur lorsqu'il
la déclare. Aucun réglage de latence, de rééchantillonnage ou de pilote n'est
proposé avant de disposer d'un véritable choix pris en charge et vérifié.

### Commandes

| Chemin cible | Contenu et comportement | État |
| --- | --- | --- |
| Commandes > Commandes tactiles > Afficher les commandes | Même préférence globale depuis les Réglages et le Menu rapide ; relâcher les touches lors du masquage | Existant, déplacement proposé |
| Commandes > Commandes tactiles > Opacité | Régler la visibilité au repos en conservant le retour visuel à l'appui | À développer |
| Commandes > Commandes tactiles > Taille et position | Ajuster la disposition et rétablir la disposition automatique ; conserver des cibles tactiles utilisables | À développer, après l'opacité |
| Commandes > Manettes > Appareils connectés | Identifier les manettes et afficher leur port courant | Entrées physiques et ports automatiques existants ; page à développer |
| Commandes > Manettes > Affectation des touches | Configurer et rétablir les correspondances d'une manette | À développer |

Ne pas créer la page Manettes avant qu'elle fournisse au moins les informations
réelles des appareils connectés. Le choix du joueur commandé par le tactile et
le clavier appartient au Menu rapide pendant une session à plusieurs joueurs.
Les ports des manettes concernent cette session ; ils ne deviennent pas une
préférence globale par simple déplacement de menu.

### Sauvegardes

Cette catégorie future contient des politiques globales, pas les boutons
Importer, Exporter, Sauvegarder l'état ou Charger l'état d'un jeu.

| Chemin cible | Contenu et comportement | État |
| --- | --- | --- |
| Sauvegardes > Copies de sécurité | Définir la conservation d'une batterie précédente lors d'un remplacement, avec une limite explicite | Politique configurable à développer ; la copie de sécurité fixe avant import est décrite dans [Sauvegardes du jeu](save-files.md) |
| Sauvegardes > Sauvegardes d'état automatiques | Créer un état à la fermeture lorsque le cœur et la session permettent une sérialisation complète | À développer ; ne pas confondre avec l'écriture automatique de la batterie |

L'enregistrement de la batterie lors de la pause, du remplacement et de la
fermeture est un comportement existant à conserver. Il ne faut pas en faire
un interrupteur permettant de perdre les progrès par inadvertance. Les chemins
historiques, les identités de contenu et les batteries distinctes des joueurs
doivent rester compatibles. Une future politique d'états automatiques doit
préserver les états manuels et signaler une erreur sans décharger le jeu.

### RetroAchievements

| Chemin cible | Contenu et comportement | État |
| --- | --- | --- |
| RetroAchievements > Activer RetroAchievements | Activation globale, mode normal | Existant, désactivé par défaut |
| RetroAchievements > Compte | Identifiant, mot de passe éphémère, connexion ou déconnexion, état du compte | Existants, regroupement proposé |
| Menu rapide > Succès | Identification du jeu, compteurs, liste des succès et état de compatibilité de la session | Existants ; actuellement partagés avec la page globale |

Le compte et l'activation restent accessibles sans cœur ni jeu chargé.
Les notifications se règlent dans Interface utilisateur ; leur masquage ne
désactive pas RetroAchievements. Le mode Hardcore reste absent tant que ses
restrictions ne sont pas implémentées et qualifiées. Les capacités mémoire et
les restrictions du câble link doivent rester clairement indiquées.

## Actions du jeu et menus du cœur

Le Menu rapide garde ses actions contextuelles. Ce plan ne transforme pas ces
actions en préférences globales.

| Emplacement | Contenu | État et limite |
| --- | --- | --- |
| Menu rapide > Sauvegardes du jeu | Importer une sauvegarde `.sav` ou `.srm` ; exporter la sauvegarde batterie du jeu | Disponible dans cette évolution ; fichiers batterie bruts, choix du joueur en câble link, confirmation avant remplacement |
| Menu rapide > Sauvegardes d'état | Sauvegarder l'état ; charger l'état | Existant, distinct des fichiers batterie ; dépend des capacités du cœur et de la session |
| Menu rapide > Options du cœur | Options et valeurs déclarées par les métadonnées publiques du cœur | Existant ; conserver les clés par cœur et les indications d'application immédiate ou au redémarrage |
| Menu rapide > Commandes | Accès rapide aux commandes tactiles ; joueur contrôlé en session multiple | Existant ; même préférence de visibilité que les réglages globaux |
| Menu rapide > Succès | Succès du jeu et informations de compatibilité | Existant |
| Menu principal > Sous-système | Choisir un sous-système du cœur, ses contenus et démarrer la session | Existant ; libellé « Sous-système » |

« Sous-système » est le libellé du menu principal. Cette entrée reste présente dès qu'un
cœur est chargé, y compris sans jeu, et disparaît quand il est déchargé. Sa
présence ne dépend pas du nombre de déclarations du module ; une page vide
explique l'absence de sous-système. Les options et sous-systèmes ne doivent
pas être recopiés en listes fixes GB, GBC ou GBA dans les réglages globaux.

L'import/export batterie utilise le sélecteur de documents Android, indique
le jeu concerné et conserve la partie et les données existantes en cas
d'annulation ou d'erreur. Les contraintes de remplacement d'une batterie en
cours d'utilisation et de sélection du joueur d'une session multiple relèvent
de l'implémentation réelle ; le menu ne doit pas annoncer ces variantes avant
leur prise en charge. L'extension `.srm` n'en fait pas un état instantané.

## Correspondance avec les cinq menus actuels

| Menu actuel | Destination proposée | Conservation |
| --- | --- | --- |
| Interface utilisateur > Thème de l'interface | Interface utilisateur > Thèmes | Mêmes palettes et identifiants |
| Vidéo > Garder l'écran allumé | Vidéo > Garder l'écran allumé | Même comportement et défaut |
| Audio > Activer le son | Audio > Activer le son | Même comportement et défaut |
| Affichage à l'écran > Surimpression tactile > Afficher les commandes | Commandes > Commandes tactiles > Afficher les commandes | Retirer l'étage général « Affichage à l'écran » ; conserver l'accès du Menu rapide |
| RetroAchievements | RetroAchievements pour l'activation et le compte ; Menu rapide > Succès pour le jeu | Conserver le compte, les succès et les opérations en attente |

## Persistance et navigation

Déplacer ou renommer une ligne ne justifie aucune migration de données. Garder
le stockage SharedPreferences `corvus` et les clés existantes :

| Clé | Type Android existant | Règle à préserver |
| --- | --- | --- |
| `theme` | Chaîne | `corvus`, `shield`, `asus` ; conserver les identifiants natifs `2`, `0`, `3` et le repli de `dark` vers `corvus` |
| `audio` | Booléen | Défaut activé |
| `keep_awake` | Booléen | Défaut activé |
| `show_input` | Entier | `0` ou `1`, défaut activé ; ne pas le convertir implicitement en booléen |
| `achievements_enabled` | Entier | `0` ou `1`, défaut désactivé |

Ne pas renommer `games`, modifier les identités stables de contenu, déplacer
les sauvegardes ou réinitialiser les options persistées par cœur. Les jetons
et les succès en attente restent dans le stockage chiffré RetroAchievements,
pas dans de nouvelles préférences en clair. Définir les nouvelles clés avec
leur type, leur défaut et leur portée seulement lors de l'implémentation.

Conserver la restauration par appui long des trois interrupteurs existants
(son, veille, visibilité tactile). Un accès depuis le Menu rapide doit revenir
au jeu suspendu et à la ligne précédente ; un accès depuis Réglages doit revenir
à cette catégorie. Vérifier cette différence lors de tout changement d'étages.

## Priorités d'implémentation

1. **Rangement sans nouvelle préférence** : remplacer Affichage à l'écran par
   Commandes, raccourcir le parcours tactile, conserver les clés et les retours ;
   distinguer le compte global et les succès du jeu sans perdre les informations.
2. **Actions batterie** : livrer et vérifier Sauvegardes du jeu séparément des
   états, avec traitement réel des fichiers, annulations et erreurs. Harmoniser
   le libellé Sous-système dans les écrans et la documentation concernés.
3. **Réglages courants** : filtres d'affichage des notifications, volume du jeu,
   présentation et filtrage vidéo, opacité tactile. Introduire chaque contrôle
   uniquement avec son comportement, sa persistance et une valeur par défaut.
4. **Commandes avancées** : page réelle des manettes, affectation des touches,
   puis taille et position tactiles avec restauration de la disposition.
5. **Politiques de sauvegarde** : définir et tester copies bornées et états
   automatiques avant de créer leur catégorie globale.

Ne pas ajouter une catégorie Système : Informations, licences, chargement et
téléchargement des cœurs ont déjà un emplacement dans le Menu principal.
Bibliothèque et fichiers pourra devenir une catégorie seulement lorsqu'une
préférence durable, telle qu'une règle de tri, sera implémentée. L'import de
contenu, la sélection d'un dossier et l'ouverture d'un jeu restent des actions
de bibliothèque, sans détour par les Réglages.

Pour chaque lot de menus, vérifier navigation aller-retour, accès depuis les
deux contextes, portrait et paysage, libellés sur petit écran, persistance après
redémarrage, absence de cœur et capacités absentes. Les nouvelles fonctions
de sauvegarde exigent en plus des vérifications de non-perte des données et
de maintien du jeu chargé après erreur.
