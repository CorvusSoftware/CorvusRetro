CorvusSoftware Quality Gate
Status: PASS
Risk: STANDARD

Criteria:
- PASS : CI main, build signé et publication réussis sur source exacte c97ccaf.
- PASS : version0.22.0/code28, certificat durable, provenance, deux ABI/16 Ko/zéro cœur/MP3 original.
- PASS : upgrade27->28 avec conservation UID/données/première installation/ASUS/historique.
- PASS : APK public retéléchargé identique à l'artefact signé qualifié.

Evidence:
- CI41493981097, signé41494442678, publication41495125139.
- SHA256 public9adf6208b05349671855f7b1e53dfb0873d01dcc0b7135d6c7ec9217f57aa4e1.
- Qualification des paquets et upgrade dans /workspace/setup/signed022-qualification.json, public022-qualification.json et signed022-runtime/upgrade.json.
- 17 suites natives+ASan/UBSan, 19 JVM et 173 contrôles Android réutilisés pour l'arbre fonctionnel identique.

Review:
- Comportement revu indépendamment dans PR #24, aucun finding restant. Documentation relue contre les reçus signés et les résultats distants.

Open risks:
- Émulateur API29 x86_64 ; aucune écoute physique, téléphone/Bluetooth/runtime ARM64 qualifiés.

Decision:
- Livraison publiée et vérifiée. Documentation à fusionner après sa CI exacte ; conserver le tag et les assets immuables.
