CorvusSoftware Quality Gate
Status: PASS (validation locale)
Risk: STANDARD

-21 suites natives et 21 ASan/UBSan passent.
-Ordre, sélection et cibles du menu rapide vérifiés pour un/deux joueurs et deux orientations ; bouton supprimé sans cible cachée.
-Build/lint et APK sans cœur, deux ABI et16Ko passent. Atlas196 inspecté dans les trois palettes.
-Android API29 x86_64 : parcours ciblés portrait/paysage réussis, captures inspectées, aucun crash enregistré.
-PNG du lanceur identique aux octets de l'avatar officiel ; SVG du petit modèle lisibles.
-Revue indépendante sans finding bloquant.

APKdebug SHA256 :edee3f440048ca6c7e635fbcdb22203c8311673d263395d075542a4aae570338
CI finale : consulter les checks du head publié avant sortie du brouillon.
Limites : pas de nouvelle qualification physique/ARM64/audio/réseau public. Le runner historique RetroAchievementsInstrumentation, déjà fondé sur d'anciens parcours de menus, n'est pas revendiqué validé.
