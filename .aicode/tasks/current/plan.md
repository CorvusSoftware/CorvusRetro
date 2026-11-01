Objectif : entrée « Sous-système » dynamique ; transferts des sauvegardes de jeu .sav/.srm dans le Menu rapide, états instantanés séparés ; plan d'organisation des réglages sans implémenter de fausses options.
Budget DEEP : sauvegardes persistantes, remplacement de session, copie SAF et annulation asynchrone, thread natif propriétaire du cœur. Agents spécialisés C, documentation, puis revue indépendante.
1. Réutiliser métadonnées de sous-systèmes et interfaces de batterie existantes. Garder IDs et chemins .sav, P2-link, options et format états inchangés.
2. Capture native immuable pour export ; import dans fichier privé borné, confirmation, candidat natif et commit durable avec backup, sans appel cœur depuis Java.
3. Garder import/export liés au joueur et contenu d'origine, annuler proprement picker/activity/busy, ignorer callbacks périmés.
4. Tester roundtrip .sav/.srm, erreurs taille/IO, P2 distinct, reprise après interruption, lifecycle Android et affichage des vrais sous-systèmes, cas sans cœur/batterie.
5. Documenter plan de réglages avec existant/proposé distinct ; revue/CI/fusion et APK signé qualifié puis publié.
