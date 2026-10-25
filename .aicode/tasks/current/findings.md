# Faits vérifiés

Frontend0.4 Java remplacé par un frontend natif original C11. Références RetroArch : platform_unix.c, android_ctx.c, gl2.c, android_input.c et aaudio.c ; aucune copie de leur implémentation.
Java est limité à NativeActivity/SAF/index/préférences/atlas et glyphes système ; C décide les menus, entrées, association des fichiers, rendu et cadence moteur. Les cinq sources natives sont compilées en C ; le SDK des moteurs reste C++20.
JNI : System.loadLibrary nécessaire pour que le ClassLoader trouve imported(), au-delà du dlopen de NativeActivity. Import conservé avant Looper et finishImport(ZI)String après validation C. Java sérialise les copies sousimportLock, y compris après recréation.
Contrat de données0.4 conservé : JSONSharedPreferences, SHA256-system, fichiersROM/batterie/états0.14.0. Un état GBA0.4 a été chargé par le lecteur0.5.
SauvegardesC .new/fsync/rename, récupération.bak, acquittementgénération après écriture. Échecdisque conservelemoteur. Réouverturemêmejeu persisteRAM avantlecturecandidat.
Revue : événementsprécoces, focusmenu rapidepaysage etveilleuniquementjeu corrigés. Rendu texteatlasprémultiplié avecGL_ONE, coordonnéesdemi-texelpourempêcherfuitesdescellules.
Tests CTestnative_engine/native_frontend etASan/UBSan passent, SDKnoninstrumenté. Buildpropre etlint passent. Android10 x86_64 : coresvides/thèmes,GBinput/états/rotation/reprise,SAFannulé/invalide/GBC/GBA,batteriedisque,crédits,défilement,320dp etanciensétats passent. Aucuncrashdanslesexécutionscorrigées.
APK : aucunancienlecteur/menuJava, entréeNativeActivity etJNIservices vérifiées,signature/ZIP/ELF16Ko valides. SourcesCorvusCoreinchangées ; noticeNativeAppGlueApache2 jointe.
Configurationcloudmiseàjour,rejouée,sauvegardéeenbrouillon. Pasdepreuvesphysiques/Androidrécent ; UI OpenGL sansarbreTalkBack.
