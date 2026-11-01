CorvusSoftware Quality Gate
Status: PASS
Risk: DEEP

Criteria:
- PASS : Sous-système réel du cœur, visible au chargement et retiré au déchargement.
- PASS : imports/exports batteries .sav/.srm, P1/P2 distincts, annulation et réponses périmées.
- PASS : candidat avant remplacement, backup live, rollback et récupération groupée après arrêt.
- PASS : plan des réglages documenté ; aucune préférence factice ; types et clés conservés.
- PASS : compilation, lint, revue, CI exact commit, signature durable et mise à jour.
- PASS : release publique v0.19.0, octets identiques à l'APK qualifié.

Evidence:
- 13 natifs + 13 ASan/UBSan/leaks + 2 sans cœur, tous verts ; complément link P1 normal/ASan.
- 8 JVM ; builds Android deux ABIs et lint ; zeroCore/ELF/alignment16Ko/manifeste sans fournisseur test.
- 38 contrôles documents et 546 parcours natifs Android API29 x86_64, 40 contrôles catalogue avec capture3cœurs.
- CI41290381247/41290648163, signé41290806524, publié41291144366 réussis.
- Upgrade23vers25 : mêmes UID/dossier/date initiale, ASUS et historique conservés, aucun nouveau crash applicatif.
- SHA256 public24d07ad24098a0f9d4ac697a93d9f52be57562ccc032d784d0f784f8f4bef063 comparé exactement au candidat qualifié.

Review:
- Revue indépendante sans finding restant ; journal partiel, chemin non initialisé,
  backup live et troncature export corrigés et retestés.

Open risks:
- Aucun point bloquant dans le périmètre livré.
- Limites : pas d'appareil physique/exécution ARM64 ; sélection DocumentsUI simulée,
  vrai ContentResolver/JNI/cœur ; tests fsync par arrêt processus, pas panne matérielle.
- Connectivité Internet émulateur : catalogue signé indisponible, sonde HTTPS Android
  ConnectException ; endpoints200 depuisworker et JSON identique aux fixtures validées.
  Téléchargement réseau sur téléphone non qualifié, ne pas le déclarer testé.
- Refondre les préférences selon le plan reste un lot futur, explicitement demandé comme plan.

Decision:
- PASS : fonctions demandées et livraison couvertes par des preuves adaptées. DONE.
