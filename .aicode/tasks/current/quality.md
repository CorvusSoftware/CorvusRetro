Résultat : PASS local
Trois CTest passent normalement et sous ASan/UBSan, APK ARM64/x86_64 et lint sans erreur, signature et alignement16Ko valides.
ÉmulateursAndroid10/15 : marges et input portrait/paysage ; Android10 : ancien dark migré vers corvus,3choix persistants, aucun crashapp. Android15 : menu, sélection thèmes, chargement/déchargement et rotation exercés après démarrage à froid lent et ANR deSystemUI indépendant.
Revue indépendante et complément couleur de barres : aucun finding bloquant. Politique sans tirets longs et git diff --check passent.
Limites : pas de téléphone physique ni exécutionARM64 ; encoches et navigation gestuelle testées dans le calcul C, sans essai matériel. CI après publication à contrôler.
