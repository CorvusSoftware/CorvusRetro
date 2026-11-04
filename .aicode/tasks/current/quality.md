CorvusSoftware Quality Gate
Status: PASS
Risk: DEEP

Criteria:
- PASS : découpage appliqué aux sources, listes/réglages/actions séparés de la navigation et du lifecycle.
- PASS : API publique, structure cr_ui, identifiants, préférences, stockage et ordre des callbacks conservés.
- PASS : types de lignes indépendants du renderer, contrats privés et liste CMake commune aux deux assemblages.
- PASS : corps des 93 fonctions et tables conservés après renommage privé/extraction initiale ; aucune logique dupliquée.

Evidence:
- 21 suites strictes et 21 ASan/UBSan PASS, incluant menu, configuration, états, moteurs, entrée, son et registre.
- Frontend core-free PASS ; compilation debug/test et lint Android PASS.
- APK : signature debug, zéro cœur, deux ABI et alignement 16 Ko.
- Parcours Android portrait et paysage complets PASS sur la base finale, captures inspectées et journaux crash vides.
- Comparaison ponctuelle des fonctions, enums, tables et compteur unique PASS ; git diff --check et politique de texte PASS.

Review:
- Architecture relue indépendamment avant extraction ; diff relu ensuite sans finding concret.
- Aucune nouvelle dépendance, thread, API publique ou modification des moteurs/pins.

Open risks:
- Qualification sur émulateur API29 x86_64 avec programmes originaux et transport contrôlé ; pas de téléphone physique/runtime ARM64/réseau production/écoute audio qualifiés.

Decision:
- Les preuves locales couvrent le refactor et sa livraison en PR. Vérifier la CI du commit exact avant PR prête. Fusion et release hors périmètre.
