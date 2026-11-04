# Codes de triche

Dans une partie, ouvrez le menu rapide puis **Codes de triche**.

1. Ajoutez un code, saisissez ses lignes puis validez avec la coche ou la touche de validation du clavier.
2. Donnez-lui un nom et activez son interrupteur.
3. Revenez à la liste et choisissez **Appliquer les modifications**.

La liste sélectionnée et les codes réellement appliqués sont distincts. Les réglages
permettent d'appliquer automatiquement les changements et de reprendre la liste au
chargement du jeu. Ces deux options sont désactivées par défaut. Une erreur de
validation conserve la liste active précédente. Désactiver un code arrête ses
écritures futures ; les valeurs déjà modifiées en mémoire ne sont pas restaurées.

## Formats et listes

- CorvusGB et CorvusGBC : Game Genie et GameShark.
- CorvusGBA : CodeBreaker brut, GameShark v1/v2 et Action Replay v3.
- Le cœur valide les variantes prises en charge. Les patches ROM GBA,
  changements de graine DEADFACE et CodeBreaker chiffrés ne sont pas acceptés.
- Maximum : 128 entrées et 4096 octets UTF-8 par code. Les décodeurs du cœur
  imposent aussi leur budget d'opérations et d'accès mémoire.

L'import remplace la liste du joueur sélectionné. L'export produit un fichier `.cht`
avec `cheats`, `cheatN_desc`, `cheatN_code`, `cheatN_enable` et `cheatN_handler = 0`.
Les retours à la ligne des codes sont exportés comme espaces. Les listes utilisant
le moteur de recherche/modification mémoire de RetroArch (`handler = 1`) sont
refusées : ce moteur n'est pas implémenté dans CorvusRetro.

Les profils privés utilisent l'identifiant stable du contenu :
`cheats/<id>.cht` et `cheats/<id>.p2.cht` pour le deuxième joueur. Même si les deux
joueurs utilisent le même jeu, leurs listes restent séparées. Les codes actifs
survivent au redémarrage et à la restauration d'un état. Ils ne sont pas inclus
dans les états ; fermer le jeu les retire du cœur sans supprimer la liste privée.
Une liste privée illisible reste protégée jusqu'à un remplacement ou effacement
explicite.

## Cœurs compatibles et architecture

L'extension publique optionnelle `corvus_get_cheats_api` appartient à CorvusAPI.
Un ancien cœur reste utilisable, avec un message d'indisponibilité dans ce menu.
La mise à jour du cœur puis son rechargement sont nécessaires pour obtenir la
fonction. L'APK de base reste sans cœur ; CorvusRepository distribue les mises à
jour indépendamment.

Le code original est réparti entre `cheats/cheat_store.c` (format et stockage),
`menu/menu_cheat.c` (navigation et profils), `cores/engine.c` (extension et listes
actives), et le service Android `CheatFiles` (documents SAF). Seul le thread natif
de l'interface appelle les cœurs. Les transferts utilisent une réponse bornée et
un identifiant unique ; une réponse annulée ou ancienne ne modifie pas une autre
partie. Aucune lecture de fichier n'est ajoutée aux boucles de rendu ou audio.

## Référence examinée

Le comportement a été étudié dans RetroArch, révision
[`665064a6f4867945bd2598e43cb7c731bafb2ae1`](https://github.com/libretro/RetroArch/tree/665064a6f4867945bd2598e43cb7c731bafb2ae1) :
`cheat_manager.c`, `cheat_manager.h`, `menu/menu_displaylist.c`,
`menu/cbs/menu_cbs_ok.c`, `menu/menu_setting.c` et le lecteur de configuration.
Cette étude concerne la gestion des listes, leur application, les options
automatiques et le format `.cht`. Le code et l'icône de CorvusRetro sont originaux.
Le mode normal RetroAchievements conserve son fonctionnement ; le mode Hardcore
n'est pas proposé par CorvusRetro.
