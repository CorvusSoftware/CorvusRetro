# Faits de la tâche

- Demande : étudier réellement RetroArch, adopter sa navigation MaterialUI et utiliser les cœurs maison CorvusSoftware/CorvusCore. Choix MaterialUI confirmé par l'utilisateur.
- Références consultées : `menu/drivers/materialui.c` et `config.def.h`. Principes retenus : onglets principal/listes/réglages, entrées tactiles et menu rapide. Aucun code/graphisme repris.
- CorvusCore 0.14.0 est figé au commit `38829d4935bbdcf4b13261714352362065b78551` par sous-module. C++20/CMake 3.22.1, SDK C API 0 préliminaire, GB/GBC et GBA expérimental.
- SDK : handles opaques, appels sérialisés, ARGB8888, PCM16 stéréo, états et batterie avec génération acquittée après persistance. Notices/attributions embarquées sans modification du sous-module.
- Revue indépendante : correction d'AtomicFile (API publique), verrou partagé de l'index, file FIFO fermeture/sauvegarde avant prochain chargement, validation d'import avant renommage, conservation de l'import lors de la rotation. Revue finale sans finding bloquant.
- Tests JNI des vrais moteurs et de la démo passent ; C avec ASan/UBSan passe ; APK/lint passent (zéro erreur), signature et alignements ZIP/ELF 16 Ko validés pour les deux ABIs.
- Android 10 x86_64 sans KVM : import SAF et frame GB réels, A modifie la palette, état restauré avec pixels identiques, arrière-plan et rotation conservent la session.
- Le CLI adb échoue à écrire dans HOME readonly. `adb_shell` Python via TCP 5555 fonctionne ; scripts et preuves brutes sous `/workspace/setup`, toolchains sous `/workspace/toolchains`.
- Pas de téléphone physique testé. Ne pas présenter les tests synthétiques comme preuve de compatibilité générale ou de performances mobiles.

- Android GBC : import, frame, reset, fermeture validés. GBA : import, frame, L/R, états et batterie disque `0x5a` validés. Contenu invalide refusé sans ROM/fichier temporaire orphelin.
