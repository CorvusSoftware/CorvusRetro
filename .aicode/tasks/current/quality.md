CorvusSoftware Quality Gate
Status: PARTIAL
Risk: DEEP

Criteria:
- PASS : cadence pilotée par la réserve audio, PCM exact, rattrapage borné et callbacks sans blocage.
- PASS : sauvegarde périodique hors rendu, acquittements et barrières durables conservés.
- PASS : 19 suites strictes et 19 ASan/UBSan ; revue indépendante sans finding.
- PASS : compilation Android debug/test et lint.
- PASS : 96 contrôles Android GBA portrait/paysage ; APK sans cœur, deux ABI et alignement 16 Ko.
- GATE EXTERNE : vérifier la CI du SHA actif dans la PR avant livraison ; ne pas déduire son état de cette note.
- UNVERIFIED : disparition perçue des parasites sur Redmi Note 13 5G/casque filaire/Pokémon Émeraude.

Evidence:
- CMake build/native et ctest : 19/19 PASS, dont 38 scénarios audio contrôlés et 321 vérifications batterie.
- CMake build/settings-asan avec CORVUS_SANITIZE=ON et ctest : 19/19 PASS.
- Android assembleDebug/assembleDebugAndroidTest/lintDebug : PASS. Instrumentation playback system=2 : 48 contrôles dans chaque orientation, sans nouveau crash.
- check_core_apks.py : PASS ; frontend core-free : PASS ; test_release_workflows.py : 8/8 PASS.

Review:
- Revue indépendante finale du contrôleur, de la boucle, du worker et des tests : aucun défaut concret relevé.

Open risks:
- Pas d'écoute physique ni de ROM du jeu signalé. Un cœur trop lent ou des retards système arbitraires peuvent toujours épuiser la réserve.

Decision:
- Correctif prêt pour revue du code et CI distante. PARTIAL concerne la disparition perçue des parasites sur le matériel signalé : pas de téléphone ni de ROM disponible pour cette preuve.
