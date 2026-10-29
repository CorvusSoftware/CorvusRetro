État : VERIFY. Correction du téléchargeur vide après migration des archives officielles.

Cause reproduite dans le vrai parser Java : CoreRepository.Core accepte deux dépôts dans la regex, puis une condition supplémentaire réserve les officiels à CorvusRetro. L'APK0.18.0 refuse donc les URL CorvusRepository. Les contrôles précédents de métadonnées/ZIP et les installations par fixtures ne prouvaient pas ce parcours ; l'affirmation précédente de compatibilité était incorrecte.

Repo #6 fusionnée : ancien catalog/corvuscore.json restauré byte pour byte depuis43218e4 ; catalogue autonome conservé souscatalog/v1. Outils de promotion limités au nouveau dossier,56 tests et métadonnées6+6 passent. APK0.18.0 retrouve ses archives historiques sans remplacement des fichiers.

Frontend branchefix/independent-core-catalog : retrait du filtre supplémentaire, deux origines HTTPS curées toujours requises ; découverte et validation des chemins catalog/v1. Version0.18.1/code23 pour mise à jour signée compatible. Quatre tests JVM exécutent le vrai parser, ajoutés à la CI. Test central rouge reproduit avec l'ancien filtre ; test vert initial obtenu, suite finale+instrumentation Android en cours.

À finir : tests JVM/Android avec vrais catalogues publics et affichage du téléchargeur ; revue et CI, fusion, qualification permanente signée, publication APK0.18.1 puis contrôle des octets publics. Ne pas annoncer le correctif APK publié avant son existence réelle.
