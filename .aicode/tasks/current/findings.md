# Faits vérifiés

Références : RetroArch ca3f823 et assets d9f9690. MaterialUI associe le chargement à cores.png ; thème NVIDIA Shield exact décrit dans docs/retroarch-reference.md.
CoreManager possède un véritable handle idle et le transfère à un seul lecteur, sur le HandlerThread FIFO partagé. Fermeture et déchargement libèrent le handle et publient Aucun cœur. Les appels d'un autre menu pendant la lecture sont refusés.
Revue : lecture de loaded capturée une seule fois par render, évitant un index négatif lors d'une libération concurrente. Aucun autre finding bloquant.
Suppression complète des anciens fichiers applicatifs, JNI et C du laboratoire, de ses tests et archives, des ressources et des commandes de build correspondantes. L'APK propre ne contient plus les anciennes classes ni leurs symboles JNI.
Politique CI : interdiction des tirets Unicode longs dans tous les fichiers du dépôt parent. Ponctuation du millésime des notices adaptée en ASCII ; tous les crédits conservés. Sous-module CorvusCore inchangé.
Tests : moteurs JNI GB/GBC/GBA, handle idle, contenu, états, batterie, déchargement/rechargement PASS. Build propre et lint PASS, signature et alignements ZIP/ELF 16 Ko PASS.
Android 10 x86_64 : chargement/remplacement/déchargement des trois moteurs, conservation du cœur pendant changement de thème, lecture depuis historique, A et état avec comparaison des pixels, refus d'un second menu, fermeture/rechargement, nouvelle batterie GBA disque et listes PASS. Aucun crash observé.
Installation cloud actualisée et rejouée avec succès. Script et instructions de démarrage enregistrés en brouillon pour revue dans les réglages.
