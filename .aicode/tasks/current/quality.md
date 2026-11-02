CorvusSoftware Quality Gate
Status: PASS
Risk: STANDARD
Criteria:
- PASS : PR21 approuvée par utilisateur, CI exacte verte puis fusion main.
- PASS : APK0.21/code27 signé avec identité habituelle et provenance exacte.
- PASS : deuxABI/alignement16Ko/zeroCore/MP3original et absence provider de test.
- PASS : upgrade26->27 et conservation UID/données/ASUS/historique vérifiés.
- PASS : publicationv0.21.0 et retéléchargement public identique aux octets qualifiés.
Evidence:
- CI41443902775, signé41444288324 et publication41445211694 réussis sursourcee6f1157.
- QualificationAPK et upgrade dans /workspace/setup/signed021-qualification.json et signed021-runtime/upgrade.json.
- PublicAPK SHA256890a3912a4e77959e9892ce4a9732dc3d9179bb26c5d112dab376619fc94f0b1.
Review:
- Changementsfonctionnels revus indépendamment pendantPR21 ; aucunfinding restant.
Open risks:
- Émulateurx86_64, pas detéléphone/manettephysique/runtimeARM64 testé. Connectivité du catalogue et moteurs distribués séparément, inchangés.
Decision:
- Livraison publiée. Documentation dequalification àfusionner après saCI. Conserver tag et assets immuables.
