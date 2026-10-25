# Frontend natif CorvusRetro 0.5

Demande corrigée : utiliser la même approche C/C++ native que RetroArch, avec code original, et non des menus Java.
Budget DEEP : migration de rendu, entrées, cycle Android et propriété des moteurs. Deux tâches parallèles distinctes : plateforme Android Java et moteur C ; frontend et rendu C intégrés par le parent, puis revue indépendante.

Architecture cible : NativeActivity et native_app_glue, boucle événementielle C, menus et navigation C, EGL/OpenGL ES2 C, audio AAudio C, SDK C CorvusCore sur moteurs C++20. Java limité au sélecteur SAF, stockage Android et préférences compatibles, glyphes et ressources système rasterisés en atlas. Aucun menu ni widget applicatif Java.

Étapes :
1. Étudier les pilotes Android, GL et audio de RetroArch et définir les frontières. Terminé.
2. Terminé : implémenter moteur C avec sauvegardes compatibles et tests hôtes ; services Android NativeActivity.
3. Terminé : implémenter menus, texte, icônes, rendu du jeu et commandes tactiles/manette en C.
4. Terminé : remplacer l'ancienne couche Java/JNI, intégrer CMake/CI et tester build/lint/natif.
5. Terminé : tester sur émulateur les thèmes, chargements, import, états, batterie, rotation et reprise, puis revoir et corriger.
6. Documentation et cloud actualisés. Publication autorisée sur main avec contrôle du workflow Android APK.

P0 : compilation C11 réelle, tests moteurs et navigation, aucune logique de menus/émulation Java, fichiers/sauvegardes 0.4 conservés.
P1 : essais Android avec contenu synthétique original, contrôles C, importSAF, pause/reprise/rotation et fermeture/libération.
Contraintes : code original signé, zéro tiret Unicode long, CorvusCore figé intact, crédits conservés, aucune copie source RetroArch.
