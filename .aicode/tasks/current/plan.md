Objectif : corriger les popups de progression répétés et examiner le grésillement GBA signalé sur Pokémon Émeraude, après la version 0.22.0.
Budget : DEEP. Base main dcf5542. Branche fix/progress-popup-and-audio.

1. Reproduire les réapparitions identiques et les variations RAM temporaires avec rcheevos réel et un programme de test original ; distinguer progression et déblocage/son.
2. Ignorer les répétitions de la même valeur par succès sans prolonger le popup, y compris après expiration/masquage et entre plusieurs succès ; conserver les vrais changements et effacer l'historique à la sortie du jeu.
3. Auditer la chaîne PCM GBA, fréquence, gain et cadence AAudio. Reproduire la sous-alimentation logicielle sur un calendrier GBA/écran60Hz, corriger minimalement si avérée et comparer les échantillons avant/après.
4. Tests stricts et ASan ciblés puis frontières Android, revue indépendante et PR avec CI exacte. Aucun moteur, API, pin, sauvegarde ou asset publié modifié.

Limites : aucune ROM de Pokémon fournie ; utiliser uniquement du PCM et des programmes synthétiques originaux. Ne pas prétendre à une écoute sur téléphone/Bluetooth depuis un émulateur sans sortie physique.
