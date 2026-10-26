Status: PARTIAL avant qualification finale et publication.
Risk: DEEP.

Vérifié : vrai dlopen privé sur Android15 target35, 35 contrôles catalogue/stockage/service,
six CTest normaux et ASan, zéro cœur,19testsPython, compilation APK/tests/lint.
Review indépendante runtime et workflows : un P1 descriptor corrigé avec régression
remplacement pendant un jeu; aucun finding P1/P2 restant.
Aucun changement de cible35, SDK, sous-module ou signature durable.
À compléter : preuve de liste immédiate et recréation pendant téléchargement,
jeux GB/GBA depuis files/cores, CI exacte, APK signé et vrais ZIP publics/catalogue.
Limites : aucune qualification téléphone physique ou exécution ARM64; transport
de tests Android déterministe, TLS public de production laissé strict.
