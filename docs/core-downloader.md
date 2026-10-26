# Téléchargeur de cœurs CorvusRetro

Le parcours actuel0.12 utilise des archives natives individuelles : consulter
[la référence RetroArch, le parcours et la publication](individual-cores.md).
Le service Android ne lance plus PackageInstaller et ne demande plus la permission
REQUEST_INSTALL_PACKAGES. Les cœurs apparaissent immédiatement après téléchargement.

## Catalogues conservés

| Client | Catalogue | Distribution |
| --- | --- | --- |
| 0.12 | cores-v3.json, format3 | ZIP natif par cœur et ABI, CorvusAPI0 |
| 0.11 | cores-v2.json, format2 | Split APK signé lié à une base Android |
| 0.10 | index.json, format1 | Ancien pack APK complet |

Ne pas réinterpréter les anciens formats ni modifier leurs URLs immuables.
Les validateurs historiques restent utiles aux anciens clients ; les nouveaux
contrôles sont tools/validate_core_archives.py et tools/test_core_archives.py.
La signature durable de la base Android reste inchangée.
