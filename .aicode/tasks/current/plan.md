Objectif : appliquer réellement le découpage par responsabilité de RetroArch au menu natif CorvusRetro. Budget DEEP, comportement inchangé.
Base : e4061e8 sur feature/mobile-interface-redesign, PR #30 ouverte, main 69952eb.
1. Extraire construction des entrées/recherche/métadonnées dans menu_displaylist.c.
2. Extraire spécifications et logique des réglages dans menu_setting.c.
3. Extraire validation et opérations de contenu/sauvegarde dans cbs/menu_cbs_ok.c.
4. Garder navigation, entrées et lifecycle dans menu_driver.c ; types de lignes indépendants du renderer et contrats privés, API publique inchangée.
5. Déclarer une seule liste CMake partagée par frontend et test de registre ; vérifier déplacement mécanique, suites strictes/ASan, zéro cœur, APK/lint, parcours Android et revue indépendante.
6. Actualiser la carte du code, publier dans PR #30 et vérifier CI exacte. Aucune fusion ou release.
