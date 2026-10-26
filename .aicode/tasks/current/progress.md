État : READY_FOR_REVIEW, catalogue vérifié prêt à publier sur main.

Code0.11 : 53f997bcf0c3ca670b7fa687a770d65df1eb35cf, base versionCode12.
CI Android40333426498, preview signé40333850326 et publisher40334541602 réussis.
Tag/préversion v0.11.0 publiés : base et trois APK individuels CorvusGB/GBC/GBA.
Branche candidate automation/core-catalog-40334541602-1 (cf275130), seul fichier cores-v2.json.
Six entrées reproduites à l'identique depuis les quatre assets téléchargés, signatures vérifiées contre l'ancienne release installée, alignement16Ko et base12 vérifiés.

Android15 : GBA seul puis ajout GB, batteries SHA-2 identiques, anciens jeux GB conservés.
Android10 : vraie release11 → base12 avec historique et ASUS conservés ; GBA seul puis GB/GBC cumulés ; même GBA repris ; mauvais signer et downgrade refusés sans perte.
Six CTest normaux/ASan, zéro cœur,15testsPython, Gradle/lint et37contrôlesAndroid passent.
Confirmation différée background et restauration après process death validées ; session UIDapp appliquée et réconciliation après relaunch.
Limite de preuve : pas de téléphone physique ni runtimeARM64 ; HTTPS des émulateurs limité par la CA privée du proxy, aucune vérification TLS désactivée.

Prochaine action : commit/push catalogue + preuves, vérifier endpoint raw retourne six entrées puis CI exacte.
AVD35TCP5557 debug ; nouvelle AVD29CorvusRelease011TCP5561 release avec trois modules installés. AncienneAVD29TCP5559 préservée mais non bootable après remount ; ne pas effacer ses données.
Ancrage certificat public : /workspace/setup/trusted-installed-release-cert.sha256. APK publiés : /workspace/artifacts/CorvusRetro-0.11-release/.
Câble link indépendant : proposition CorvusAPI, pas implémenté dans cette tâche.
