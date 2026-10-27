État : VERIFY, code0.17.0/code20 soumis à CI pendant qualification Android finale. Amont841d675 synchronisé avant modification.
Capture inspectées : vidéo en haut portrait et plein centre paysage, contours gris transparents, fonctions visibles selon console. Pas de ROM/image commerciale ajoutée au projet.
Layout et rendu C : GB/GBCDPad+A/B+Start/Select, GBA+L/R ; vidéo native avec gouttières paysage ; menu portrait central et paysage latéral selon nouvelle capture ; hide réversible.
Menu rapide natif plein écran : Reprendre/Redémarrer/Fermer/États/Commandes/Succès/Informations/Menuprincipal. États save/load réels, historique conserve pause/onglet/focus/scroll au retour. Aucun faux shader/triche/enregistrement.
Preuves finales locales :9/9 CTest normal,9/9 ASan/UBSan ; corefree et Gradle Debug/Release/testAPK/lint PASS ; base sans moteurs/sixmodules16KB, archives4 et workflows6 PASS. Coordonnées tests via layout partagé, feedbackcontours, masquage, moteur réel et refus stockage. AndroidGB etGBCportrait40 contrôles chacun PASS ; GBAportrait et trois paysages encore en cours.
Revue indépendante finale sans finding P1/P2, y compris pointes duDPad/hit8directions ; plusaucuneéditionproduction prévue.
Prochain : terminer sixcas Android etRA, CI exacte et buildsigné ; seulement alors tag/publication0.17.0/code20.
