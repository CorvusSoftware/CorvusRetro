# CorvusRetro

Frontend Android original C11 : NativeActivity, EGL/OpenGL ES2, AAudio. Java fournit uniquement les services OS.
Version publiée 0.18.0/code22 ; cible35/min26 ; NDK27.2 ; JDK17+ ; Gradle8.13/AGP8.9.1.
Sources : menu/, cores/, gfx/drivers/gl2, gfx/widgets/notifications, audio/drivers/aaudio, input/, frontend/drivers/, tasks/, cheevos/.
Android sous pkg/android/, wrapper Gradle racine délégué ; dépendances sous deps/.
Base sans moteur ; six modules CorvusAPI distincts, exclus de l'APK, distribués en archives individuelles vérifiées.
Les catalogues JSON ont été volontairement retirés du frontend : découverte de CorvusRepository/catalog/*.json, validation par fournisseur et architecture. Ne pas réintroduire les anciens endpoints.
CorvusCore0.15.0 pin063cf1f674788aa5913cebe184457a10b5946edd, CorvusAPI39c927c5a1a55b78ab55758a41cd16b0971c4684, header public corvusapi.h, extensions mémoire/options/sous-systèmes facultatives, ABI0 inchangée.
rcheevos MIT pin1433173220a7eaede6a9ed7a18e94117be1821e0 ; frontend original AGPL-3.0-only.
Succès en mode normal ; hash à l'ouverture, évaluation après chaque frame émulée ; token et file d'envois AES-GCM Keystore, aucun mot de passe persisté.
Notifications C : file8 ; cartes en haut au centre, descente/ouverture/repli/remontée330ms chacune, maintien4s, total5.32s ; progression mesurée2s ; callbacks copiés, clear à la fermeture et HIDE/reset.
Carte de lancement uniquement après identification du jeu ; texture préparée avant descente, attente3s maximum, fallback figé. Aucun corbeau provisoire remplacé ensuite par une image.
Images officielles media.retroachievements.org : PNG borné512KiB/1024px, tuile96px, deux workers/queue16, mémoire32/disque64, texturesGL16. Aucun IO/décodage dans le rendu.
Le thread frontend possède seul le cœur, rc_client, les notifications et les textures. Les callbacks image réveillent seulement le looper.
Préserver les identités historiques SHA-0/-1/-2, sauvegardes et acquittements de batterie ; un échec de sauvegarde conserve le cœur.
Thèmes stables :0Shield/2Corvus/3ASUS, défautCorvus. Les notifications respectent le viewport sûr et les commandes, avec choix du meilleur espace lisible en paysage court.
Validation : make test et SANITIZE=ON ; Gradle Debug/Release/lint ; runner CoreRepositoryInstrumentation par défaut, RetroAchievementsInstrumentation pour compte/popups, RetroAchievementsImagesInstrumentation pour images.
Aucun compte réel, téléphone physique ou runtimeARM64 revendiqué par les tests synthétiques Android.
Publication autorisée sur main et APK signé durable. Depuis la séparation de distribution, les workflows frontend ne construisent que la base APK ; CorvusRepository construit ou importe les modules, publie ses propres releases et propose les catalogues relus. Aucun JSON de catalogue à maintenir ici.

Cycle contenu : frontend exige cœur sélectionné compatible avant pick/import/historique. cr_engine_unload_game sauvegarde puis ferme seulement le contenu en gardant handle/module/output/API/system ; cr_engine_close reste unload complet. Refus batterie sans mutation, RA/notifications nettoyés seulement après fermeture réussie.
UI : Listes de lecture=Historique seul ; navbar maison/engrenage, switches natifs arrondis. Logo launcher adaptatif blanc/fond noir sous res/drawable-nodpi/corvus_logo.png.
Instrumentation navigation : attendre focus fenêtre et état active=1 après rotation ; lire préférences via activity.preference (thème String/audio Boolean). Vérifier une vraie frame vidéo avant d'analyser le popup.

Interface0.17 : input/play_layout et input/overlay calculent et dessinent les profils GB/GBC8 contrôles et GBA10. Traits gris, feedback clair, hide réversible ; vidéo native au-dessus en portrait et entre les gouttières en paysage. Menu portrait centré, paysage au-dessus de la croix gauche. Menu rapide plein écran, CR_SAVESTATES pour save/load réels ; retour restaure pause/focus/scroll/onglet.
Qualification UI : attendre la confirmation asynchrone des préférences et le focus + état natif actif après HOME. Libérer la réponse image dès capture du bitmap immuable, avant encodagePNG ; le lancement dispose de3s d'attente image puis de sa descente. Une capture de1.8s ne couvre pas ce budget.
Menus0.18 : sous-systèmes19 dès engine.handle, options20 au rang4 du menu rapide, valeurs21, configuration2jeux22, sélection23. Valeurs persistées par core/key/valueID ; palettes et audio immédiats, matériel GBA au prochain chargement/reset. Référence RetroArch dda31fef/menu/menu_displaylist.c, code original et frontière CorvusAPI publique.
Session corvus.link.two_player : deux ROMs, écrans côte à côte, commandes séparées ; P1 conserve son .sav, P2 utilise -link-p2.sav même avec la même ROM. États et mémoire des succès indisponibles pour le link officiel ; compte et envois conservés. Le menu rapide choisit le joueur tactile/clavier ; deux premières manettes à ports distincts.
Persistance : copier les backups options dans .bak.new et fsync avant rename en .bak, jamais de hard link refusé par SELinux. Une copie partielle ne doit jamais être promue. Garder CR_CORE_VERSION0.14.1 comme suffixe historique du format solo ; les états groupés de fournisseurs compatibles ont une identité distincte par paire et sous-système.
Mise à jour de module : garder l'ancien handle jusqu'à sélection explicite ; le fastpath idle compare aussi le chemin ouvert. Les informations du cœur utilisent le module ouvert, pas le remplacement dans le registre.
Livraison0.18.0 : tagv0.18.0 sur67385be702dbf146f18b4e529b8559c9e38d0b32, CI40740763674, buildsigné40741288414, publication40742906577 SUCCESS. APK et six ZIP0.15 publics comparés aux octets qualifiés, signature durable et 16Ko vérifiés.
Qualification :12/12 natifs +12/12 ASan/UBSan, corefree, Gradle Debug/Release/lint/testAPK ; API29x86_64 six parcours configuration GB/GBC/GBA portrait/paysage,782 contrôles. Mise à jour signée0.17.1->0.18.0 acceptée ; ASUS et historique conservés sur profil sans cœur. Fermer uniquement les alertes système identifiées par capture ; transport ADB couvrant toute la durée du runner. Aucun téléphone physique ou runtime ARM64 revendiqué.

Distribution indépendante : CorvusRepository/providers déclare les fournisseurs approuvés ; catalog/<fournisseur>.json utilise fournisseur.identifiant, sauf les ID officiels historiques. Le lot central v0.15.1 contient les moteurs 0.15.0 depuis Core 063cf1f et API 39c927c ; le tag du lot est indépendant des versions moteur et APK. Le client 0.18 accepte déjà les releases centrales. GITHUB_TOKEN sépare qualification en lecture seule et publication ; ne pas transférer de clé APK au distributeur.
