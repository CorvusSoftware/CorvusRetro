# Faits et décisions

- RetroArch applique les entrées activées du gestionnaireEMU(handler0), avec
  staging, application explicite et options après chargement/modification.
- Handler1 appartient à sa recherche mémoire ; refus explicite dans cette version.
- Les noms `.cht` conservent les échappements de guillemets/backslashes ; les
  codes multilignes sont exportés avec espaces. Import UTF-8, BOM facultatif.
- Extension CorvusAPI optionnelle par joueur, sans changer ABI0. Remplacement
  atomique ; les anciennes listes survivent à une erreur et aux restaurations.
- Mode normal RetroAchievements conservé. Aucun mode Hardcore simulé.
- Une sélection InputFilter doit être retournée intacte lors d'un collage refusé.
- Profils indépendants `<id>.cht`/`<id>.p2.cht`, y compris mêmeROM sur deuxjoueurs.
