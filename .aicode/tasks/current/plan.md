Objectif actualisé : corriger la PR21 refusée visuellement, conserver les dimensions et la disposition d'origine0.20.0. Garder32icônes originales, ordre rapide et retrait du libellé expérimental GBA.
1. Restaurer dessin/touch/scroll et barres d'origine, retirer autoDPI/JNI/fontes/wrapmodule et tests devenus sans objet.
2. Conserver les tests antérieurs de navigation et adapter uniquement la correspondance des actions rapides.
3. Vérifier natif15, ASan15, Android debug/lint/JVM et captures portrait/paysage ; revue indépendante.
4. Publier la correction sur la même PR21, attendre la CI du SHA exact. Pas de fusion ni release pendant cette correction de PR à revoir.
