# Interface mobile CorvusRetro

La refonte suit la maquette du mainteneur pour les menus et le menu rapide.
L'écran de jeu, la disposition tactile par console et les pilotes audio restent
ceux de la version précédente. Les menus sont dessinés en C11 avec le peintre
OpenGL existant ; Java fournit les services Android, le clavier et les images.

## Navigation

Les cinq onglets sont **Accueil**, **Jeux**, **Saves**, **Cœurs** et **Réglages**.
L'accueil donne accès à la dernière partie réelle, à la bibliothèque, aux états
et au cœur actif. Charger/décharger un cœur et ses sous-systèmes restent
accessibles. Une partie ne peut démarrer qu'après chargement du cœur associé.

L'accueil adapte les proportions du bandeau, de Continuer, des trois raccourcis
et du cœur à la hauteur disponible entre en-tête et navigation. Dans le viewport
portrait de référence 384 x 696 dp, leurs hauteurs sont respectivement 218, 106,
92 et 92 dp, avec 16 dp sous la carte du cœur. Le facteur reste borné pour
préserver la lisibilité sur petits écrans et fenêtres hautes ; le paysage garde
une présentation compacte. Le sous-système, lorsqu'un cœur est chargé, reste
une entrée supplémentaire accessible par défilement. L'illustration conserve
son ratio et les zones tactiles utilisent les mêmes rectangles que le dessin.

La bibliothèque présente les jeux importés en grille ou en liste, avec recherche
et filtres Tous/GB/GBC/GBA fondés sur l'identité durable du système. Les jeux
restent visibles lorsque leur cœur est désinstallé. Le menu de bibliothèque
regroupe import d'un jeu, import d'un dossier, historique et choix grille/liste.
Les vignettes RetroAchievements identifiées sont conservées par jeu ; les autres
cartes utilisent une illustration originale de la console. Aucun jeu ni contenu
de démonstration n'est ajouté à l'APK.

L'onglet Cœurs et les fiches affichent les informations réelles du catalogue et
des modules installés. Installation et réinstallation utilisent le même
installateur vérifié. La désinstallation garde ses confirmations, protège la
partie active et conserve jeux et sauvegardes.

## Sauvegardes

Saves donne accès aux états de chaque jeu et à la partie courante. Un module
compatible peut créer jusqu'à 32 états nommés avec date et aperçu. Les actions
permettent chargement, renommage, duplication, partage Android d'une copie en
lecture seule et suppression après confirmation.

Les états sont liés au jeu, à la session et à la version du cœur qui les a créés.
Un état absent, corrompu ou d'une version incompatible est refusé avant de
remplacer la partie courante. Un échec final d'écriture est signalé et la liste
est relue pour refléter l'état réel du stockage.

Les anciens états manuels/automatiques et les imports/exports batterie `.sav` et
`.srm` gardent leurs formats et chemins. Ils restent accessibles depuis le jeu
actif. Les cœurs peuvent refuser les états pour une session particulière,
notamment le câble link des modules officiels actuels ; l'interface expose
cette limite plutôt que de proposer une action sans effet.

## Réglages et menu rapide

Affichage, Commandes, Audio, Système, Sauvegardes, RetroAchievements et À propos
regroupent les outils existants. Affichage conserve les choix réellement
implémentés : présentation native ou entière, filtrage et écran allumé.
Les thèmes CorvusRetro, NVIDIA Shield et ASUS gardent leurs identifiants et
préférences ; les couleurs des commandes de jeu sont conservées.

Un [jeu de 42 icônes vectorielles](icons.md) harmonise la navigation, les réglages
et les actions. Renommer, dupliquer, partager et supprimer disposent de symboles
dédiés, avec le même tracé décliné dans les trois palettes.

Le menu rapide est un panneau sur la dernière image du jeu. Ses premières
actions sont Reprendre, Charger un état, Sauvegarder un état, Redémarrer,
Options du jeu et Quitter le jeu. Les autres outils restent accessibles en
faisant défiler : états classiques, batterie, commandes, succès, informations
et accueil. Quitter le contenu conserve le cœur chargé.

## Frontières techniques

Le pilote coordonne navigation, entrées et cycle de vie. Les listes et leurs
métadonnées sont dans `menu_displaylist.c`, les réglages dans `menu_setting.c`,
et les actions dans `cbs/menu_cbs_ok.c`. Les données de ligne et le lecteur
appartiennent à `menu_entries.h` ; le dessinateur consomme ce contrat sans
définir le modèle du menu. Voir [l'organisation des composants](repository-structure.md).

La géométrie des cartes, de la grille et des panneaux est commune au dessin,
au toucher, au focus et au défilement. Les champs du clavier restent rendus en
C ; chaque édition Android porte un ticket qui invalide les anciens résultats.
Les images sont préparées sur un worker avec caches bornés et textures limitées
à 512 pixels. Le partage utilise une URI privée, une permission temporaire et des
fichiers en lecture seule ; les instantanés sont bornés à 16 Mio.

Les états nommés sont stockés sous `states/library` dans des entrées atomiques,
avec métadonnées et aperçu PPM. Les appels au cœur restent sur le thread
propriétaire ; aucune lecture d'état ni écriture de fichier n'est ajoutée au
chemin de dessin. Le magasin n'altère pas les chemins historiques de batterie
ou des anciens états.

## Vérification

`native_interface_test` exerce les parcours et les refus de chargement avec
conservation de la partie courante. `native_state_store_test` couvre les fichiers,
échecs d'écriture et compatibilité. Les suites natives historiques sont adaptées
à la géométrie et conservent leurs contrôles de jeu, son, mémoire et sauvegardes.

`FrontendServicesInstrumentation` teste les services Android réels d'images et de
partage. `InterfaceRedesignInstrumentation` est le parcours de référence pour
les nouveaux menus, les tickets IME et les captures portrait/paysage. Il utilise
des programmes originaux générés uniquement dans l'APK de tests, ainsi qu'un
transport contrôlé pour les archives dont la validation reste active. Les
captures de qualification ne constituent pas du contenu fourni dans l'application.
