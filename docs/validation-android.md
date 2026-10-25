# Validation Android

## Version 0.9 : feedback et catégories

Cinq suites CTest passent normalement et sous ASan/UBSan. `native_menu` couvre :
arborescence réelle, informations d'un cœur chargé, retour avec filtre/sélection
conservés, fondu du toucher, tap/maintien/reset, timestamps indépendants du rendu,
menu à un doigt, annulation et défilement. Il vérifie aussi le premier flash d'un
tap malgré une frame lente et la consommation des pulses moteur, son extinction
sans entrée coincée, et la désactivation du feedback sans perte d'entrée.
Revue indépendante et compléments latch visuel/reset : aucun problème bloquant.
APK ARM64/x86_64 et lint sans erreur ; signature et alignement 16Ko vérifiés.

Sur Android15 x86_64 (480x960, densité200, sans KVM), données 0.8 conservées :

- Nouveaux réglages et retour dans les parents ; maintien d'une catégorie sans ouverture, puis tap pour l'ouvrir.
- Réglage audio : maintien court sans modification, tap modifie la valeur sauvegardée, restauration de la valeur initiale.
- Afficher les appuis désactivé, choix conservé après redémarrage, puis réactivé.
- Historique dans Listes de lecture ; GB existant réellement relancé depuis sa liste.
- Commande A tenue : comparaison des pixels de sa zone entre repos/appui/relâchement ; l'appui diffère, le relâchement retrouve exactement le repos.
- Informations du vrai cœur GB, accès au menu rapide depuis le menu principal et reprise, menu central après rotation.
- APK final réinstallé : appui de1200ms sur Audio désactivé restaure effectivement la préférence activée ; retour Audio/Réglages/Menu principal correct.
- Aucun crash fatal de CorvusRetro observé ; le démarrage de l'émulateur a de nouveau présenté une boîte SystemUI avant stabilisation et essais valides.

[Références et captures](navigation.md). Les gestes multitouch simultanés restent
vérifiés dans le modèle C ; pas de qualification sur téléphone physique.

## Version 0.8 : écran et commandes

Quatre suites CTest passent normalement et sous ASan/UBSan, dont `native_controls`.
Cette dernière couvre trois systèmes et dix tailles de fenêtre : ratios natifs,
cibles dans les limites et sans collisions, diagonales/centre neutre, menu central,
rendu sans titre, correspondance entre l'icône dessinée et son toucher, pause avec
un autre doigt sur A, reprise, déplacement et annulation sans entrée coincée.
APK ARM64/x86_64, lint, signature et alignement 16 Ko passent. Revue indépendante :
aucun problème bloquant relevé.

Essais Android 15 x86_64, 480 x 960, densité 200, sans KVM :

- Import GB réel par le sélecteur Android ; vidéo et commandes séparées en portrait, menu tactile central puis reprise par Retour Android.
- Appui sur A recalibré : modification des pixels du programme GB original, prouvant l'entrée dans le moteur.
- Portrait/paysage/portrait : contenu conservé, proportions natives respectées, menu central utilisable après rotation et fermeture du contenu par le menu rapide.
- Import GBA réel : boutons L/R visibles et menu central utilisable en portrait et paysage ; aucun crash fatal de CorvusRetro observé. La fixture GBA teste la batterie et affiche un fond noir.
- Surface utile portrait `0,30,480,900`, paysage `0,30,900,480` ; les barres Android restent séparées du lecteur.
- Comme en 0.7, le démarrage à froid a montré une boîte ANR de SystemUI de l'émulateur, avant les essais valides de CorvusRetro.

Captures : [portrait](screenshots/player.png), [paysage](screenshots/player-landscape.png),
[menu rapide](screenshots/quick-menu.png), [GBA portrait](screenshots/player-gba.png),
[GBA paysage](screenshots/player-gba-landscape.png). Références et géométrie dans
[les commandes](controls.md). Pas de qualification sur téléphone physique.

