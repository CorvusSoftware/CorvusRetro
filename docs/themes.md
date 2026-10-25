# Thèmes de CorvusRetro

Les palettes et tous les rôles du rendu sont définis en C dans
`app/src/main/cpp/theme.c`. Java stocke le choix et applique aux barres système
la couleur ARGB transmise par le frontend natif.

## CorvusRetro par défaut

Palette inspirée visuellement du logo de corbeau fourni par l'utilisateur.
Les tons ont été adaptés au contraste des petites icônes et du texte :

| Rôle | Couleur |
| --- | --- |
| Fond bleu presque noir | `#090D14` |
| En-tête bleu nuit | `#111827` |
| Navigation ardoise | `#141E30` |
| Accent lavande/violet | `#B69AFF` |
| Icônes indigo | `#9A9DFF` |
| Texte | `#EFF1FA` |
| Texte secondaire | `#A8B2CD` |

L'icône de lancement reprend aussi ce fond et l'accent violet. Le logo fourni
sert de référence chromatique ; il n'est pas remplacé par une image retouchée.

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

Ordre du menu : CorvusRetro, NVIDIA Shield, MaterialUI sombre, ASUS.
Les identifiants enregistrés restent stables : Shield `0`, sombre `1`, Corvus `2`,
ASUS `3`. Les clés Android `shield` et `dark` gardent leur sens, complétées par
`corvus` et `asus`.

CorvusRetro devient le défaut lorsqu'aucun choix n'est enregistré. Un choix
explicite antérieur est conservé. Une valeur inconnue revient à CorvusRetro.
Les palettes couvrent menus, sélection, icônes, boutons, lecteur, menu rapide et
barres système ; elles ne modifient pas les couleurs des frames émulées.
