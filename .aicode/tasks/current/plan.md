Objectif : configuration du sous-système visible si cœur chargé et options du cœur dans le menu rapide, après consultation des sources RetroArch.
Portée STANDARD : menu_driver, tests natifs et parcours Android, documentation ; version0.17.1/code21.
Référence vérifiée : RetroArch af622105435d776c1ce203c28387cbc27e0c7925/menu/menu_displaylist.c. Sous-systèmes normalement conditionnés par leur déclaration ; options avant commandes, état vide supporté.
Contrat local : afficher la configuration dès engine.handle, masquer après unload complet, conserver après fermeture du jeu ; options séparées de la surimpression, retour pause/focus/scroll intact.
Limite établie : CorvusAPI publié et pin actuel sans extension sous-systèmes/options. Aucun faux câble link ou réglage moteur ; pages explicitement indisponibles. Pas de modification de sous-module ou contrat ABI privé.
Vérification P0 : native_menu réel GBA et corefree ; absence sans cœur, apparition avant contenu, disparition unload, maintien fermeture/échec batterie, navigation options portrait/paysage. P1 : tests natifs normal/ASan, APK/lint/testAPK, émulateur deux orientations, revue indépendante, CI SHA exact et APK signé.
