# Livraison 0.24.0 et cœurs 0.16.0

Autorisation du mainteneur : fusionner CorvusRetro, préparer la sortie et mettre
à jour les cœurs dans CorvusRepository. Budget DEEP, signature et distribution.

1. Fusionner PR32 après sa CI exacte : fait, `01f3e03`.
2. Qualifier Core0.16.0 et sa révision immuable avec API fusionnée.
3. Préparer versionCode33, À propos, README et pin Core dans une PR de sortie.
4. Construire les six archives autonomes, vérifier les octets publics et leur
   exécution Android avant de promouvoir uniquement `catalog/v1/corvuscore.json`.
5. Qualifier la base signée habituelle et l'upgrade32->33, puis publier les octets
   contrôlés sous un nouveau tag ; aucun asset historique remplacé.
6. Vérifier le catalogue public et l'APK publié, documenter les preuves.
