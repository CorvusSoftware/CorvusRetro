Objectif : réaliser la maquette native CorvusRetro pour accueil, bibliothèque/recherche, cœurs/détails, sauvegardes/actions, réglages et menu rapide. Écran de jeu et commandes conservés. Budget DEEP.
Base main69952eb ; branche feature/mobile-interface-redesign. Les fonctions existantes, identités, sauvegardes, thèmes et protections restent accessibles. Pas de démo ni de jeux factices. Aucun changement des moteurs/ABI/pins.
1. Navigation C11 cinq onglets et géométrie commune dessin/toucher/focus/défilement.
2. Présentation native : cartes/arrondis/dégradés, header original, consoles, états vides et adaptations portrait/paysage/thèmes.
3. Métadonnées/images/services Android bornés et détails réels des cœurs ; recherche native avec IME.
4. Magasin d'états nommés réel, previews et actions sûres, anciens chemins conservés ; batteries .sav/.srm et link toujours accessibles.
5. Intégration des parcours puis tests hôtes stricts/ASan, Java, APK/lint et Android visuel/fonctionnel. Review indépendante puis PR/CI exacte. Pas de fusion/release implicite.
Risques : anciens tests figent trois onglets/76dp ; données absentes de la maquette ne doivent pas devenir boutons sans effet ; stockage d'états et images restent bornés et hors rendu.
