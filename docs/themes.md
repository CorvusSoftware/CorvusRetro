# Thèmes de CorvusRetro

Les palettes et tous les rôles du rendu sont définis en C dans
`menu/theme.c`. Java stocke le choix et applique aux barres système
la couleur ARGB transmise par le frontend natif.

## CorvusRetro par défaut

Palette inspirée visuellement du logo de corbeau fourni par l'utilisateur.
Les tons ont été adaptés au contraste des petites icônes et du texte :

| Rôle | Couleur |
| --- | --- |
| Fond bleu presque noir | `#090B24` |
| En-tête bleu nuit | `#090D21` |
| Navigation bleu nuit | `#090B20` |
| Accent violet | `#C465F5` |
| Icônes lavande | `#D4A4FF` |
| Texte | `#FAF8FF` |
| Texte secondaire | `#B7B6D3` |

Les cartes ajoutent des dégradés dans cette palette. Les couleurs de l'icône
Android, des barres système et des commandes de jeu conservent leurs valeurs
antérieures. Le logo fourni sert de référence chromatique ; il n'est pas
remplacé par une image retouchée.

## ASUS

Variante claire noire et blanche, inspirée du logotype corporate ASUS.
La référence publique [Simple Icons 14.0.0, entrée ASUS](https://github.com/simple-icons/simple-icons/blob/14.0.0/_data/simple-icons.json)
indique `hex: 000000` et renvoie au [site ASUS](https://www.asus.com).
Le logotype monochrome sert de référence ; cette palette ne prétend pas reproduire
une charte de marque complète. L'accès direct au site ASUS est refusé par le
proxy de cet environnement, donc son guide actuel n'a pas pu être consulté ici.

| Rôle | Couleur |
| --- | --- |
| Fond et panneaux | `#FFFFFF` |
| En-tête, navigation et accent | `#000000` |
| Texte | `#111111` |
| Texte secondaire | `#515151` |
| Sélection | `#EDEDED` |

Les en-têtes et onglets noirs ont un texte blanc ; les listes et le menu rapide
utilisent un texte foncé sur blanc. Les commandes du lecteur restent lisibles
sur le fond noir du jeu.

## Compatibilité des choix

Ordre du menu : CorvusRetro, NVIDIA Shield, ASUS. Le thème sombre a été supprimé.
Les identifiants publiés restent stables : Shield `0`, Corvus `2`, ASUS `3`.
L’ancien identifiant `1` revient à CorvusRetro et la clé Android `dark` est migrée
vers `corvus`. Les choix `shield` et `asus` restent conservés.

CorvusRetro est le défaut en absence de choix ou avec une valeur inconnue.
Les palettes couvrent menus, sélection, icônes, boutons, lecteur, menu rapide et
barres système ; elles ne modifient pas les couleurs des frames émulées.
