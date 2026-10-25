# Quality gate

PASS pour l'implémentation et la validation locale.
Moteurs JNI, chargement vide, déchargement/rechargement, APK propre, lint, signature et alignements 16 Ko validés.
Scénarios Android du cycle de vie, des thèmes, des états et de la batterie validés ; preuves dans docs/validation-android.md.
Revue indépendante traitée, aucun finding bloquant. Politique de ponctuation et intégrité du sous-module vérifiées.
Le workflow Android APK publie l'artefact pour chaque push sur main ; consulter son résultat distant pour la révision publiée.
Limites : tests sur émulateur x86_64, sans validation sur téléphone physique, sans mesure audio à l'écoute ou de compatibilité d'un catalogue. GBA expérimental.
