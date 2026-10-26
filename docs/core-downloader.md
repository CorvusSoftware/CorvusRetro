# Découverte des cœurs

CorvusRetro recherche automatiquement les fichiers JSON du dossier
[`catalog/`](https://github.com/CorvusSoftware/CorvusRepository/tree/main/catalog)
dans le dépôt [CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository).

## Fonctionnement

1. Le service Android consulte l'[API GitHub du dossier catalog](https://api.github.com/repos/CorvusSoftware/CorvusRepository/contents/catalog?ref=main).
2. Il sélectionne les fichiers `*.json` à nom valide, vérifie leur chemin,
   leur URL publique et leur taille, puis les télécharge via HTTPS.
3. Chaque fichier doit respecter le format 3 de CorvusAPI 0. Les cœurs sont
   filtrés par architecture et fusionnés dans une liste unique.
4. Le téléchargement des ZIP impose une release GitHub CorvusSoftware
   approuvée, le SHA-256 exact, la taille annoncée et la vérification ELF
   avant installation dans le stockage privé.

**Aucun index manuel n'est nécessaire.** Quand un nouveau fichier
`catalog/fournisseur.json` est accepté dans CorvusRepository, CorvusRetro
le découvre au prochain rafraîchissement du téléchargeur.

## Fournisseurs et sécurité

- `catalog/corvuscore.json` contient les cœurs officiels CorvusGB,
  CorvusGBC et CorvusGBA.
- Les autres catalogues utilisent un identifiant par fournisseur
  (`fournisseur.identifiant`) et une bibliothèque native `lib...so`.
- Un catalogue ne peut pas emprunter les identifiants d'un autre fournisseur.
- Seuls les ZIP des releases **CorvusRetro** et **CorvusRepository** dans
  l'organisation CorvusSoftware sont autorisés.
- Le client accepte au maximum **16 catalogues**, **32 cœurs compatibles**,
  **96 Kio par catalogue** et **32 Mio par ZIP**.
- Si l'API GitHub n'est pas accessible, il retente directement
  `catalog/corvuscore.json`. Les modules déjà installés restent disponibles.

**Attention :** un cœur est du code natif exécutable. Une empreinte SHA-256
confirme l'identité des fichiers, pas leur innocuité. Tout nouveau catalogue
et toute nouvelle bibliothèque doivent faire l'objet d'une revue avant fusion
dans le dépôt officiel.

## Publication

CorvusRetro produit les archives qualifiées et un catalogue temporaire
dans ses artefacts GitHub Actions. Après publication et vérification des
archives, les métadonnées peuvent être proposées dans CorvusRepository,
dont la CI vérifie l'ensemble des JSON de `catalog/`.

Les anciens flux de CorvusRetro ont été retirés à la demande du
mainteneur ; les APK v0.13.0 et antérieurs qui utilisaient leurs anciennes
adresses ne peuvent plus rafraîchir leurs catalogues. Une nouvelle
release de l'application est nécessaire.

[Documentation des modules](individual-cores.md) ·
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository)
