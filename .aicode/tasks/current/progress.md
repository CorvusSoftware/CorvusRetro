État : implémentation et qualification locale terminées. Branche fix/home-quick-branding, basef334a80.

Trois corrections demandées : bouton Réglages du titre Accueil retiré (dessin/cible/action), menu rapide rangé par rôle avec fermeture finale et libellés explicites, modèle du logo de l'organisation repris. Lanceur : PNG officiel intact ; pictogrammes vectoriels de petit format avec front/yeux/plumes/long bec. Aucun détourage généré intégré.

Les 21 suites natives strictes et 21 ASan/UBSan passent ; les tests vérifient absence de cible cachée et ordre/toucher/clavier pour un/deux joueurs en portrait/paysage. Build/lint et contrôles APK passent, atlas : 196 contrôles inspecté. Parcours Android ciblés portrait/paysage : sauvegarde et rechargement d'un vrai état, options du cœur et fermeture sans décharger le cœur ; captures inspectées et crash.logs vides. Revue sans finding bloquant.

Base debug SHA256 :edee3f440048ca6c7e635fbcdb22203c8311673d263395d075542a4aae570338. Preuves : /workspace/setup/home-quick-branding. Un dialogue System UI du boot a été fermé avant reprise du contrôle ; pas de modification de l'APK pour le contourner.

Livraison en PR, contrôler CI du head publié avant sortie du brouillon. Pas de fusion/release pour cette nouvelle demande ; version et signature de publication inchangées.

Nouvelle autorisation explicite du mainteneur : une fois terminé, fusionner PR #31 et publier la version corrective. Préparation0.23.1/code32, même certificat durable. CI GitHub encore en file au début de cette reprise ; contrôler le nouveau head avant fusion, puis CI main, build signé, qualification de la mise à jour31->32 et publication exacte. Reprise : /workspace/setup/release0231/state.json.
