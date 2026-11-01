# Paramètres et sous-paramètres

Le rangement de ce document est implémenté dans les menus natifs de CorvusRetro.
Les réglages globaux sont conservés après redémarrage, même sans cœur chargé.
Les actions propres à une partie restent dans le Menu rapide.

## Arborescence

| Catégorie | Sous-paramètres |
| --- | --- |
| Interface utilisateur | Thèmes ; Notifications |
| Vidéo | Garder l'écran allumé ; Présentation de l'image ; Filtrage |
| Audio | Activer le son ; Volume du jeu |
| Commandes | Commandes tactiles ; Manettes |
| Sauvegardes | Copies de sécurité ; État automatique |
| RetroAchievements | Activer RetroAchievements ; Compte |

Une sélection de valeur affiche la valeur courante et la valeur par défaut.
Retour restaure la page, la sélection et le défilement précédents. Depuis le
Menu rapide, le retour conserve la partie en pause jusqu'à sa reprise.

## Interface utilisateur

**Thèmes** : CorvusRetro, NVIDIA Shield et ASUS. Les anciens choix sont conservés.

**Notifications** : deux interrupteurs indépendants :

- Cartes de jeu et de succès : cartes de lancement, de déblocage et de complétion des succès.
- Progression des succès : indicateur avec badge et pourcentage mesuré.

Ces préférences concernent les affichages du frontend pendant le jeu. Elles
ne désactivent ni l'évaluation des succès ni leurs envois. Les messages d'erreur
restent visibles. Désactiver un affichage retire aussi ses notifications en cours.

## Vidéo

- **Garder l'écran allumé** : activé par défaut pendant le jeu.
- **Présentation de l'image** : ratio natif adapté, par défaut ; ou échelle entière.
  Le calcul entier utilise les pixels physiques de l'écran et conserve les
  marges Android et l'espace des commandes. Si aucun facteur entier ne tient,
  le frontend revient au ratio adapté.
- **Filtrage** : pixels nets, par défaut ; ou interpolation. Le filtrage change
  la texture du jeu, sans lisser les textes ou les icônes de l'interface.

Les palettes, couleurs de compatibilité et autres options déclarées par un
cœur restent dans **Menu rapide > Options du cœur**.

## Audio

- **Activer le son** : activé par défaut.
- **Volume du jeu** : 0 à 100 %, par pas de 10, défaut 100 %. Le gain s'applique
  à la sortie PCM du frontend, sans changer le volume système Android.

La source audio d'un câble link reste une option du cœur. Le volume du frontend
s'applique à la sortie sélectionnée par ce cœur.

## Commandes

### Commandes tactiles

- **Afficher les commandes** : même préférence depuis Réglages et Menu rapide.
  Les touches sont relâchées lorsque les commandes sont masquées.
- **Intensité au repos** : 10 à 100 % du style de base, par pas de 10. Le défaut
  100 % conserve les contours discrets existants. Le retour visuel à l'appui et
  les boutons menu/masquage restent lisibles.
- **Taille et position > Taille des commandes** : 100 à 130 % par pas de 10.
- **Taille et position > Position verticale** : de -15 à +15 % par pas de 5 ;
  0 correspond à la disposition automatique.
- **Taille et position > Rétablir la disposition** : rétablit taille et position,
  sans effacer l'intensité ni la visibilité choisies.

La géométrie est bornée par l'espace disponible : portrait, paysage et petite
fenêtre adaptent la disposition. Les zones touchées suivent les dessins.
GB/GBC conservent leurs boutons ; GBA conserve en plus L/R.

### Manettes

La page montre les appareils réellement affectés aux ports des joueurs 1 et 2.
Un port vide indique qu'aucune manette n'est connectée.

