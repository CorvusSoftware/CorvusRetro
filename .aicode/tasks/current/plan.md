Objectif : notifications natives de lancement, succès débloqué et progression avec icônes/badges, selon les comportements RetroArch.
Budget : DEEP, callbacks C, images asynchrones Java/JNI et textures GL, sans changement SDK ni sauvegardes.
Référence : RetroArch dda31fef21aeb472be9de7ee53eb386cc8db1417 ; rcheevos existant.
Plan : événements typés rc_client ; widget C borné ; service images officiel et caches bornés ; rendu GL ; intégration dans viewport sûr ; tests et revue ; publication autorisée sur main/CI/APK.
Vérifications : protocole synthétique avec vraie progression RAM, queue/timing/petits viewports, images hostiles/cache/asynchronisme, CTest normal/ASan, Android build/lint et parcours réel sur émulateur, CI exacte.
Invariants : interface C11 ; Java seulement OS/HTTPS/décodage ; aucun IO dans rendu ; TLS et origines strictes ; aucun badge inventé ; pas de modification des cœurs, contenus ou sauvegardes.

TestMatrix : P0 protocole/mesureRAM+notifications, widgetqueues/timing/géométrie, serviceimages TLS/bornes/cache/async ; P1 JNI/GL vrais pixels et rotation, menu+savehistoriques ; P2 régressiondownload selonamont etCI/qualificationsignée.
