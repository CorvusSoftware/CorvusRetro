Reprise sur main54cb1b7 après migration CorvusAPI et packs APK0.10.

Objectif utilisateur : ligne Charger/Décharger unique et sous-système Deux joueurs (câble link) conditionnel.
Budget STANDARD pour la ligne et ses régressions ; le câble nécessite un contrat public distinct.

1. Sauvegarder le prototype local et avancer sur main sans écraser les nouveaux commits. Fait.
2. Adapter la ligne unique aux modules CorvusAPI et au mode sans cœur. Fait.
3. Vérifier noyau/menu, ASan/UBSan, zéro cœur, APK/lint, packaging base/pack et émulateur.
4. Documenter le contrat manquant avant toute modification d’ABI ou fournisseur.
5. Publier la partie compatible sur main et contrôler sa CI exacte.

Ne pas activer un faux sous-système : l’ABI0 actuelle ne permet ni déclaration ni câble.
Prochaine étape pour le link : proposer puis faire valider l’extension dans CorvusAPI,
implémenter l’adaptateur CorvusCore dans son propre dépôt, enfin intégrer le frontend.
