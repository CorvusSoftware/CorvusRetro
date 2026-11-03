Objectif : réduire les parasites persistants après 0.22.1 sur GBA, signalés sur Pokémon Émeraude, Redmi Note 13 5G et casque filaire.
Budget : DEEP. Base main fca4f45. Branche fix/audio-clock-and-save-io.

1. Isoler les défauts testables sans ROM propriétaire : dérive audio +100 ppm sur 600 s et pauses fsync dépassant la réserve PCM. Aucune cause spécifique au téléphone n'est confirmée.
2. Ajuster la cadence réelle par l'occupation audio lissée, avec correction bornée à +/-0,5 %, PCM exact et au plus trois trames émulées par rendu. Préserver entrées et RetroAchievements pour chaque trame.
3. Déporter les sauvegardes périodiques sur un travailleur : copie et acquittement sur le propriétaire, un instantané borné, writer atomique existant, barrières pour les transitions critiques.
4. Vérifier dérives, coûts CPU, sauvegardes bloquées/échouées, générations et cycle de vie en strict et ASan/UBSan. Compiler Android, exercer GBA pendant le jeu et les transitions système, obtenir une revue indépendante et la CI du commit exact.

Préserver PCM, volume, VSync, cycles par trame, formats/identités des sauvegardes, moteurs, ABI et pins. Le mainteneur autorise maintenant la fusion de PR #28 et la publication corrective 0.22.2/code30. Contrôler la CI exacte, le signer durable, les octets publiés et la mise à jour signée avant livraison. Une écoute sur le téléphone reste une validation distincte.
