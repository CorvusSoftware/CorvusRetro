# Téléchargeur de cœurs CorvusRetro

Le téléchargeur récupère un catalogue natif (format 3) depuis
[CorvusRepository](https://github.com/CorvusSoftware/CorvusRepository), puis
télécharge un ZIP pour le cœur et l'architecture choisis.

**Catalogue actuel :**
`https://raw.githubusercontent.com/CorvusSoftware/CorvusRepository/main/cores-v3.json`

Les six entrées initiales sont **identiques** à celles du catalogue historique
de CorvusRetro, et leurs archives ZIP restent sur les releases officielles de
CorvusRetro. Les tailles et empreintes SHA-256 correspondent aux assets publiés.
Le client contrôle toujours les origines autorisées, les tailles et le SHA-256,
puis installe un module natif dans le stockage privé sans PackageInstaller.

## Compatibilité des anciens clients

| Client | Catalogue | Distribution |
| --- | --- | --- |
| Nouvelles versions | CorvusRepository / `cores-v3.json` | ZIP natif par cœur et ABI, CorvusAPI 0 |
| 0.12-0.13 | CorvusRetro / `core-repository/cores-v3.json` | ZIP natif par cœur et ABI |
| 0.11 | CorvusRetro / `core-repository/cores-v2.json` | Split APK signé lié à la base |
| 0.10 | CorvusRetro / `core-repository/index.json` | Ancien pack APK complet |

**Ne jamais supprimer ou rediriger les anciens catalogues.** Une installation
existante continue d'utiliser l'URL qui était intégrée à son APK. Les archives
publiées restent immuables, et les identifiants des jeux et des sauvegardes ne
changent pas avec le déplacement des métadonnées.

Le code actuel n'accepte encore que les archives de releases CorvusRetro.
Héberger les ZIP dans CorvusRepository demandera une migration séparée de
la liste des origines autorisées, des tests et du processus de publication.

Voir [les modules natifs](individual-cores.md) et la
[procédure de publication du nouveau dépôt](https://github.com/CorvusSoftware/CorvusRepository/blob/main/docs/PUBLISHING.md).
