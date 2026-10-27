État : VERIFY, implémentation0.15.1/code17 prête pour CI.
Demande : animation RetroArch, cartes en haut au centre, logo qui descend avant le titre, lancement et déblocage identiques.
Amont c1d25d3 propre et synchronisé avant modification. Livrable0.15.0 conservé.
Référence inspectée : RetroArch dda31fef21aeb472be9de7ee53eb386cc8db1417, gfx_widget_achievement_popup.c.
Chronologie originale : descente330ms, ouverture330ms, maintien4s, fermeture330ms, remontée330ms ; ease-out quadratique. Badge seul à l'entrée et à la sortie, texte révélé par clip, total centré pendant le déploiement.
Premier update ancré pour préserver l'entrée après chargement lent ; métadonnées tardives sans retour en arrière. Corridor paysage symétrique, respect des commandes et restauration du clip.
Preuves locales :9/9 CTest normal et9/9 ASan/UBSan,5176 contrôles notifications ; Gradle Debug/Release/testAPK/lint ; base sans moteurs et six modules/16KB ; tests Python archives4/4 et workflows6/6 ; politique texte et diff propres.
Revue indépendante : aucun finding P1/P2. Captures de test Android séparées, aucune image ou ROM commerciale embarquée.
Android15 x86_64 :18 contrôles portrait et18 vrai paysage PASS ; viewport paysage960x480 confirmé, captures badge seul puis titre ouvert et zone sûre inspectées.
À terminer : publication main/CI exacte, build signé durable, tag immuablev0.15.1, vérification des actifs publics, lien README et reçu final.
