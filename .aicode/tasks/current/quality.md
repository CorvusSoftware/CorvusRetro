Status: PASS pour code et distribution ; CI du catalogue à contrôler après push.
Risk: DEEP.

PASS : base zéro moteur, trois splits indépendants, même package/version/signer durable,
installation cumulative Android10/15, historique/préférences/batteries stables,
restauration de la pause et de la confirmation après recréation ou mortprocess,
rejet des versions et signatures incompatibles, six CTest normaux/ASan,
quinze tests Python,37contrôlesAndroid, compilation/lint et revues sans finding restant.
CI source40333426498, preview signé40333850326, publisher40334541602 réussis.
Vrais assets v0.11.0 téléchargés ; six entrées reproduites depuis leurs octets et un certificat de confiance indépendant.

Limites déclarées : pas de téléphone physique ni exécution ARM64 ; transport HTTPS
sur émulateur non qualifié à cause de la CA privée du proxy, TLS conservé strict.
Ne pas prétendre que le catalogue est en ligne avant vérification raw après push.
