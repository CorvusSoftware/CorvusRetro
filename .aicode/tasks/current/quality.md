CorvusSoftware Quality Gate
Status: PASS (correction locale de PR)
Risk: STANDARD

Criteria:
- PASS : dimensions et disposition0.20.0 restaurées, autoDPI/menu latéral supprimés.
- PASS : nouvelles icônes, ordre et retrait du libellé expérimental GBA conservés.
- PASS : source de dessin/touch/scroll comparée à55e6181 ; différences du rendu limitées ààpropos/version/GBA.
Evidence:
- Native15/15 et ASan/UBSan15/15 réussis.
- Androiddebug2ABI, lint, testsJVM et compilation instrumentation réussis.
- APKzeroCore et ELF16Ko réussis.
- Android SettingsInstrumentation323portrait+323paysage réussis ; captures des réglages inspectées.
Review:
- Revue indépendante sans défaut, adaptations de tests cohérentes.
Open risks:
- Aucun téléphone physique testé. Pas de nouvelle release signée dans cette correction de PR.
Decision:
- PR prête pour revue avec dimensions d'origine. Consulter les contrôles CI de son dernier commit avant fusion ; aucune fusion/release effectuée.
