Objectif STANDARD : jeu d'icônes original et cohérent pour la nouvelle interface.
1. Redessiner les32 glyphes existants et ajouter10 actions dédiées : grille, recherche, renommer, dupliquer, partager, corbeille, plus, validation, sortie et ajout.
2. Garder les anciens IDs, adapter bloc d'atlas à464..511 sans modifier1024x2048 et borner le cache de glyphes avant les icônes. Mettre à jour C/Java et les usages sémantiques.
3. Vérifier ressources réelles Android :42 formes distinctes, bornées et lisibles à24px, planche et trois thèmes ; tests natifs/build/lint puis parcours/captures et revue ciblée.
4. Publier dans PR30, CI exacte avant PR prête ; aucune fusion/release.
