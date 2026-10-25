# Quality gate

PASS : frontend natif C11, rendu GLES2 et audio AAudio, services Java limités à Android.
P0 : tests natifs et mémoire, compilation C effective, APK/lint, intégrité des données et crédits.
P1 : scénarios réels Android et migration d'un ancien état, interfaces et import corrigés, aucune erreur restante observée.
Revue indépendante traitée. Vérifications du paquet : anciennes classes retirées, points d'entrée natifs et alignements présents.
Workflow Android APK construit et teste chaque push sur main ; résultat et artefact distants à consulter pour la révision publiée.
Limites : émulateur x86_64, pas de téléphone physique/Android récent ; TalkBack non exposé, qualité audio et performances mobiles à mesurer. GBA expérimental.
