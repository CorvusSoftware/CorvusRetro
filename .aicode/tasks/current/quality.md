Status: BLOCKED pour la demande complète ; PASS local pour la ligne unique.
Risk: STANDARD pour le diff exécutable ; contrat link à traiter séparément.

Critères PASS : action cœur unique selon handle, conservation après échec batterie,
retour Charger après succès, indices avec contenu et sans contenu, téléchargeur zéro cœur.
Preuves : cinq CTest normaux et ASan/UBSan, zéro cœur, catalogue5/5,
APK base/lint et pack explicite, composition/signature/alignement16Ko des deux APK,
Android15 base/pack/rotation/jeu existant, revue indépendante sans finding concret.
Aucune dépendance ou clé de signature changée. Publication et CI exacte à contrôler.

Critère FAIL : sous-système Deux joueurs non implémenté dans l’ABI actuelle.
Le SDK privé de CorvusCore ne peut pas remplacer un contrat CorvusAPI public.
Prochaine action : proposition CorvusAPI, adaptateur CorvusCore, puis intégration du menu.
Ne pas marquer la tâche complète DONE. Aucun téléphone physique testé.
