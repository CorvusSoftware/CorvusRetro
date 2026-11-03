# Carte de la refonte native

- `menu/menu_driver.[ch]` possède la navigation, les filtres, le moteur actif, les opérations de sauvegarde et les callbacks plateforme.
- `menu/interface_layout.[ch]` fournit la géométrie commune au dessin, toucher, focus et défilement. `interface_draw.[ch]` et `widgets.[ch]` dessinent les cartes, grille, commandes de menu et panneaux au travers de `cr_painter`.
- `cores/state_store.[ch]` gère les états nommés, métadonnées et aperçus PPM transactionnels sous `states/library`. Il appelle `cr_engine_state_copy/restore` sur le propriétaire. Les anciens manuels/auto et la batterie gardent leurs chemins.
- `frontend/platform.h` -> `android_backend.[ch]` -> `MainActivity` : frontières JNI bornées pour données jeux/catalogue, clavier, images et partage. Les résultats texte sont filtrés par ticket de génération et champ actif, avec mailbox invalidée à chaque nouvelle édition.
- `GameLibrary` conserve identités et index ; la jaquette facultative est une métadonnée indépendante. `FrontendImages`/`FrontendImageFiles` préparent les bitmaps sur un worker ; GL garde une LRU bornée de textures jusqu'à512px.
- `StateShares`/`StateShareFiles` -> `StateShareProvider` : export d'un instantané de données en lecture seule par URI privée et permission temporaire.
- `input/play_layout`, `input/overlay`, les pilotes audio, CorvusCore/CorvusAPI et leurs pins ne sont pas modifiés par cette refonte.

Preuves : native_interface (parcours réels), native_state_store (135 contrôles), anciennes suites natives et ASan/UBSan, tests Java, FrontendServicesInstrumentation et nouveau InterfaceRedesignInstrumentation.
