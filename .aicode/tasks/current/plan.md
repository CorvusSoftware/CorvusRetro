Objectif : animation RetroArch pour les cartes de lancement et de succès, en haut au centre.
Référence lue : gfx_widget_achievement_popup.c, descente330ms, ouverture330ms, maintien4s, fermeture330ms, remontée330ms ; ease-out quadratique et centrage conservé pendant l'ouverture.
Périmètre : gfx/widgets/notifications, clip du painter dans menu_driver, tests natifs et qualification Android ; aucun changement réseau, SDK, sauvegardes ou progression.
Plan : chronologie déterministe ancrée au premier affichage ; badge seul puis révélation du texte par clip ; repli et sortie ; tests positions/clip/phases/queue ; Android portrait/paysage ; review, CI et APK0.15.1.
Vérifications P0 : badge centré en descente, aucun titre avant ouverture, carte centrée pendant ouverture/fermeture, maintien complet et queue5.32s, pas de phase sautée après chargement.
P1 : géométrie GB/GBC/GBA, images tardives et restitution du clip, build/lint/ASan, vidéo/captures Android réelles.
