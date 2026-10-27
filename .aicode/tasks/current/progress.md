État : DONE,0.15.2/code18 publiée et vérifiée. Amont21044d8 propre et synchronisé avant modification.
Demande : aucun corbeau provisoire avant l'icône du jeu.
Bug reproduit sur APK0.15.1 avec image retardée et screenshot ; test échoue sur absence du popup provisoire.
Correction native et GL implémentée ; carte générique de lancement supprimée, attente image sans bloquer l'émulation, fallback3s figé et annulation au déchargement.
Preuves locales :9/9 CTest normal et9/9 ASan/UBSan ;5260 contrôles notifications ; Gradle Debug/Release/testAPK/lint PASS ; base sans moteurs, six modules/16KB et catalogue temporaire vérifiés ; tests archives4 et workflows6 PASS. Android15 x86_64 :26 contrôles portrait et26 paysage PASS, identification puis image bloquées, cache froid.
Revue ciblée du diff final : cohérence des horloges et de la queue, choix image avant timeout, stabilité des visuels, aucune attente réseau dans le rendu ; aucun défaut P1/P2 identifié.
Source e31dd9e66c11839358c12df29ef0e5426cc737fc publiée sur main et tag immuablev0.15.2. Actifs0.15.1 conservés.
CI40521807336 SUCCESS ; build signé40522298624 SUCCESS ; publication40522728518 SUCCESS.
APK public CorvusRetro-0.15.2-base-release.apk SHA2564751e8bf9e7d0a261f3a4d9d1c5777801a3743070cd6c4cd43ece8d0bda086f9 ; certificat durable vérifié contre pin indépendant, version18, zéro moteur et noticeMIT conservée.
Six ZIPs publics vérifiés : export mémoire, ABI,16KB, tailles/hash ; identiques aux actifs0.15.1, aucune modification de catalogue fournisseur nécessaire. Aucun JSON réintroduit dans le frontend.
Android10 x86_64 : mise à jour signée0.15.1 vers0.15.2 acceptée sans réinstallation ; thèmeASUS et ligne historique corvus-preserved.gba conservés visuellement ; aucun crash applicatif journalisé.
README pointe sur l'actif vérifié ; reçu local /workspace/setup/launch-artwork152-qualification.json.
Limites : réponses compte et images synthétiques ; pas de compte personnel, téléphone physique ou runtimeARM64 qualifié.
