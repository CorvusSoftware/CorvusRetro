# Téléchargeur de cœurs CorvusRetro

CorvusRetro consulte le catalogue officiel **[catalog/corvuscore.json](https://github.com/CorvusSoftware/CorvusRepository/blob/main/catalog/corvuscore.json)**,
hébergé par CorvusRepository :

```text
https://raw.githubusercontent.com/CorvusSoftware/CorvusRepository/main/catalog/corvuscore.json
```

Le catalogue contient les trois moteurs officiels CorvusCore (GB, GBC, GBA)
pour `arm64-v8a` et `x86_64`, dans le format 3 de CorvusAPI 0.
Les ZIP restent publiés par les releases CorvusRetro. Avant le chargement,
l'application contrôle l'origine de téléchargement, la taille, le SHA-256,
le contenu de l'archive et l'architecture ELF.

## Distribution

Le workflow de CorvusRetro construit et vérifie les six archives, puis produit
un fichier temporaire `catalog/corvuscore.json` comme artefact de revue.
Les données du catalogue doivent être publiées et validées dans CorvusRepository,
sans versionner de JSON dans CorvusRetro.

## Rupture volontaire avec les anciens APK

Le mainteneur a demandé la suppression des trois anciens catalogues du dépôt.
Les versions de CorvusRetro qui utilisaient les URL historiques (notamment
v0.13.0) ne peuvent plus rafraîchir les cœurs. Il faut publier un nouvel APK
pour activer le catalogue externe. Ce nettoyage ne supprime pas les jeux,
les sauvegardes ni les modules déjà installés chez les utilisateurs.

## Autres fournisseurs

Pour éviter les confusions, CorvusRepository réserve un dossier et un fichier
nommés selon le fournisseur : `catalog/corvuscore.json`. Les nouveaux
fournisseurs ne seront **pas chargés automatiquement** : CorvusRetro ne prend
en charge que les trois identifiants officiels jusqu'à une évolution du
registre, de la validation et des tests de sécurité.

[Archives natives](individual-cores.md) ·
[Publication des catalogues](https://github.com/CorvusSoftware/CorvusRepository#publier-une-mise-à-jour)
