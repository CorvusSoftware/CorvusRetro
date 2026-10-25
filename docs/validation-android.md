# Validation du frontend natif 0.5

## Tests hôtes et construction

- CMake compile réellement les fichiers du frontend en C11 ; les moteurs CorvusCore restent en C++20.
- CTest `native_engine` : GB/GBC/GBA réels, cœur vide, frames ARGB/PCM, reset, états disque, batterie, fermeture/rechargement et refus de contenu invalide sans perdre le moteur courant.
- Tests de panne disque : un échec d'écriture bloque remplacement et fermeture, conserve la batterie sale et permet une reprise. La réouverture du même jeu persiste la RAM récente avant de relire la batterie.
- CTest `native_frontend` : navigation tactile/manette, thèmes, annulation, association des extensions, appuis simultanés, annulation des entrées, impulsion courte, pause et défilement du menu rapide en paysage.
- AddressSanitizer et UndefinedBehaviorSanitizer exécutés sur notre C portable et ses tests, SDK non instrumenté.
- APK ARM64/x86_64 et lint Android sans erreur.
- Revue indépendante : résultat SAF conservé avant initialisation du Looper, focus du menu rapide visible en paysage et veille limitée au jeu actif.

## Exécution Android

Émulateur AOSP Android 10 x86_64, 480 × 960 pixels, densité 200, sans KVM.
Les essais utilisent les programmes synthétiques originaux générés par
`python tests/generate_roms.py`. Aucun jeu commercial ou BIOS constructeur.

Scénarios exécutés sur le frontend natif :

- Démarrage NativeActivity, menus/rendu C OpenGL, chargement/remplacement/déchargement des trois cœurs sans contenu.
- Onglets, historique existant et choix natif des thèmes Shield et sombre, avec préférences Android.
- GB : appui A avec changement effectif de palette, état enregistré sur disque, seconde modification et restauration avec pixels identiques.
- Arrière-plan/reprise : session et image conservées.
- Rotation portrait/paysage/portrait : EGL recréé, moteur conservé et menu rapide utilisable avec focus défilant.

Sur l'APK avec liaison JNI corrigée :

- Annulation SAF : retour au menu C, puis un nouvel import reste possible.
- Contenu invalide : refus, cœur non chargé et liste des fichiers ROM identique avant/après, sans temporaire résiduel.
- Import GBC et GBA : système associé en C, contenu réellement exécuté et fermeture native.
- Nouvelle batterie GBA : fichier de notre fixture supprimé avant lancement, puis première valeur disque `0x5a` vérifiée après fermeture.
- Aucun crash dans le buffer Android de cette exécution corrigée.
- Un état GBA créé par le lecteur 0.4 a été chargé par le lecteur C 0.5, sans conversion.
- Crédits et licence natifs, défilement, Retour Android et menu à 320 dp vérifiés.
- L'horloge des notifications est réinitialisée à la reprise pour conserver les erreurs reçues depuis le sélecteur.
- Dernier APK : nouvel import GB avec copie sérialisée, entrée native puis fermeture, aucun crash. Le gestionnaire de fenêtres confirme `mHoldScreenWindow=null` dans le menu : la veille n'y est pas bloquée.
- Paquet vérifié : anciens menus/lecteur Java absents, points d'entrée NativeActivity/C et JNI de services présents, notices NDK/CorvusCore embarquées ; signature et alignements ZIP/ELF 16 Ko valides sur les deux ABIs.

## Reproduire

```sh
python tests/generate_roms.py
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb push build/test-roms/. /sdcard/Download/
adb shell am start -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n com.corvussoftware.corvusretro/.MainActivity
```

Tester les cœurs vides, les thèmes, un contenu depuis l'historique, A, les états,
le menu rapide, la rotation et l'arrière-plan. Annuler le sélecteur puis importer
un fichier invalide : l'application doit rester disponible sans fichier résiduel.
Importer GBC et GBA, vérifier le cœur choisi et la persistance de la batterie GBA.

## Limites

Pas de téléphone physique, d'exécution ARM64 ou Android récent validés.
La qualité audio à l'écoute, les performances mobiles et la compatibilité d'un
catalogue restent à mesurer ; GBA reste expérimental. Les menus OpenGL n'exposent
pas encore d'arbre d'accessibilité TalkBack. Les manettes physiques et les gestes
multitouch réels n'ont pas été exercés ; le modèle C teste leurs contrats d'entrée.
