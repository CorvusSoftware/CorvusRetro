# Validation de livraison

- P0 : CI exacte des révisions fusionnées et pins API/Core cohérents.
- P0 : six archives natives, ZIP unique, ELF/ABI/16Ko, export triche obligatoire,
  provenance/source et octets publics SHA256 identiques au lot qualifié.
- P0 : exécution des archives publiques x86_64 dans le frontend, triches,
  chargement et fermeture, IME/SAF en portrait et paysage.
- P0 : catalogue v1 vérifié avant promotion, flux historique0.18.0 inchangé.
- P0 : APK durablement signé, version0.24.0/code33, zéro cœur, deux ABI16Ko,
  provenance, son original et À propos ; mise à jour32->33 conservant les données.
- P0 : publication des octets déjà qualifiés, nouveau tag et assets immuables,
  téléchargement public identique ; workflow de publication réactivé.
- P1 : suite native, sanitizers, JVM/lint et CI Android de la préparation.
- Limites : émulateurAPI29x86_64, pas de nouveau test sur téléphone physiqueARM64.
