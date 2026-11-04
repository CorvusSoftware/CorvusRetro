# CorvusSoftware Quality Gate

Status: PASS
Risk: DEEP

Criteria:
- [PASS] Extension ABI0 optionnelle, anciens modules et réponses malformées.
- [PASS] Effets réels GB/GBC/GBA, joueurs isolés, refus et allocations atomiques,
  conservation après reset, états et import batterie ; fermeture sans perte de cœur.
- [PASS] Parser `.cht` borné, UTF-8/BOM, échappements, doublons, handler mémoire
  refusé, fichiers atomiques et profils privés corrompus protégés.
- [PASS] Édition et collage non destructif, callbacks périmés/annulés, transferts
  SAF/JNI et navigation Android portrait/paysage.
- [PASS] Build/lint/JVM, APK sans cœur, deux ABI 16 Ko, atlas cohérent.

Evidence:
- CMake/CTest Release : 24/24 ; ASan/UBSan/leaks : 24/24, puis 7/7 ciblés.
- Dernier ajustement des noms vides : native_menu_cheats/native_cheat_store 2/2
  dans les deux builds, puis build Android et lint réussis.
- Gradle Debug/testDebugUnitTest/lintDebug : PASS, 37 tests JVM.
- InterfaceRedesignInstrumentation, mode cheats_only : PASS portrait/paysage,
  ROM synthétique et nouveaux modules Core x86_64.
- IconAtlasInstrumentation : 201 contrôles PASS.
- check_core_apks.py Debug : package, signature de debug, zéro cœur, ABIs et
  alignement 16 Ko PASS ; 8 tests des workflows de publication PASS.
- CI API7/Core13 : tous les jobs des commits épinglés verts. CI frontend initiale
  533985b verte ; consulter PR32 pour le contrôle exact du dernier commit.

Review:
- Revue indépendante UI/JNI/SAF réalisée. Finding P2 de collage corrigé : une
  insertion refusée retourne la sélection originale au filtre Android.
  Régression JVM passée ; aucun finding critique ou élevé restant.

Open risks:
- Pas de validation sur téléphone physique. Pas de Hardcore ni recherche mémoire.
- La distribution des nouveaux modules exige les PR API/Core et la qualification
  séparée de CorvusRepository. Aucun module ni catalogue de production publié ici.

Decision:
- Implémentation prête pour revue, preuves locales suffisantes et frontières
  critiques exercées. Aucune fusion ou publication de version exécutée.
