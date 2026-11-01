Objectif : GBA sans mention expérimentale, icônes originales modernes complètes, ordre et dimensions selon référence tactile Android RetroArch.
Budget DEEP : géométrie commune dessin/touch/scroll, navigation portrait/paysage et atlas.
1. Audit RetroArch b28b8a4 : échelle Android0.75, métriques DPI/fontes/barres, lignes adaptatives et ordre réel.
2. Famille originale32icônes, IDs existants conservés, atlas480..511 et sémantique complète.
3. Métriques/wrap UTF8 communes au dessin/touch/clamp/retour, nav droite paysage et respect marges Android.
4. Table de présentation rapide sans changer IDs : groupes sauvegardes/commandes, informations puis Menu principal.
5. Tests natifs/mémoire/Android, atlas et captures, revue, CI/main et APK signé0.21.0/code27.
Conserver jeux/sauvegardes/préférences et3thèmes. Les marqueurs GBA retirés sont des libellés ; pas de nouvelle affirmation de validation matérielle universelle. CorvusCore/API et archives inchangés.

Matrice de validation : P0 dimensions/densité/rotation/wrapUTF8 + hitboxes/scroll/ordre et atlas32 ; P1 compilation native stricte16tests etASan16, basecorefree, Android2ABI/debug/release/lint/JVM et instrumentation réglages/visuel/atlas ; P2 CIexactSHA puisAPKsignémêmecertificat et upgrade0.20->0.21 sans effacement. Revue indépendante obligatoire. Limite matérielle : émulateurx86_64, pas de téléphone/ARM64/manettephysique.
