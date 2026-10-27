Objectif : interface jeu conforme aux deux captures RetroArch, avec commandes propres GB/GBC/GBA et menu rapide en liste plein écran.
Périmètre DEEP ciblé : input/play_layout, nouveau renderer C overlay, primitive GLline, menu_driver et navigation sousmenus, tests et ressources menu.
Contrats : ratio natif et zones hors commandes, min48dp téléphones et multiwindow proportionnel ; GB/GBC8 boutons/GBA10 ; diagonales et multitouch inchangés ; masquage réel/réversible sans inputs bloqués ; menu pause, retours sousmenus et sauvegardes réelles ; fermeture garde cœur et refus stockage sans perte.
Références : captures utilisateur portrait/paysage, retropad.cfg normalized descriptors/hide et menu_displaylist.c:4740-4830 Resume/Restart/Close/States.
P0 : géométrie30 combinaisons/tous inputs et surfaces ; renderer traits neutres et feedback ; menu plein écran, save/load moteur réel et échec stockage ; reprise après hide/submenu/rotation ; tests9 normal/ASan et Android six casGB/GBC/GBA portrait/paysage.
P1 : pipeline RA et notifications sur nouveau viewport ; corefree/build/lint/archives/signature ; revue indépendante finale et publication autorisée0.17.0/code20.
