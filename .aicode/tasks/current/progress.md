État : READY_FOR_REVIEW, catalogue réel vérifié à activer dans ce commit.

Objectif : téléchargement de cœurs natifs comme RetroArch hors Play, sans installer des APK.
Source1c509160c35c8914fca7f642555d1cfc070dab36 et tag v0.12.0 publiés.
CI40342714444, preview signé40343296103 et publisher40343898470 réussis.
Base0.12/code13 et six ZIP de cœurs publiés, même signer durable; catalogue candidat
sur automation/core-catalog-40343898470-1, seul fichier cores-v3.json modifié.
Sept assets publics téléchargés et vérifiés; six entrées reproduites exactement.
39contrôles repassés avec les ZIP publics. Vrai APK0.12 mis à jour sur Android10,
ASUS et historique conservés, captures examinées.
39contrôlesAndroid15, vrais jeux GB/GBA, batterie GBA identique, recréation pendant
transfert, liste et chargement immédiats, six CTest normaux/ASan, zéro cœur,
19testsPython, Gradle/lint et revue indépendante sans P1/P2 restant.

Prochaine action : pousser ce catalogue vers main, vérifier raw cores-v3.json = six
entrées puis observer la CI du commit exact. Aucun nouveau changement applicatif requis.
Captures/logs hors git dans /workspace/setup, assets dans /workspace/artifacts/CorvusRetro-0.12-release.
Limites : pas de téléphone physique/ARM64 runtime; HTTPS des tests Android simulé,
TLS public inchangé. Aucun SDK, sous-module, jeu, réglage ou sauvegarde modifié/supprimé.