**Affectation des touches** propose un profil numérique commun aux manettes :
Haut, Bas, Gauche, Droite, A, B, Start, Select, L et R. Sélectionner une entrée,
puis appuyer sur le bouton physique souhaité. Retour annule la capture. Une
manette doit être connectée pour capturer une touche ; les touches système ou
déjà affectées sont refusées. **Rétablir les touches** restaure le profil de base.

La navigation des menus reste accessible indépendamment du profil. Les axes
analogiques et les chapeaux directionnels gardent leur correspondance
automatique. Le profil ne prétend pas offrir une calibration des axes ou des
profils différents par appareil. Déconnexion, pause et rotation annulent une
capture et relâchent les touches maintenues.

Le joueur des commandes tactiles et du clavier se choisit dans le Menu rapide
d'une session à deux joueurs. Les deux premières manettes conservent leurs
ports distincts.

## Sauvegardes

**Copies de sécurité** : conserver 1, 2 ou 3 versions de la batterie précédant
un import, par joueur. Le défaut reste une copie. Cette préférence s'applique
au prochain import confirmé et réussi ; elle ne supprime rien lors de sa
sélection. La copie la plus récente reste le fichier de secours historique,
les versions précédentes sont conservées dans un historique borné. Une
transaction groupée protège les batteries et les copies pendant leur remplacement.

Pour récupérer une version, ouvrir **Menu rapide > Sauvegardes du jeu > Copies
de secours**. Les copies disponibles peuvent être exportées en `.sav`, puis
importées avec la confirmation habituelle. En câble link, choisir d'abord le
joueur dans Sauvegardes du jeu. Les copies absentes ou illisibles sont désactivées.

**État automatique** : désactivé par défaut. Lors d'une fermeture ou d'un
remplacement du contenu, créer un état distinct si la session permet une
sérialisation complète. Les modules officiels en câble link ne fournissent pas
cette capacité ; ils conservent leurs batteries sans créer de faux état groupé.
L'état automatique n'écrase jamais l'état manuel. Il se charge explicitement
avec **Menu rapide > Sauvegardes d'état > Charger l'état automatique** ; aucune
reprise automatique n'est imposée. Un échec d'écriture lors d'une fermeture
explicite conserve le contenu et affiche l'erreur.

L'enregistrement normal de la batterie à la pause et à la fermeture reste
obligatoire, indépendamment de l'état automatique. Les identités historiques
et les batteries séparées P1/P2 sont préservées. Voir [Sauvegardes du jeu](save-files.md).

## RetroAchievements

L'activation et **Compte** restent accessibles sans cœur ni contenu. Compte
regroupe identifiant, mot de passe éphémère, connexion/déconnexion et état du
compte. Les mots de passe ne sont pas enregistrés ; les jetons et les succès en
attente conservent leur stockage chiffré.

La liste des succès et les informations du jeu sont dans **Menu rapide > Succès**.
Les filtres de notification se trouvent dans Interface utilisateur. Le mode
normal et les limites des sessions restent indiqués ; aucun réglage Hardcore
n'est ajouté sans son implémentation.

## Persistance et compatibilité

Le stockage SharedPreferences `corvus` et les clés historiques restent inchangés :
`theme` est une chaîne, `audio` et `keep_awake` des booléens, `show_input` et
`achievements_enabled` des entiers. Le thème conserve les identifiants
0/Shield, 2/CorvusRetro, 3/ASUS et le repli de l'ancien thème dark.

Les nouvelles clés sont des entiers : `audio_volume`, `video_integer`,
`video_smooth`, `overlay_opacity`, `overlay_size`, `overlay_position`,
`notify_cards`, `notify_progress`, `backup_count`, `auto_state` et
`controller_bind_0` à `controller_bind_9`. Les valeurs hors plage reviennent aux
défauts ; un profil de touches invalide revient au profil de base.

Les trois interrupteurs historiques son, veille et visibilité tactile gardent
la restauration par appui long. Les réglages globaux ne déplacent ni les jeux,
ni leurs sauvegardes, ni les options persistées par cœur.
