RetroArch : rcheevos_show_game_placard et rcheevos_show_achievement_popup partagent gfx_widget_achievement_popup ; progression dans gfx_widget_leaderboard_display.
Les URLs sont fournies par rc_client_game_get_image_url et rc_client_achievement_get_image_url ; badge couleur unlocked, badge verrouillé active.
Images officielles media.retroachievements.org, /Images/*.png et /Badge/*(_lock).png.
Les images tardives ne doivent jamais créer une notification ; seul le thread frontend gère les notifications et utilise les URLs de son jeu courant.
Le viewport C est déjà décalé des barres Android. En portrait limiter les notifications à la zone au-dessus du panneau tactile ; en paysage dégager les épaules et A/B.

Repriseamont6f00c2d : catalogueslocauxretirés volontairement, CorvusRepository auto-discovery conservée, v0.14.0 existe.
rc_client_unload_game détache client.game puispeutlevermastery différée ; appelresetpendingUI avantunload corrige sansmodifierdependency. Régression mesurée passeASan.