## Version 0.7 : marges système et retrait du thème sombre

Le service Android publie un snapshot immuable des dimensions de la fenêtre et
des marges des barres système/encoches. La surface NativeActivity utilise les
coordonnées de la fenêtre entière sur toutes les versions prises en charge.
Le rendu C en déduit le viewport, le clipping et la même origine tactile.
Un snapshot d'orientation périmée est rejeté. Aucune marge en dp n'est codée en dur.

CTest `native_viewport` couvre barres haut/bas, navigation gestuelle, encoche,
barre latérale en paysage, rotation, surface déjà réduite et valeurs transitoires
invalides. Les trois suites CTest passent avec et sans ASan/UBSan.
Le catalogue C ne contient plus que CorvusRetro, NVIDIA Shield et ASUS ; l'ancien
ID `1` revient à Corvus sans réassigner les IDs publiés `0`, `2`, `3`.
APK et lint sans erreur, signature et alignement 16 Ko vérifiés.
Revue indépendante : aucun problème bloquant relevé.

Sur Android 10 x86_64, 480 x 960, densité 200 :

- Surface complète 480 x 960, viewport portrait `0,30,480,900` : horloge et navigation entièrement séparées du menu.
- Paysage : surface 960 x 480, viewport `0,30,900,480`, barre de navigation à droite respectée ; retour en portrait correct.
- Chargement/déchargement tactiles en portrait et paysage, dont cœur GBA après rotation.
- Ancienne clé `dark` injectée dans les préférences de test : remplacée par `corvus`, démarrage avec le thème Corvus, données conservées.
- Les trois choix du catalogue sont sélectionnables, persistants après relance ; captures actualisées.
- Installation finale 0.7.0/code7 et démarrage réussis, aucun crash Android observé.


Sur Android 15 x86_64 (API35, APK ciblant35), même résolution/densité :

- Le menu natif réserve bien le viewport `0,30,480,900` dans la surface pleine fenêtre `480x960` ; titre et onglets hors des barres.
- Chargement puis déchargement d'un cœur par toucher, catalogue réduit à trois choix, sélection ASUS et retour au menu vérifiés.
- Paysage : surface `960x480`, viewport `0,30,900,480` ; chargement tactile GBC réussi avec la barre de navigation à droite.
- La zone de barre Corvus garde `#090D14`, vérifiée sur la capture de l'APK final.
- Le démarrage à froid sans KVM a provoqué un ANR de SystemUI de l'émulateur ; la boîte a été fermée avant les essais de CorvusRetro. Ce blocage de l'environnement n'est pas compté comme un essai applicatif réussi.

## Version 0.6 : thèmes

Tests CTest passés pour les quatre choix, le nouveau défaut Corvus, les valeurs
inconnues et les identifiants historiques. Build APK et lint sans erreur.
Revue indépendante : aucun finding bloquant sur les préférences et les rôles
visuels du thème clair.

Sur émulateur Android 10 x86_64 : préférence Shield 0.5 conservée après mise à
jour, Corvus et ASUS conservés après relance, lecteur et menu rapide ASUS lisibles,
retour aux anciens thèmes sombre et Shield avec persistance, puis absence de
préférence simulée sans supprimer contenus/sauvegardes : démarrage Corvus confirmé.
Captures des quatre choix et du nouveau catalogue réalisées ; aucun crash observé.

## Frontend natif 0.5

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

Émulateurs Android 10 et 15 exercés ; pas de téléphone physique ni d'exécution ARM64 validés.
La qualité audio à l'écoute, les performances mobiles et la compatibilité d'un
catalogue restent à mesurer ; GBA reste expérimental. Les menus OpenGL n'exposent
pas encore d'arbre d'accessibilité TalkBack. Les manettes physiques et les gestes
multitouch réels n'ont pas été exercés ; le modèle C teste leurs contrats d'entrée.
