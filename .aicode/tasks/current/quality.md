CorvusSoftware Quality Gate
Status: PARTIAL
Risk: DEEP

Criteria:
- PASS : audio FIFO, amorçage, adaptation bornée, reprise et callbacks tardifs testés.
- PASS : croix portrait abaissée, bornes et autres géométries préservées.
- PASS : suppression confirmée, sauvegarde en échec conservant le cœur, identité stable et fichiers protégés.
- UNVERIFIED : parcours Android réels sur le diff final et CI de la PR, en cours.

Evidence:
- 17 suites natives strictes et 17 ASan/UBSan passent.
- 284 contrôles audio et 11 tests JVM de stockage passent.
- Frontend sans cœur et 8 tests de workflows passent.
- Build debug/runner deux ABI, signature et alignement APK passent.

Review:
- Revue indépendante effectuée ; les deux défauts concrets sont corrigés et relus.

Open risks:
- Essais Android et lint/build release finaux en cours.
- Aucun appareil physique ni écoute audio qualifiés.

Decision:
- Conserver VERIFY jusqu'aux preuves Android et CI ; ne pas présenter une release comme publiée.
