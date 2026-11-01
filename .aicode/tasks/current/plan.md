Objectif : développer le plan docs/settings-plan.md en réglages réellement opérationnels, sans options factices. Budget DEEP : menus, média, contrôleurs et persistance traversent C/JNI/Android.
1. Conserver six catégories propres et les clés existantes ; séparer compte global RA et succès du jeu.
2. Ajouter notifications configurables, taille vidéo entière/filtrage, volume, opacité/taille/position tactiles avec restauration.
3. Montrer les manettes réellement connectées et permettre affectation/restauration des touches.
4. Ajouter conservation bornée des backups et états automatiques distincts, erreurs sans perte du jeu.
5. Tests ciblés, natifs/sanitizers/Android, revue indépendante, CI/main et APK signé avec mise à jour qualifiée.
Contrats : root possède menus/préférences/notifications/docs. Agent média possède rendu/audio/layout ; agent manettes possède backend plateformes/contrôleurs/JNI ; agent sauvegardes possède moteur/tests persistance. Pas de modifications CorvusAPI/Core ni du téléchargeur.
