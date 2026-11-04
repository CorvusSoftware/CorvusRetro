# Carte de la refonte native

- `menu/menu_driver.[ch]` possède l'API publique, la navigation, les entrées, le cycle de vie et la coordination du moteur actif.
- `menu/menu_displaylist.c` construit les lignes, filtres et métadonnées à partir de `cr_ui`. `menu_entries.h` porte leurs types, lecteurs et sélecteurs partagés sans dépendre du dessinateur.
- `menu/menu_setting.[ch]` possède les spécifications immuables, lecture/validation initiale et application des préférences. `menu/cbs/menu_cbs_ok.c` exécute les actions de cœur/contenu/états/import et transferts ; `menu_cbs.h` conserve leurs IDs et contrats privés.
- `menu/menu_internal.h` limite le pont actions/pilote aux changements de page et à la synchronisation des succès. `CORVUS_MENU_SOURCES` est réutilisée par le frontend et native_registry_test.
- `menu/interface_layout.[ch]` fournit la géométrie commune au dessin, toucher, focus et défilement. `interface_draw.[ch]` et `widgets.[ch]` dessinent les cartes, grille, commandes de menu et panneaux au travers de `cr_painter`.
- `cores/state_store.[ch]` gère les états nommés, métadonnées et aperçus PPM transactionnels sous `states/library`. Il appelle `cr_engine_state_copy/restore` sur le propriétaire. Les anciens manuels/auto et la batterie gardent leurs chemins.
- `frontend/platform.h` -> `android_backend.[ch]` -> `MainActivity` : frontières JNI bornées pour données jeux/catalogue, clavier, images et partage. Les résultats texte sont filtrés par ticket de génération et champ actif, avec mailbox invalidée à chaque nouvelle édition.
- `GameLibrary` conserve identités et index ; la jaquette facultative est une métadonnée indépendante. `FrontendImages`/`FrontendImageFiles` préparent les bitmaps sur un worker ; GL garde une LRU bornée de textures jusqu'à512px.
- `StateShares`/`StateShareFiles` -> `StateShareProvider` : export d'un instantané de données en lecture seule par URI privée et permission temporaire.
- `input/play_layout`, `input/overlay`, les pilotes audio, CorvusCore/CorvusAPI et leurs pins ne sont pas modifiés par cette refonte.
- Icônes : `gfx/render.h` -> `MainActivity.ICONS` et les 42 VectorDrawable -> atlas rasterisé -> `gfx/drivers/gl2`. Base commune464, 48 cellules réservées ; glyphes fixes0..232, cacheUnicode233..463. Taille1024x2048 et marqueur blanc conservés. Voir `docs/icons.md` et `IconAtlasInstrumentation` avant toute modification du contrat.

Preuves : native_interface (parcours réels), native_state_store (135 contrôles), anciennes suites natives et ASan/UBSan, tests Java, FrontendServicesInstrumentation et nouveau InterfaceRedesignInstrumentation.

La [carte structurelle](../docs/repository-structure.md) compare ces frontières
aux fichiers RetroArch réellement lus à la révision 168c451a le 4 novembre 2026.
La séparation suit les responsabilités : navigation/actions, géométrie, dessin,
primitives visuelles, stockage et services Android.
