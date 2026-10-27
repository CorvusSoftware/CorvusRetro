État : VERIFY, code0.16.0/code19 prêt pour CI et publication.
Amontac4479f propre et synchronisé avant modifications.
Livrable : logo corbeau blanc/fond noir pour launcher adaptatif ; cœur compatible chargé explicitement avant import/historique ; Historique seul ; Fermer contenu garde le même cœur ; switches arrondis et navbar maison/engrenage.
cr_engine_unload_game sauvegarde avant mutation et conserve handle/module/output/API/system. cr_engine_close reste le déchargement complet. RA et notifications sont invalidés après fermeture réussie. Import SAF tardif refusé via finish_import(false) si cœur absent/incompatible.
Preuves :9/9 CTest normal,9/9 ASan/UBSan, corefree PASS ; moteur réelGB/GBC/GBA fermeture/réouverture et refus sauvegarde ; gardes frontend/import/historique ; géométrie et action des switches.
Gradle Debug/Release/testAPK/lint PASS ; base sans moteurs et six modules16KB PASS ; archives4/workflows6 et catalogue temporaire six entrées PASS ; texte/diff propres.
Android15 x86_64 :78 contrôles navigation portrait et78 paysage PASS ; captures launcher, Historique seul, blocage sans cœur, fermeture gardant cœur, réouverture puis unload et ON/OFF dans trois thèmes inspectées. Succès/images37 contrôles PASS, dont fermeture conservant cœur après déblocage et nettoyage des notifications.
Revue indépendante finale : aucun finding P1/P2. Simplification : ACT_SYSTEM et return_page retirés car devenus inutiles.
Prochain : publication main, CI exacte, signature durable, tag immuablev0.16.0, vérification des fichiers publics et mise à jour README ; actifs0.15.2 conservés.
