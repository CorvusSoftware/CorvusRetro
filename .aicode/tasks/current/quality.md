CorvusSoftware Quality Gate
Status: PASS
Risk: DEEP

Criteria:
- PASS : continuité pendant perte de focus, vraie pause HOME et reprise EGL/audio observées sur Android.
- PASS : FIFO audio, amorçage, adaptation bornée, reprise et callbacks tardifs testés.
- PASS : croix portrait abaissée, bornes et autres géométries préservées.
- PASS : suppression confirmée, autre moteur préservé, échec batterie conservant le moteur, ROM/batterie/historique conservés et réinstallation.

Evidence:
- 17 suites natives strictes et 17 ASan/UBSan passent, dont 284 contrôles audio.
- 19 tests JVM, frontend sans cœur et 8 tests de workflows passent.
- Lint, builds debug/release et contrôle du paquet debug deux ABI/16 Ko passent.
- Android : 41 contrôles playback portrait, 41 paysage et 91 suppression portrait passent.
- Après précision du nom dans la confirmation, les trois suites menu/registre/suppression strictes et ASan passent, ainsi que le rebuild debug/runner et lint.
- CI 41491545182 réussie sur la source 5ccdb7b ; tout nouvel exact head doit être contrôlé avant livraison/fusion.

Review:
- Revue indépendante effectuée ; les entrées maintenues après perte EGL et le callback d'erreur tardif ont été corrigés et relus. Aucun finding concret restant.

Open risks:
- Pas d'écoute physique ni de téléphone/Bluetooth/runtime ARM64 qualifiés.
- Le lanceur revient en portrait après HOME ; pas de preuve de retour HOME en paysage.
- Les politiques audio Android, notamment les appels, restent applicables.

Decision:
- Implémentation prête à la revue. Publication de release et fusion séparées. Le statut distant exact de la PR doit encore être observé après le dernier push.
