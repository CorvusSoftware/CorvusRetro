État : VERIFY. Branche feat/playback-and-core-management, base main 7303a32.
Référence RetroArch documentée dans docs/av-retroarch-reference.md. Tous les changements fonctionnels sont implémentés.

Preuves finales hôtes : 17 suites natives strictes et 17 avec ASan/UBSan passent, dont 284 contrôles AAudio ; frontend sans cœur et 8 tests de workflows passent. 11 nouveaux tests CoreStore passent, ainsi que les tests JVM existants. Base debug et runner compilés pour les deux ABI ; signature, alignement 16 Ko et zéro cœur vérifiés.
Revue indépendante : entrées effacées après échec EGL et callback d'erreur AAudio tardif sécurisés. Nouvelle revue audio et exécution des 284 contrôles : aucun finding restant.
Android : agent android_playback_removal_validation utilise exclusivement le profil debug TCP5563, phases playback/core_removal dans RetroAchievementsInstrumentation.java ; build final vert, essais en cours. Lint/JVM et build release finaux dans /workspace/av-android-final.log.

Prochaines actions : terminer et inspecter les parcours/captures Android, consigner les limites de matériel, commit/push PR, vérifier CI exacte. Ne pas modifier les moteurs, l'API, leurs pins ou les fichiers de contenu. Pas de fusion/release dans ce lot. Logs hors dépôt /workspace/av-* et playback-final-runtime.log.
