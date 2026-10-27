Objectif : supprimer le corbeau provisoire avant l'icône réelle au lancement d'un jeu.
Périmètre STANDARD : menu/open_game, notifications C et disponibilité de la texture GL ; aucun changement réseau, ABI CorvusAPI, cœur ou sauvegarde.
Contrat : une seule carte après identification ; descente seulement après préparation de l'image ; visuel stable pendant toute l'animation. Échec image après3s : fallback unique figé. Progression indépendante et animation5.32s conservées.
Plan : reproduire sur0.15.1 avec réponse image bloquée ; enlever la carte générique ; différer l'animation jusqu'à image prête ; tests attente/timeout/pause/queue/unload ; Android avec identification/image retardées ; build/lint/ASan ; main/CI et APK0.15.2/code18 signé.
