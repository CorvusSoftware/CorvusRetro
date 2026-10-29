État : IMPLEMENT. Distribution indépendante des cœurs dans CorvusRepository.

La tâche précédente options/sous-système est livrée : APK0.18.0/code22 public, Core0.15.0/API39c, cataloguemain43218. Les preuves précédentes restent dans l'historique5ba0898 et findings.md.

Branches locales des deux dépôts : feat/independent-core-distribution. Aucun nouveau commit publié pour cette tâche à cette étape. Application inchangée0.18.0, aucun nouveau binaire APK nécessaire : son client accepte déjà les archives des releases CorvusRepository.

Préparé : registry providers/corvuscore.json, buildNDK direct Core qualifié, import de fournisseurs approuvés, publication immuable puis branche/PR de catalogue et CI explicite. Frontend workflows APK-only, Gradle target unique corvusretro et anciens outils archives supprimés. Documentation contribution et namespaces conservant ABI0.

Preuves locales : six modules autonomes réellement construits depuis063cf1f, ELF16Ko/4exports/dépendances système vérifiés. Ces ZIP diffèrent des anciens octets, donc migration0.15 doit MIRRER les6ZIP CorvusRetro0.18 byteexact sans remplacement. Gradle debug/release/lint/testAPK PASS, workflowtests8/8, APKfrontend exact2SO et16Ko PASS. Outils Repository encore en développement/revue.

Suite : terminer tests et revue indépendante du publisher/import ; commit/PR et CI exacte ; fusionner outils qualifiés ; lancer dry-run officiel ; miroir public byteexact v0.15.0 dans CorvusRepository, re-télécharger puis PRcatalogue ; fusionfrontendCIverte ; docs preuves et qualitygate.
