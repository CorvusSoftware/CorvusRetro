Objectif : améliorer la continuité audio/vidéo face aux notifications, abaisser légèrement la croix en portrait et permettre la suppression des cœurs téléchargés.
Budget : DEEP. Branche : feat/playback-and-core-management. Base : main 7303a32.
Référence : RetroArch 901ebff, sources consultées hors dépôt ; code CorvusRetro original.

1. AAudio : FIFO stéréo sans mutex, amorçage deux blocs, reprise différée, tampon matériel borné et protection des callbacks tardifs.
2. Android : distinguer focus et pause réelle ; effacer les entrées ; retrouver une surface EGL valide et une cadence stable après interruption.
3. Commandes : abaisser uniquement la croix en portrait, dans l'espace libre, sans changer le paysage ni les autres boutons.
4. Suppression : accès visible, confirmation, sauvegarde et fermeture du seul cœur concerné, fichiers privés propriétaires seulement, registre à indices stables et bibliothèque conservée.
5. Vérification : tests natifs stricts et sanitizers, JVM, Android, revue indépendante, PR et CI sur le commit exact.

Préparation de la version 0.22.0/code28. Ce lot sera proposé en PR ; la fusion et la publication d'une nouvelle release restent séparées.
