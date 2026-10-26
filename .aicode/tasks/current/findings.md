Catalogue0.10 index.json format1 est vide. Le pack releasev0.10.0-cores.1 existe mais c’est une mise à jour complète, pas3modules individuels.
APK publiée : versionCode11/versionName0.10.0, contient3modulesARM64/x86_64. Source a366246.
Ne pas casser les clients0.10 : utiliser nouveau feed cores-v2.json format2 pour0.11.
Contraintes : aucun dlopen depuis stockage téléchargé modifiable, préserver contrôle signature et confirmationAndroid.
Piste : APKfeature splits officiels (samepackage/version/signer), MODE_INHERIT_EXISTING, nativeLibDir extrait parOS, découverte après redémarrage.
Indicescatalogue partiels : frontend import ancien if(system<3) utilisait mauvais suffixe si GBA seul=index0. Corriger vers coreId stable à toutes les frontières.
Le SDK et CorvusAPI restent figés/intacts. Câble link nonimplémenté, propositiondocs/subsystems-corvusapi-proposal.md toujoursvalable.

Android35 peut bloquer silencieusement un lancement de confirmation depuis un receiver en arrière-plan (BAL_BLOCK). CoreInstallState conserve désormais Intent OS privé via URI liée à la session et le lance depuis MainActivity au focus. Dans le harness, attendre la fin du verrou/commit SharedPrefs avant finishInstrumentation. Une selfupdate peut tuer son receiver final : au prochain lancement, SessionInfo absent réconcilie l’état terminé et libère la pause. Preuve SessionApplied=true et CoreInstallState vide après relaunch.
