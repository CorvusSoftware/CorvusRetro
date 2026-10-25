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
moteur fictif n'est nécessaire au menu Android. Les cœurs sont intégrés à l'APK,
le chargement concerne leur instance native et non l'installation d'un fichier externe.

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

CorvusRetro utilise cette palette dans ses composants Android et conserve un
second thème sombre bleu. Les choix de thème sont mémorisés.

## Icônes

Le pilote associe `MENU_ENUM_LABEL_CORE_LIST_STR` à `MUI_TEXTURE_CORES`, puis à
[glui/cores.png](https://github.com/libretro/retroarch-assets/blob/d9f969054dc7fbb6fa89519036d2b971e0855b51/glui/cores.png).
Cette icône est un bloc plein avec un symbole découpé. `load_content.png`,
`playlist.png`, `settings.png`, `file.png` et `history.png` ont aussi été examinés.

CorvusRetro dessine ses propres vecteurs : module avec tête de corbeau en réserve,
profil de corbeau pour l'identité, classeur pour le contenu, lignes pour les listes
et réglages, horloge pour l'historique, éjection pour le déchargement. Les formes
restent lisibles à petite taille. Aucun fichier graphique de RetroArch n'est embarqué.
