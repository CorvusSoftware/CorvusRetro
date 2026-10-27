Cause1 : open_game poussait immédiatement une carte sans URL avant CR_CHEEVOS_GAME_READY.
Cause2 : badge dessinait le corbeau chaque frame tant que image() ne disposait pas de texture, puis le remplaçait à la fin du téléchargement.
Reproduction : APK debug0.15.1 avec nouveau runner et cache froid, réseau synthétique contrôlé ; FAIL waiting-image shows no preliminary crow popup. Capture montre titre identifié et corbeau avant réception PNG.
Correction : carte créée uniquement par GAME_READY ; image_ready prépare la texture sans dessin, IO ou attente réseau ; horloge visuelle ne démarre qu'après disponibilité. Délai image3s depuis premier prepare, choix de fallback seulement après vérification de disponibilité ; image déjà prête au retour d'une pause prioritaire sur le timeout.
Une carte animée conserve son artwork, même lors d'une texture temporairement absente ou de métadonnées tardives. Progression et file8 restent indépendantes ; fermeture efface toutes les attentes.
