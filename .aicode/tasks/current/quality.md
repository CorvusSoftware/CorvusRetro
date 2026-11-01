CorvusSoftware Quality Gate
Status: PARTIAL
Risk: DEEP

Code : PASS local. Six catégories et sous-paramètres réels, préférences persistées et historiques conservées ; médias, manettes et sauvegardes reliés à leurs backends. Documentation du fonctionnement dans docs/settings-plan.md.
Preuves : 15 natifs, 15 ASan/UBSan, base sans cœur, 8 JVM, builds Android et lint passent ; 710 contrôles API29 x86_64 portrait/paysage dont64avecjeu réel etgéométrie/rendu personnalisés. Revue indépendante sans finding bloquant ; récupération .bak des états automatiques corrigée et retestée.
Livraison : CI exactcommit, fusionmain, APK signé durable, upgrade et publication en attente. Ne pas annoncer publié avant ces preuves.
Limites : aucune manette physique, téléphone physique ou exécution ARM64 ; axes des manettes automatiques, profil numérique commun. Connectivité Internet de l'émulateur au téléchargeur non qualifiée, problème connu distinct du lot paramètres.
