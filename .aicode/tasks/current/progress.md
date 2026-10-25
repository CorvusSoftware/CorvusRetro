État : ACTIVE
Dernière étape : refonte implémentée, revue indépendante et essais Android 10 réussis.
Prochaine étape : publier sur main et attendre la CI APK.

Quality Gate : PASS pour la refonte sur Android 10 (émulateur logiciel).
Preuves : make test ; make jni-test -Xcheck:jni ; assembleDebug ; lintDebug (0 erreur, 2 avertissements).
Runtime : bibliothèque/réglages, licence hors ligne, lancement laboratoire, pause/reprise, reset et retour Android vérifiés.
Direction maintenue : position écran du carré déplacée de (231,354) à (303,354) ; entrée ignorée en pause vérifiée par pixels.
Arrière-plan/reprise via lanceur : laboratoire et état pause conservés ; retour à la bibliothèque réussi.
Revue indépendante : aucun finding bloquant. Rotation réinitialise toujours volontairement la démo.
Captures réelles : docs/screenshots/ (Android 10, 480x960).
Limites : téléphone physique, TalkBack, multitouch simultané et autres versions Android non testés.
Avertissements lint : règles de sauvegarde Android 12 ; constructeur spécialisé de DirectionButton utilisé uniquement par code.
