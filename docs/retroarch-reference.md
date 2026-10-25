# Références d'interface

La refonte 0.3 suit le choix explicite **MaterialUI**, adapté à Android tactile.
Références effectivement examinées :

- [RetroArch : menu/drivers/materialui.c](https://github.com/libretro/RetroArch/blob/master/menu/drivers/materialui.c) : onglets principal, listes et réglages ; entrées de liste avec icône et valeur ; barre de navigation ; menu rapide.
- [RetroArch : config.def.h](https://github.com/libretro/RetroArch/blob/master/config.def.h) : valeurs par défaut du pilote et thème.
- [CorvusCore : SDK C](https://github.com/CorvusSoftware/CorvusCore/blob/38829d4935bbdcf4b13261714352362065b78551/sdk/include/corvuscore/bridge.h) : contrat d'intégration des moteurs.

CorvusRetro implémente ces principes avec ses propres composants Java Android,
icônes vectorielles, palette anthracite/bleu et libellés français. Les listes
remplacent les cartes du dashboard 0.2 ; les contenus sont regroupés par console.
Le menu rapide met le cœur en pause et donne accès aux opérations réelles du SDK.
Il s'agit d'une inspiration de navigation, pas d'une reproduction pixel par pixel.

Le sous-module CorvusCore fournit les moteurs maison C++20. Le pont JNI original
utilise son SDK C directement : aucun moteur RetroArch n'est embarqué et l'API
libretro n'est pas nécessaire à cette intégration. L'API C préliminaire est
vérifiée au chargement et sa révision est figée par le gitlink du sous-module.
Les notices/attributions CorvusCore restent inchangées et accompagnent l'APK.
