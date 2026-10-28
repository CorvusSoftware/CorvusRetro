AICode+ Quality Gate
Status: PASS
Risk: STANDARD, menus C11 sur contrats existants.

Criteria:
- PASS : entrée de configuration conditionnée au cœur chargé, maintien après fermeture du jeu, disparition à l'unload ; native_menu avec vrais GB/GBA, échecs de persistance préservés.
- PASS : options dans le menu rapide avant commandes, page distincte, pause et retour focus/scroll conservés ; natif et Android API29 dans deux orientations.
- PASS : états indisponibles fidèles à CorvusAPI actuel, aucune API privée ou capacité fictive ; références RetroArch immuables documentées.
- PASS : APK sans moteur, signature habituelle, upgrade compatible, publication immuable et provenance exacte.

Evidence:
- make test :9/9 ; SANITIZE=ON :9/9 ; corefree PASS.
- Gradle assembleDebug/assembleRelease/assembleDebugAndroidTest/lintDebug PASS ; outils archives4 et workflows6 PASS.
- API29debug :49 contrôles portrait et49 paysage PASS ; captures réelles des deux pages inspectées. Les tentatives startup ANR et timeout transport ne comptent pas comme succès ; logs conservés hors dépôt.
- Signature indépendante + source.json +SHA256SUMS vérifiés. Public0.17.1/code21 :1276332 octets, SHA256352e27a9c0ffaab8f10e0278b78384f1e8c89c825c670c340d17f6a16764e5b7, identique au candidat qualifié. Six ZIPs ABI/16KB validés et inchangés ; aucun catalogue frontend ajouté.
- Mise à jour signée0.17.0->0.17.1 acceptée et runtime sans cœur ; ASUS et historique préservés. Aucun cœur testé dans ce profil signé, aucun téléphone physique/ARM64 revendiqué.
- Sourcebfbf7b5666c2b2c5dcd2b23f3b21d57332571495 : CI40686561237, signature40687009888, publication40688881844 SUCCESS.

Review:
- Revue indépendante du diff : aucun P1/P2. Simplification : mécanismes ACT_PAGE/push_page/pop_page et renderer existants réutilisés, aucune abstraction nouvelle.

Open risks:
- Les deux pages restent indisponibles fonctionnellement tant qu'un contrat public de sous-systèmes/options et son implémentation n'existent pas. Le câble link n'est pas un livrable de cette version. Limite annoncée dans UI/docs/release.

Decision:
- Périmètre demandé de présence/navigation des entrées livré et prouvé. Ne pas assimiler cet ajout à une prise en charge de sous-systèmes ou d'options moteur.
