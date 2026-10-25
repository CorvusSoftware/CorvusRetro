Ancien lecteur : titre 56dp, positions fixes, quatre disques directionnels, Select/Start adjacents.
Références publiques consultées : common-overlays retropad, flat/gba, neo-retropad. Configurations normalisées portrait/paysage ; flat/gba portrait place son menu à x=.5/y=.94792. Corvus garde son code et ses ressources originaux.
play_layout.c/h fournit vidéo, panneau portrait, croix et cibles ; frontend dessine et touche le même résultat. Croix diagonale sur les neuf secteurs avec centre neutre, A/B radiaux, menu isolé des entrées moteur. Échelle bornée pour éviter les collisions sur petites fenêtres.
Un glissement démarré sur Menu ne devient pas un bouton moteur ; l’ouverture du menu efface toutes les entrées. Rotation conserve le moteur et annule les appuis.
Android15 sans KVM : ANR SystemUI au démarrage avant essais valides ; aucun crash fatal de CorvusRetro pendant les essais GB/GBA.
