État : implémentation et vérification locale terminées, PR #30.

Les icônes décrivent leur fonction : Système, Import, Export, Verrou, Curseurs et Filtre ajoutés dans la réserve existante ; audio, liste, sauvegardes, redémarrage et liaison redessinés. Les cartes d'action et écrans vides ne masquent plus les symboles avec une GBA générique. Continuer retrouve le jeu courant.

Actions, 42 anciens IDs, dimensions et allocation de l'atlas conservés. Pas de modification du jeu, des moteurs, de l'audio ou des dépendances.

Preuves locales : 21 suites natives, 21 suites ASan/UBSan, build/lint Android, contrôles APK et atlas196 passent. Parcours Android portrait complet et captures des menus concernés inspectés ; crash.log vide. Revue indépendante sans finding bloquant. Preuves hors dépôt : /workspace/setup/icon-meaning.

Publication dans PR #30 ; vérifier la CI du commit publié avant sortie du brouillon. Aucune fusion ni release dans cette tâche.

Nouvelle demande du mainteneur : fusionner PR #30 et sortir la version. Autorisation explicite reçue après validation des icônes. Préparation0.23.0/code31 ; mettre à jour version APK, À propos et liens README. Vérifier la CI du nouveau head, fusionner, attendre CI main et build signé, qualifier l'APK signé/mise à jour, puis tag et publication exacte. État de livraison hors dépôt : /workspace/setup/release0230/state.json.
