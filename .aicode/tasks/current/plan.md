# Thèmes CorvusRetro et ASUS

Objectif : thème CorvusRetro par défaut inspiré du logo fourni (noir, bleu nuit, violet/lavande), ajout du thème ASUS corporate et conservation des thèmes existants.
Périmètre : catalogue de palettes C, menus/rendu C, adaptation des préférences Android et couleurs des barres, icône de lancement, validation et captures.
Compatibilité : conserver les IDs0Shield/1Sombre et les clés enregistrées shield/dark ; défautCorvus uniquement en absence de choix. Ne pas modifier les moteurs ou sauvegardes.
Étapes réalisées : référence logotypeASUSpublique consultée, quatrepalettes cataloguées, rôlesappliqués, tests/persistance/rendu/build/lint/revue validés. Publication autorisée surmain avec vérificationCI.
Preuves : tests du changement de thème et des anciens choix, buildAPK/lint, captures Android par thème et conservation après relance.
Contrainte : rester en C natif pour les thèmes et le rendu, Java se limite à stocker les choix et appliquer les couleurs de fenêtre fournies par C ; signatures et règles de ponctuation inchangées.
