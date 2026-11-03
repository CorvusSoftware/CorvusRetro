État : ACTIVE, vérification Android de la refonte. Branche feature/mobile-interface-redesign, base main69952eb.

Périmètre accepté : maquette1..10 et menu rapide12 ; écran11, commandes de jeu et audio conservés. Navigation5onglets, bibliothèque/recherche/filtres, cœurs/détails,32 états nommés/aperçus/actions/partage et réglages sont implémentés. Anciens manuels/auto/.sav/.srm/sous-systèmes/options/RA restent accessibles. Aucun moteur/API/pin modifié.

Preuves actuelles : 21/21 suites natives strictes et21/21 ASan/UBSan ; native_interface108 contrôles, state_store135 ; Java25 tests ; rendererprobe3451 contrôles strict/ASan. FrontendServicesInstrumentation31 contrôles Android réels PASS. Frontend core-free et8 tests de publication PASS ; compilation debug/tests etlint PASS.

Revue indépendante :3P2 corrigés puis recheck sans finding. Prévalidation état avant changement de partie, tickets IME C/JNI par génération, refresh après erreur finale IO. Un test Android a ensuite trouvé l'oubli ui_editing3/4 dans la garde d'entrée NativeActivity : clés et axes corrigés. APK reconstruit et nouveau runner figé.

APK actuel ee15f9cf49893132af016b67637f279d5ddd0f65f8f3f360f321028a6b8a2442 ; testAPK1079beeb30101d0bdac603a73fd63bedbacd34b7ee7f7dfdf2985ca2092069b5. Source runnerSHA256d2250b573eef82638fae9d2a641726910285caa6ec8aed2eb452bc4bd8589dc5.

Runtime CorvusUI TCP5563 actif. AVD /workspace/toolchains/avd ; PythonADB PYTHONPATH=/workspace/toolchains/python-adb. Premier ANR SystemUI du boot inspecté, Wait pressé et focus app restauré ; aucune protection Android désactivée. Ne pas toucher profil signé5561.
Le parcours portrait est en cours via /workspace/setup/interface-redesign/run-ui.py --install --orientation portrait, log portrait-current.log, processPID54046/session40255 (vérifier s'il survit à une autre interruption). Le pilote extrait captures/logs/reçus dans runtime/ui-portrait-<timestamp> puis écrit runtime/latest-portrait.txt. Après succès, exécuter paysage avec le même APK sans --install et inspecter les captures. Les anciens essais rouges sont conservés comme preuves de reproduction.

Prochaines étapes : terminer portrait/paysage et preuve Entrée/IME_DONE/stale ticket ; corriger tout vrai échec, inspecter captures, documenter les résultats puis PR et CI exacte. Source et docs peuvent être publiées en brouillon pendant les tests ; pas de fusion ni nouvelle version autorisée dans cette tâche. Les agents de l'étape précédente ne sont plus actifs après reconnexion ; fichiers et reçus sont préservés.
