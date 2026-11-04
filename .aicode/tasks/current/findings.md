# Découpage natif

Architecture et diff relus indépendamment sans finding concret. Listes, réglages, actions et orchestration sont séparés, avec données de lignes indépendantes du renderer et contrats privés.

La comparaison ponctuelle de 93 fonctions, de la lecture initiale, des enums et des globals confirme la préservation. La compilation a capturé deux omissions mécaniques corrigées : compteur déplacé avec les transferts et nom du callback plateforme préservé.

21/21 strictes et 21/21 ASan/UBSan, core-free, build/lint/APK et parcours Android portrait/paysage passent. Captures inspectées, aucun crash enregistré. La dernière barrière avant PR prête reste la CI de la tête exacte.
