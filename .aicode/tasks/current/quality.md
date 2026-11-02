CorvusSoftware Quality Gate
Status: PASS (implémentation locale pour revue)
Risk: STANDARD
Criteria:
- PASS : commandes portrait plus espacées, tailles inchangées, géométrie commune au dessin et au toucher.
- PASS : petites surfaces/configuration100..130% et positions±15 bornées, cibles téléphones48dp et absence chevauchement.
- PASS : paysage et menus conservés.
Evidence:
-15natives et15ASan/UBSan réussis.
- Androiddebug2ABI/lint/testAPK ; APKzeroCore/ELF16Ko réussis.
-177contrôles Android overlay portrait répartis surGB/GBC/GBA ; capturesGB/GBA inspectées. La première captureGBC est transitoirement noire, ses interactions et menus passent ; géométrie identiqueGB/GBC dans le code.
Review:
- Indépendante sans défaut,127260configurations avec comparaison champ par champ au paysage précédent.
Open risks:
- Aucun téléphone physique testé. Pas de nouvelle release signée dans cette PR.
Decision:
- PR prête pour revue ; consulter CI du dernier commit avant fusion. Ni fusion ni release dans cette tâche.
