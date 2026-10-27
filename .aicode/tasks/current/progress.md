État : VERIFY. Amont21044d8 propre et synchronisé avant modification.
Demande : aucun corbeau provisoire avant l'icône du jeu.
Bug reproduit sur APK0.15.1 avec image retardée et screenshot ; test échoue sur absence du popup provisoire.
Correction native et GL implémentée ; carte générique de lancement supprimée, attente image sans bloquer l'émulation, fallback3s figé et annulation au déchargement.
Preuves locales :9/9 CTest normal et9/9 ASan/UBSan ;5260 contrôles notifications ; Gradle Debug/Release/testAPK/lint PASS ; base sans moteurs, six modules/16KB et catalogue temporaire vérifiés ; tests archives4 et workflows6 PASS. Android15 x86_64 :26 contrôles portrait et26 paysage PASS, identification puis image bloquées, cache froid.
Revue ciblée du diff final : cohérence des horloges et de la queue, choix image avant timeout, stabilité des visuels, aucune attente réseau dans le rendu ; aucun défaut P1/P2 identifié.
Prochain : publication main/CI exacte, signature durable et APK0.15.2/code18 ; conservation des actifs0.15.1.
