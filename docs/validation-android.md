# Validation de la version 0.3

## Vérifications exécutées

- Tests C de la démo portable, y compris AddressSanitizer et UndefinedBehaviorSanitizer.
- Test Java/JNI de la démo avec `-Xcheck:jni`.
- Tests Java/JNI des trois vrais moteurs CorvusCore 0.14.0 : chargement, dimensions, ARGB, PCM stéréo, entrées, restauration d'état, reset, buffers invalides et batterie GBA.
- Construction APK debug ARM64 et x86_64 ; lint Android : zéro erreur.
- Vérification `apksigner`, `zipalign -c -P 16 4` et segments ELF alignés sur 16 Ko pour les deux architectures.
- Notices et copyright CorvusCore présents dans l'APK ; sous-module inchangé.
- Revue indépendante du pont JNI, de l'import et du cycle de vie ; aucun finding bloquant restant.

## Exécution Android

Émulateur AOSP Android 10 x86_64, 480 × 960 pixels, densité 200, exécution
logicielle sans KVM. APK installé et réellement lancé ; captures produites avec
`screencap`, navigation par événements Android et inspection `uiautomator`.

Contenus utilisés : programmes synthétiques originaux produits par
`python tests/generate_roms.py`. Aucun jeu commercial ou BIOS n'est nécessaire.
GB affiche des bandes et inverse sa palette à chaque pression sur A ; GBC reprend
ce programme en mode couleur ; GBA écrit `0x5a` dans la SRAM puis boucle.

Contrôles réalisés :

- Menu principal en listes, choix des trois cœurs et sélecteur de fichiers système.
- Import `.gb`, démarrage réel CorvusGB et affichage de sa frame.
- Pression tactile maintenue sur A : changement effectif de la palette du programme.
- État écrit sur disque, nouvelle pression sur A, chargement de l'état : comparaison des pixels de l'image restaurée, identiques à l'image sauvegardée.
- Passage en arrière-plan puis reprise via le lanceur : session et pixels conservés.
- Rotation portrait/paysage/portrait : commandes visibles et pixels de la session conservés.
- Fermeture du contenu, liste Game Boy persistante, réglages et dialogue de licence.
- Import `.gbc`, affichage CorvusGBC, redémarrage du contenu et fermeture.
- Import `.gba`, affichage CorvusGBA, présence L/R, sauvegarde/chargement d'état et fermeture. Batterie persistée dans le stockage privé, premier octet `0x5a` attendu vérifié sur disque.
- Fichier GB invalide refusé ; liste des fichiers ROM avant/après identique, aucun fichier temporaire restant.
- Historique persistant regroupant les trois contenus ouverts ; aucun crash présent dans le buffer Android dédié.

Les captures du README montrent cette exécution Android. Les bandes affichées
sont le résultat du programme synthétique, pas une illustration de jeu.

## Reproduire sur un appareil de test

Construire et installer l'APK puis générer les programmes :

```sh
python tests/generate_roms.py
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb push build/test-roms/. /sdcard/Download/
adb shell am start -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n com.corvussoftware.corvusretro/.MainActivity
```

Utiliser **Charger du contenu → Downloads** pour ouvrir les fichiers. Sur GB,
appuyer sur A, sauvegarder l'état, appuyer à nouveau sur A, puis charger l'état :
la palette précédente doit revenir. Vérifier ensuite menu rapide, reset,
arrière-plan/reprise, rotation, listes et historique. Ouvrir GBC puis GBA ;
vérifier les commandes L/R et la sauvegarde batterie de ce dernier. Ouvrir
`Corvus-invalid.gb` : une erreur doit apparaître sans ajouter de contenu.

## Limites

Aucun téléphone physique, appareil ARM64, Android récent ou écran 16 Ko n'a été
exécuté pendant ces contrôles ; l'APK ARM64 et les alignements ont été vérifiés
par construction et inspection. La génération de PCM a été testée ; la qualité
audio à l'écoute et la latence réelle restent à mesurer sur téléphone.
TalkBack, manettes physiques et multitouch simultané n'ont pas été testés.
Les programmes synthétiques ne valident ni un catalogue de jeux, ni les performances
sur matériel mobile. GBA reste expérimental conformément à CorvusCore.
