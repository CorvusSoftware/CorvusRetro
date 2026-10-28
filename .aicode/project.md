# CorvusRetro

Frontend Android original C11 : NativeActivity, EGL/OpenGL ES2, AAudio. Java fournit uniquement les services OS.
Version publiée 0.17.0/code20 ; cible35/min26 ; NDK27.2 ; JDK17+ ; Gradle8.13/AGP8.9.1.
Sources : menu/, cores/, gfx/drivers/gl2, gfx/widgets/notifications, audio/drivers/aaudio, input/, frontend/drivers/, tasks/, cheevos/.
Android sous pkg/android/, wrapper Gradle racine délégué ; dépendances sous deps/.
Base sans moteur ; six modules CorvusAPI distincts, exclus de l'APK, distribués en archives individuelles vérifiées.
Les catalogues JSON ont été volontairement retirés du frontend : découverte de CorvusRepository/catalog/*.json, validation par fournisseur et architecture. Ne pas réintroduire les anciens endpoints.
CorvusCore0.14.1 pin2d7bdd2020894fa27d153cbecd421da47bcb37f4, CorvusAPI bfdbed337ed7bc7334ed2d8e1f510ab503fa475d, header public unique corvusapi.h, extension mémoire facultative, ABI0 inchangée.
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
Publication autorisée sur main et APK signé durable. Les nouveaux catalogues sont des artefacts temporaires pour revue dans CorvusRepository ; aucun JSON de catalogue à maintenir ici.

Cycle contenu : frontend exige cœur sélectionné compatible avant pick/import/historique. cr_engine_unload_game sauvegarde puis ferme seulement le contenu en gardant handle/module/output/API/system ; cr_engine_close reste unload complet. Refus batterie sans mutation, RA/notifications nettoyés seulement après fermeture réussie.
UI : Listes de lecture=Historique seul ; navbar maison/engrenage, switches natifs arrondis. Logo launcher adaptatif blanc/fond noir sous res/drawable-nodpi/corvus_logo.png.
Instrumentation navigation : attendre focus fenêtre et état active=1 après rotation ; lire préférences via activity.preference (thème String/audio Boolean). Vérifier une vraie frame vidéo avant d'analyser le popup.

Interface0.17 : input/play_layout et input/overlay calculent et dessinent les profils GB/GBC8 contrôles et GBA10. Traits gris, feedback clair, hide réversible ; vidéo native au-dessus en portrait et entre les gouttières en paysage. Menu portrait centré, paysage au-dessus de la croix gauche. Menu rapide plein écran, CR_SAVESTATES pour save/load réels ; retour restaure pause/focus/scroll/onglet.
Qualification UI : attendre la confirmation asynchrone des préférences et le focus + état natif actif après HOME. Libérer la réponse image dès capture du bitmap immuable, avant encodagePNG ; le lancement dispose de3s d'attente image puis de sa descente. Une capture de1.8s ne couvre pas ce budget.
Livraison0.17 : tagv0.17.0 surcb30229508ae5ee91181672fdb9054ad1de32662, CI40647479489, buildsigné40647835058, publication40649001619 SUCCESS. APK public identique au candidat qualifié, signature durable ; six archives0.14.1 identiques à0.16.0, donc catalogues existants conservés.
