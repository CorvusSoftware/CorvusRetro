État : ACTIVE, reprise de 0.17.0/code20 sur main b2afc4f.
Les six cas Android conservés ont réussi (40 contrôles chacun), CI40542270785 et build signé40542897779 SUCCESS pour b2afc4f ; publication absente.
Correction finale : SELECT tronqué en paysage, reproduit par test avec dimensions fractionnaires. Marge de caption de1 pixel bornée à la zone disponible ; aucun autre changement fonctionnel.
Preuves reprise :9/9 natifs et9/9 ASan/UBSan (hors sandbox ptrace), Gradle Debug/Release/testAPK/lint PASS ; archives4/workflows6 PASS. Revue indépendante ciblée sans finding P1/P2.
Prochain : qualification Android sur correction finale, commit et CI exacte, nouveau build signé, tag/publication puis contrôle des actifs publics et mise à jour signée.
