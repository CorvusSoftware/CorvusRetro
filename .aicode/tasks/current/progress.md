État : READY_FOR_REVIEW. Implémentation achevée, branche feat/playback-and-core-management, PR #24 ; préparation 0.22.0/code28.
Référence RetroArch et décisions originales documentées dans docs/av-retroarch-reference.md.

Preuves : 17 suites natives strictes et 17 ASan/UBSan, 284 contrôles AAudio, 19 tests JVM dont 11 nouveaux CoreStore, frontend sans cœur, 8 tests de workflows, lint et builds Android debug/release passent. Base debug deux ABI, signature et alignement 16 Ko vérifiés. Revue indépendante terminée, deux findings corrigés et relus.
Android : playback portrait et paysage 41 contrôles chacun, suppression/réinstallation portrait 91 contrôles ; ROM, batterie et historique conservés. HOME ramène l'écran en portrait ; pas de revendication de retour HOME en paysage. Captures inspectées. Dernière précision de libellé affichant le nom du cœur à supprimer : tests menu/registre/suppression stricts et ASan, build debug/runner/lint passent ; nouvelle capture ciblée en cours sans changement de logique.
CI 41491545182 verte sur 5ccdb7b. Vérifier la CI du nouvel exact head après ce commit documentaire et de libellé avant de présenter la PR prête. Aucun merge/release pour ce lot.

Journaux éphémères hors dépôt /workspace/av-* et /workspace/setup/playback-removal-runtime. L'émulateur ne permet pas une écoute physique ; aucune qualification téléphone/Bluetooth/runtime ARM64 revendiquée.
