État : DONE,0.16.0/code19 publiée et vérifiée.
Amontac4479f propre et synchronisé avant modifications.
Livrable : logo corbeau blanc/fond noir pour launcher adaptatif ; cœur compatible chargé explicitement avant import/historique ; Historique seul ; Fermer contenu garde le même cœur ; switches arrondis et navbar maison/engrenage.
cr_engine_unload_game sauvegarde avant mutation et conserve handle/module/output/API/system. cr_engine_close reste le déchargement complet. RA et notifications sont invalidés après fermeture réussie. Import SAF tardif refusé via finish_import(false) si cœur absent/incompatible.
Preuves :9/9 CTest normal,9/9 ASan/UBSan, corefree PASS ; moteur réelGB/GBC/GBA fermeture/réouverture et refus sauvegarde ; gardes frontend/import/historique ; géométrie et action des switches.
Gradle Debug/Release/testAPK/lint PASS ; base sans moteurs et six modules16KB PASS ; archives4/workflows6 et catalogue temporaire six entrées PASS ; texte/diff propres.
Android15 x86_64 :78 contrôles navigation portrait et78 paysage PASS ; captures launcher, Historique seul, blocage sans cœur, fermeture gardant cœur, réouverture puis unload et ON/OFF dans trois thèmes inspectées. Succès/images37 contrôles PASS, dont fermeture conservant cœur après déblocage et nettoyage des notifications.
Revue indépendante finale : aucun finding P1/P2. Simplification : ACT_SYSTEM et return_page retirés car devenus inutiles.
Source54a4b71f20e73073cd34770170e92b60439e724f publiée sur main et tag immuablev0.16.0 ; actifs0.15.2 conservés.
CI40530246766 SUCCESS ; build signé40530986470 SUCCESS ; publication40531457540 SUCCESS.
APK public CorvusRetro-0.16.0-base-release.apk SHA25633ba4bab19a4913151ff07fe58d47171cfe0247cd8c29a249a73e35da9180c73 ; certificat durable vérifié via pin indépendant, code19, zéro moteur et noticeMIT conservée. Logo PNG optimisé du paquet identique aux pixels source1254x1254, ressource adaptative présente.
Six ZIPs publics vérifiés : mémoire/ABI/16KB/hash/taille ; octets identiques0.15.2, aucun changement de catalogue fournisseur nécessaire. Aucun JSON de catalogue réintroduit dans le frontend.
Android10 x86_64 : mise à jour signée0.15.2 vers0.16.0 acceptée sans réinstallation ; ASUS et historique corvus-preserved.gba conservés visuellement, navbar et gardes nouvelles visibles ; aucun crash applicatif journalisé.
README pointe sur actif vérifié. Reçu local /workspace/setup/navigation016-qualification.json. Aucun compte personnel, téléphone physique ou runtimeARM64 revendiqué.
