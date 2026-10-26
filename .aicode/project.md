# CorvusRetro

Frontend Android original C11, version0.13/code14, cible35/min26, NDK27.2, JDK17+, Gradle8.13/AGP8.9.1.
Structure : menu/ UI, cores/ registre/engine, gfx/drivers/gl2, audio/drivers/aaudio, input/ overlay,
frontend/drivers/platform_android+android_backend, tasks/ mailbox, cheevos/ client officielrcheevos,
pkg/android/ Gradle/servicesOS, deps/ dépendancesversionnées. ./gradlew racine délègue au projet.
Base zéro moteur, six .so directs construits et exclus parGradle, sixZIP du cataloguev3.
Anciens cataloguesv1/v2 conservés pour anciens clients, pipelines pack/split supprimés.
CorvusCore0.14.1 pin2d7bdd2020894fa27d153cbecd421da47bcb37f4, CorvusAPI headerunique corvusapi.h
incluant extension mémoireoptionnelle corvus_get_achievements_api version1 ; ABI0 inchangée.
Rcheevos MIT pin1433173220a7eaede6a9ed7a18e94117be1821e0 ; intégrationfrontendoriginaleAGPL.
Succès mode normal, hash àl'ouverture, évaluation aprèsframe émulée uniquement, secret via IME,
token/outboxAESGCMKeystore, POSTfixedHTTPS, workers->mailbox->frontend, générationlogout.
Contenus/sauvegardes inchangés : GB/GBC/GBA suffixesSHA-0/-1/-2 indépendants indicesregistre;
batterie .new/fsync/rename+ackgeneration ; failedsave conserve moteur.
ThèmesIDs0Shield/2Corvus/3ASUS, défautCorvus, ancien1dark retiré. WindowInsets safeviewportcommun.
Tests : make test et SANITIZE=ON, GradleassembleDebug/lintDebug ; runnerCoreRepositoryInstrumentation
par défaut, RetroAchievementsInstrumentation par -PcorvusTestRunner pourauth/crypto/jeuréel.
Preuves détaillées docs/retroachievements.md ; pas de compte réel/téléphonephysique/ARM64runtime revendiqué.
L'utilisateur a explicitement autorisé l'extension des deux SDK et leur publication pourcettetâche.
