# Références MaterialUI et Shield

Révisions étudiées :

- [RetroArch ca3f823](https://github.com/libretro/RetroArch/tree/ca3f823755762005b393ec7ce7b46b0310bdf0db).
- [retroarch-assets d9f9690](https://github.com/libretro/retroarch-assets/tree/d9f969054dc7fbb6fa89519036d2b971e0855b51).
- [SDK CorvusCore 0.14.0](https://github.com/CorvusSoftware/CorvusCore/blob/38829d4935bbdcf4b13261714352362065b78551/sdk/include/corvuscore/bridge.h).

## Architecture native 0.5

Fichiers effectivement examinés dans RetroArch :

- [frontend/drivers/platform_unix.c](https://github.com/libretro/RetroArch/blob/master/frontend/drivers/platform_unix.c) : callbacks NativeActivity, fenêtres, entrées et Looper Android.
- [gfx/drivers_context/android_ctx.c](https://github.com/libretro/RetroArch/blob/master/gfx/drivers_context/android_ctx.c) : contexte EGL et surface Android.
- [gfx/drivers/gl2.c](https://github.com/libretro/RetroArch/blob/master/gfx/drivers/gl2.c) : pilote graphique natif OpenGL.
- [input/drivers/android_input.c](https://github.com/libretro/RetroArch/blob/master/input/drivers/android_input.c) : événements natifs Android et routage des commandes.
- [audio/drivers/aaudio.c](https://github.com/libretro/RetroArch/blob/master/audio/drivers/aaudio.c) : sortie audio native.
- [RetroActivityFuture.java](https://github.com/libretro/RetroArch/blob/master/pkg/android/phoenix/src/com/retroarch/browser/retroactivity/RetroActivityFuture.java) : intégration Android et gestion des surfaces.

CorvusRetro reprend cette approche de frontend C et de pilotes natifs. Ses menus,
son modèle d'entrée, shaders, batch de quads, cache de glyphes, boucle native,
tampon audio et gestionnaire de moteurs sont des implémentations originales.
La colle standard native_app_glue est compilée depuis le NDK avec sa notice
Apache-2.0 conservée. Java fournit des services du système, sans menus ni boucle
d'émulation : SAF, index et préférences Android, polices et ressources en atlas.
Le SDK C de CorvusCore reste l'interface des moteurs ; libretro n'est pas l'ABI
utilisée par cette intégration.

## Chargement et déchargement

Dans [command.c](https://github.com/libretro/RetroArch/blob/ca3f823755762005b393ec7ce7b46b0310bdf0db/command.c), `command_unload_core` libère le moteur, efface le dernier chemin de cœur et rafraîchit le menu. `command_close_content` utilise aussi le mécanisme de libération du menu.

CorvusRetro adapte ce contrat : charger crée un véritable handle du SDK, décharger
le détruit et publie Aucun cœur. La fermeture du contenu persiste la batterie avant
libération. Le gestionnaire reste disponible pour un prochain chargement ; aucun
moteur fictif n'est nécessaire au menu Android. Depuis0.12, les modules sont téléchargés en archives natives individuelles puis
chargés via CorvusAPI. Voir [les références du téléchargeur](individual-cores.md).

## Palette NVIDIA Shield

Valeurs examinées dans `materialui_theme_nvidia_shield`, dans
[menu/drivers/materialui.c](https://github.com/libretro/RetroArch/blob/ca3f823755762005b393ec7ce7b46b0310bdf0db/menu/drivers/materialui.c).

| Élément | Couleur |
| --- | --- |
| Fond des listes | `#121212` |
| En-tête | `#1F1F1F` |
| Barre de navigation | `#242424` |
| Surface | `#1D1D1D` |
| Sélection | `#255D00` |
| Icônes et navigation actives | `#7AB547` |
| Navigation inactive | `#558B2F` |
| Texte | `#DEDEDE` |
| Indications | `#999999` |
| Séparateurs | `#498515` |

CorvusRetro conserve cette palette comme option NVIDIA Shield, avec les thèmes
CorvusRetro et ASUS. Le catalogue et le rendu sont natifs C ; les choix
sont mémorisés. Voir [les palettes actuelles](themes.md).

## Icônes

Le pilote associe `MENU_ENUM_LABEL_CORE_LIST_STR` à `MUI_TEXTURE_CORES`, puis à
[glui/cores.png](https://github.com/libretro/retroarch-assets/blob/d9f969054dc7fbb6fa89519036d2b971e0855b51/glui/cores.png).
Cette icône est un bloc plein avec un symbole découpé. `load_content.png`,
`playlist.png`, `settings.png`, `file.png` et `history.png` ont aussi été examinés.

CorvusRetro dessine ses propres vecteurs : module avec tête de corbeau en réserve,
profil de corbeau pour l'identité, classeur pour le contenu, lignes pour les listes,
roue dentée pour les réglages, historique et éjection pour le déchargement.
Depuis 0.21, la famille compte 32 icônes aux contours arrondis cohérents, dont les
catégories vidéo/audio/commandes, succès, sous-systèmes et sauvegardes. Les formes
restent lisibles à petite taille. Aucun fichier graphique de RetroArch n'est embarqué.


## Audit des dimensions et de l'ordre des menus

Révision consultée :
[`b28b8a4b9fa82d5e05cb625a9d8165333b9c129a`](https://github.com/libretro/RetroArch/tree/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a).
Les fichiers de référence ont été lus, sans reprise de leur code ou de leurs
ressources dans l'application.

### Taille automatique Android

Sources :
[`materialui_layout`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/menu/drivers/materialui.c#L9629),
[`gfx_display_get_dpi_scale_internal`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/gfx/gfx_display.c#L118),
[`gfx_display_get_dpi_scale`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/gfx/gfx_display.c#L241),
[`config.def.h`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/config.def.h#L1909)
et
[`VIDEO_PX`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/gfx/video_defines.h#L318).

MaterialUI utilise une unité `B = 212 * S` en pixels. `S` est une échelle
adaptée à la diagonale de l'image et à la densité rapportée par le contexte graphique. Le facteur
menu par défaut vaut **0,75 sur Android**, contre 1 sur bureau. Ce facteur
ne doit pas être oublié lors d'une comparaison des tailles.

Sur Android,
[`android_display_get_metrics`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/gfx/display_servers/dispserv_android.c#L481)
lit d'abord la propriété système `ro.sf.lcd_density`, puis tente `wm density`.
Une valeur absente ou non positive revient à 90 DPI. Il ne s'agit pas d'une
lecture de `DisplayMetrics.xdpi` ou `ydpi`. Utiliser `densityDpi` depuis l'API
Android correspond au cas habituel, mais une modification utilisateur de la
densité peut différer de cette propriété prioritaire de RetroArch.

Pour un écran de téléphone, la branche de calcul correspondant à une diagonale
estimée inférieure à 12 pouces se décrit ainsi :

```text
D = hypot(largeur_pixels, hauteur_pixels)
p = D / sqrt(1920² + 1080²)
d = DPI_rapporté * 0,83
f = (D / d) / 12
S = ((1 - f) * p + f * d / 96) * 0,75
B = 212 * S
```

Entre 12 et 24 pouces estimés, l'échelle automatique vaut `d / 96`. Entre
24 et 32 pouces, elle interpole de `d / 96` vers `p`, avec une fraction
`(diagonale_estimée - 24) / 8`. Au-delà de 32 pouces, elle vaut `p`. Le facteur
menu Android de 0,75 s'applique ensuite dans tous ces cas. Sans DPI exploitable,
le calcul revient à l'échelle issue des pixels. Les valeurs converties par
`VIDEO_PX` sont arrondies à l'entier le plus
proche. Les retours à la ligne et les métriques de la police interviennent
ensuite dans les dimensions des lignes.

Exemple de référence : **480 x 960 pixels, DPI rapporté 200, densité Android
1,25 et facteur menu par défaut**. La densité logique seule ne garantit pas
que le pilote rapporte effectivement 200 DPI. Sous cette hypothèse,
`S = 0,8672983` et `B = 183,8672 pixels`.

| Élément MaterialUI | Proportion | Pixels arrondis | Équivalent dp à 1,25 |
| --- | --- | --- | --- |
| Police du titre | B / 7 | 26 | 20,8 |
| Police des lignes | B / 9 | 20 | 16 |
| Police des indications | B / 11 | 17 | 13,6 |
| Icône de ligne | B / 3 | 61 | 48,8 |
| Barre du titre | B / 3 | 61 | 48,8 |
| Barre de navigation | B / 3 | 61 | 48,8 |
| Barre système interne | B / 7 | 26 | 20,8 |
| En-tête interne complet | somme des deux barres | 87 | 69,6 |
| Marge latérale | B / 9 | 20 | 16 |
| Écart label/indication | B / 42 | 4 | 3,2 |
| Réserve en fin d'indication | B / 20 | 9 | 7,2 |

La taille des lignes normales n'est **pas une constante de 76 dp**.
[`materialui_compute_entries_box_default`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/menu/drivers/materialui.c#L3350)
calcule la hauteur comme la hauteur métrique d'une ligne de police principale,
plus les hauteurs des lignes d'indication repliées, plus `B / 10 + B / 13`.
Dans l'exemple, ce dernier espacement vaut environ 32,53 pixels, soit 26,02 dp.
La hauteur finale dépend donc de la police réelle, de la largeur disponible,
des icônes et du texte. Une ligne sans indication est plus courte qu'une ligne
avec deux indications repliées.

La hauteur de police n'est pas assimilable à sa taille nominale :
[`materialui_init_font`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/menu/drivers/materialui.c#L9622)
arrondit `font->metrics.height`. Le
[`pilote FreeType`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/gfx/drivers_font_renderer/freetype.c#L168)
la tire de `face->size->metrics.height / 64` après sélection de la taille en
pixels. Une adaptation avec la police Android propre à CorvusRetro doit donc
mesurer ses propres hauteurs, ascendantes et largeurs de glyphes. Un ratio
constant entre taille nominale et hauteur ne peut être promis pour toutes
les polices et tous les moteurs.

Les dimensions de base sont identiques en portrait et paysage à diagonale et
DPI identiques. MaterialUI déplace par défaut sa barre de navigation à droite
en paysage (`DEFAULT_MATERIALUI_AUTO_ROTATE_NAV_BAR = true`). Sa barre système
interne contient des informations telles que l'heure et la batterie en haut.
La barre d'état inférieure de MaterialUI reste à zéro dans la liste standard ;
elle est activée dans certaines vues de playlists avec miniatures.
CorvusRetro réserve déjà les barres Android natives et ne doit pas ajouter
une seconde barre système par-dessus les marges sûres.

La comparaison des dimensions nominales n'implique pas une copie des polices,
icônes ou rendus de RetroArch. Les ressources originales de CorvusRetro restent
adaptées à ses marges Android et à ses métriques de texte.

### Ordre des fonctions prises en charge

Sources :
[`DISPLAYLIST_SETTINGS_ALL`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/menu/menu_displaylist.c#L13807),
[`DISPLAYLIST_MAIN_MENU`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/menu/menu_displaylist.c#L17744)
et
[`menu_displaylist_parse_load_content_settings`](https://github.com/libretro/RetroArch/blob/b28b8a4b9fa82d5e05cb625a9d8165333b9c129a/menu/menu_displaylist.c#L4779).

- Réglages : Interface utilisateur, Vidéo, Audio et Commandes apparaissent dans
  cet ordre ; Sauvegardes vient ensuite, puis RetroAchievements parmi d'autres
  catégories. Le sous-ensemble de six catégories de CorvusRetro respecte déjà
  cet ordre relatif. Aucune catégorie non implémentée n'est nécessaire.
- Menu principal : Menu rapide si un contenu fonctionne ; sinon chargement ou
  déchargement du cœur. Charger du contenu précède les sous-systèmes et les
  informations. CorvusRetro conserve son accès explicite au déchargement,
  y compris avec une partie, conformément à son comportement demandé.
- Menu rapide : Reprendre, Redémarrer, Fermer, états de sauvegarde, options du
  cœur, commandes, puis d'autres fonctions optionnelles, succès et informations.
  Les fonctions de disque, triche, capture ou shaders absentes de CorvusRetro
  ne doivent pas devenir de fausses entrées.
- Les ajouts propres à CorvusRetro se placent avec les fonctions proches :
  batteries après les états ; joueur contrôlé après les commandes ; retour au
  menu principal en fin de liste. Les identifiants d'actions restent stables
  même si leur position visible change.
- Les Listes de lecture de CorvusRetro conservent Historique seul, selon la
  demande précédente de l'utilisateur ; cette demande prévaut sur les
  favoris et autres listes optionnelles de RetroArch.

Ces mesures décrivent la référence étudiée. La proposition de les appliquer
à CorvusRetro dans la PR21 a été retirée après retour de l'utilisateur : la
présentation de 0.20.0 est conservée, avec ses tailles fixes, sa barre inférieure
et ses libellés d'onglets. Aucun calcul DPI ni menu latéral n'est ajouté.
Les icônes restent des créations vectorielles originales de CorvusRetro.
