État : DONE, version0.15.1/code17 publiée et vérifiée.
Demande : animation RetroArch, cartes en haut au centre, logo qui descend avant le titre, lancement et déblocage identiques.
Amont c1d25d3 propre et synchronisé avant modification. Livrable0.15.0 conservé.
Référence inspectée : RetroArch dda31fef21aeb472be9de7ee53eb386cc8db1417, gfx_widget_achievement_popup.c.
Chronologie originale : descente330ms, ouverture330ms, maintien4s, fermeture330ms, remontée330ms ; ease-out quadratique. Badge seul à l'entrée et à la sortie, texte révélé par clip, total centré pendant le déploiement.
Premier update ancré pour préserver l'entrée après chargement lent ; métadonnées tardives sans retour en arrière. Corridor paysage symétrique, respect des commandes et restauration du clip.
Preuves locales :9/9 CTest normal et9/9 ASan/UBSan,5176 contrôles notifications ; Gradle Debug/Release/testAPK/lint ; base sans moteurs et six modules/16KB ; tests Python archives4/4 et workflows6/6 ; politique texte et diff propres.
Revue indépendante : aucun finding P1/P2. Captures de test Android séparées, aucune image ou ROM commerciale embarquée.
Android15 x86_64 :18 contrôles portrait et18 vrai paysage PASS ; viewport paysage960x480 confirmé, captures badge seul puis titre ouvert et zone sûre inspectées.
Source fa78d5e17120207f6fca554f00565c6e1750cf97 publiée sur main et tag immuablev0.15.1.
CI40467148520 SUCCESS ; build signé40467458744 SUCCESS ; publication40467718550 SUCCESS.
APK public CorvusRetro-0.15.1-base-release.apk, SHA256 bce272807733278d389bc08cf98397aa996483b7e7369761a2803b19a795490a ; signature durable vérifiée avec pin indépendant, version17, base sans moteurs et noticeMIT conservée.
Six ZIPs publics vérifiés : ABI, export mémoire des succès, segments16KB, taille/hash ; octets identiques aux actifs0.15.0, aucun changement de catalogue fournisseur nécessaire. Aucun JSON réintroduit dans le frontend.
Android10 x86_64 : mise à jour de l'APK signé0.15.0 vers0.15.1 acceptée sans réinstallation ; thèmeASUS et ligne historique corvus-preserved.gba conservés visuellement ; aucun crash applicatif journalisé. Pas de mesure de fluidité sur cet ancien émulateur.
README pointe sur l'actif public vérifié. Reçu local : /workspace/setup/centered-popup151-qualification.json.
Limites : compte et images synthétiques, aucun compte personnel, téléphone physique ou runtimeARM64 revendiqué.
