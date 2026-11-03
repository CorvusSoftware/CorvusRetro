État : READY_FOR_REVIEW. Correction des parasites persistants après 0.22.1.
Base main fca4f45 ; branche fix/audio-clock-and-save-io. Le dépôt distant a été revérifié et reste sur cette base.

Implémenté :
- audio_sync portable : cible amorce + demi-lot, EMA 0,5 s, correction proportionnelle bornée à +/-0,5 %. Lecture propriétaire de la file AAudio ; au plus trois trames avant chaque rendu. PCM inchangé et évaluation RA par trame.
- Worker batterie persistant, un instantané immuable à deux joueurs maximum. Aucune API cœur sur le worker ; acquittement propriétaire après durabilité de tous les joueurs. Pause/fermeture/reset/import/restauration attendent les écritures concernées. Échec conserve cœur et générations sales.
- Régressions native_audio_sync et native_async_battery ; test Android playback étendu à GBA avec vérification de sauvegarde pendant le jeu.

Preuves :
- 19/19 suites natives strictes et 19/19 ASan/UBSan passent.
- 38 scénarios de dérive contrôlée : PCM exact et zéro silence ; baseline +100 ppm/600 s reproduit 1 597 trames de silence. Coûts CPU 6/10/15 ms passent ; surcharge 25 ms explicitement distinguée.
- Batterie : 321 contrôles, fsync bloqué pendant émulation GBA, échecs/récupération, générations récentes, câble link et barrières de cycle de vie.
- Revue indépendante finale : aucun finding concret. Android assembleDebug/assembleDebugAndroidTest/lintDebug passent. GBA API29 x86_64 : 48 contrôles portrait + 48 paysage, batterie 32 Ko/0x5a observée pendant le jeu avant menu ; notifications, HOME/reprise et fermeture passent, aucun nouveau crash. APK core-free/deux ABI/16 Ko et huit tests de publication passent.

Livraison : publier la branche en PR et contrôler les checks GitHub sur son SHA actif avant de la déclarer prête. Les checks distants sont la source de vérité pour la CI. Après revue, la prochaine qualification fonctionnelle est une écoute sur le Redmi ; ne pas fusionner ni publier une version dans ce lot.
Limite : pas de ROM Pokémon ni de téléphone accessible ; aucune écoute physique effectuée. Les défauts du frontend sont reproduits, pas la cause exacte des parasites du Redmi.
