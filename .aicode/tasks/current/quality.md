CorvusSoftware Quality Gate
Status: PASS
Risk: STANDARD (distribution d'un correctif déjà revu)

Criteria:
- PASS : autorisation de fusion/publication, CI exacte PR et main réussie.
- PASS : APK signé durable, provenance, version 0.22.2/code 30, deux ABI/16 Ko/zéro cœur/MP3 original/À propos.
- PASS : upgrade 29->30 conservant UID/dossier/première installation/ASUS/historique.
- PASS : publication réussie et APK public identique aux octets qualifiés.

Evidence:
- CI 41597282038, signé 41597582130, publication 41598126420 sur f83ecd694f5cf373873a011febc910a9a658a2ad.
- SHA-256 public : 2d9cf2198f33263e0b9c132bf13d35f9f92207363b0b721d6311164f8b3cec88 ; reçus signés, mise à jour et comparaison publique dans /workspace/setup/.

Review:
- Changements fonctionnels de PR #28 revus indépendamment ; aucun finding restant.
- Documentation relue contre les reçus ; contrôle de la CI documentaire avant fusion.

Open risks:
- Qualification runtime sur émulateur API29 x86_64 ; aucune écoute physique/ROM Pokémon/Bluetooth/runtime ARM64.
- Le PASS atteste la livraison. La disparition perçue des parasites du Redmi reste non vérifiée.

Decision:
- Version publiée et distribution vérifiée. Conserver tags/assets immuables et fusionner la documentation après sa CI exacte.
