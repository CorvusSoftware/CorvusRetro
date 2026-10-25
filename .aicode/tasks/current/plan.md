# CorvusRetro 0.4

Objectif : supprimer tous les composants de laboratoire, ajouter un véritable cycle de vie de cœur chargé/déchargé, refaire les icônes et adapter le thème NVIDIA Shield de MaterialUI.

Contexte ciblé : MainActivity (menus et choix), NativeCore/core_bridge (handle), EmulationSession/PlayerActivity (propriété et fermeture), Ui/ressources (thème/icônes), CMake/Makefile/CI (nettoyage).

Plan :
1. Étudier MaterialUI, son thème Shield, les icônes et la commande de déchargement. Terminé.
2. Terminé : retirer les anciens fichiers C/Java/JNI, leurs tests, menus, ressources et références ; remplacer les tirets longs par de la ponctuation simple.
3. Terminé : centraliser la propriété du cœur sur la file d'émulation : charger sans contenu, fermer/décharger, changer de cœur et réouvrir un contenu.
4. Terminé : implémenter la palette Shield, les nouvelles icônes originales et la tête de corbeau.
5. Terminé : tests du cycle natif, compilation/lint, contrôle Android des transitions, sauvegardes et captures ; revue indépendante.
6. Instructions cloud et documentation actualisées. Publication autorisée sur main avec vérification du workflow Android APK.

Invariants : préserver contenus et sauvegardes, fermer après persistance, aucun appel concurrent par handle, notices CorvusCore conservées, sources du sous-module intactes. Aucun tiret Unicode long dans les fichiers du dépôt parent.
