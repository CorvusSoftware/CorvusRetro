# CorvusSoftware Quality Gate

Status: PASS
Risk: DEEP

Criteria:
- [PASS] Sources fusionnées et CI exactes des versions et pins API/Core.
- [PASS] Six archives publiques/provenance/SHA/ABI/16Ko/exports vérifiés ; runtime
  GBA triche Android x64 en portrait et paysage avant promotion du catalogue.
- [PASS] Catalogue public identique au candidat et ancien flux inchangé.
- [PASS] APK sans cœur, certificat durable, code33, deux ABI16Ko, son et provenance.
- [PASS] Upgrade32->33 conservant UID/données/historique/ASUS ; images inspectées.
- [PASS] Octets APK publics identiques au fichier signé testé, workflow actif.

Evidence:
- CI release41932276773, main41932715340, signé41933214823, publié42017261496 : succès.
- Core main41931403008, distribution41932376500, catalogue41933537606 : succès.
- Tests natifs24/24, sanitizers et JVM/lint dans les CI ; GBA public4099+4512 checks.
- Téléchargement public APK, signature et empreinte identique vérifiés.

Review:
- Revue de la fonctionnalité précédente acquise ; versions, pins, fixtures et
  provenance relus. Le défaut visuel de navigation observé existe en signé0.23.1 ;
  pied À propos identique pixel pour pixel en0.24.0, aucun correctif annoncé.

Open risks:
- Défaut visuel préexistant sur émulateur logiciel, pas de validation sur téléphone.
- ARM64 compilé/contrôlé, pas exécuté physiquement ; anciens états liés à leur core.
- Publication manuelle dupliquée42017261434 refusée car le push avait déjà publié.
  Le run effectif est vert ; aucun asset remplacé. Éviter les deux déclenchements.

Decision:
- Livraison autorisée terminée, preuves et limites précises dans docs/validation-android.md.
