P0 : contenu exact des listes, IDs/actions/valeurs, navigation/focus/back et géométrie conservés ; suites native_menu, native_interface, configuration, bibliothèque et no-core.
P0 : ordre d'initialisation des préférences, effets d'application, stockage/états/batterie et callbacks inchangés ; tests natifs existants et comparaison mécanique des fonctions déplacées.
P0 : tous les nouveaux modules sont présents dans corvus_frontend et native_registry_test ; compilation stricte et ASan/UBSan, build core-free.
P1 : compilation APK ARM64/x86_64, lint, zéro cœur et alignement 16 Ko ; parcours Android sur code final.
P1 : revue indépendante de frontières, symboles privés et préservation, CI de la tête exacte.
