# Codes de triche

État : READY_FOR_REVIEW. Implémentation et qualification locale terminées.

- Référence RetroArch : `665064a6f4867945bd2598e43cb7c731bafb2ae1`, sans copie de source.
- CorvusAPI [PR7](https://github.com/CorvusSoftware/CorvusAPI/pull/7),
  `38928026e9b592eb1414ab6de04f060fa9893365` : 4 jobs CI verts.
- CorvusCore [PR13](https://github.com/CorvusSoftware/CorvusCore/pull/13),
  `c23943ad2629a5f3e7e3ed5f190f50504d052def` : 6 jobs CI verts.
- CorvusRetro [PR32](https://github.com/CorvusSoftware/CorvusRetro/pull/32) :
  listes par contenu/joueur, édition, application, import/export `.cht`,
  compatibilité des anciens cœurs et atlas de 49 icônes.
- 24/24 tests natifs et 24/24 ASan/UBSan/leaks, puis 7/7 ciblés après corrections.
  Dernier ajustement des noms vides : 2/2 régressions normales et instrumentées.
- Android Debug, lint, 37 tests JVM, APK sans cœur et deux ABI 16 Ko : PASS.
- Émulateur Android API29/x86_64 : parcours triche en portrait et paysage PASS,
  y compris IME, tickets périmés, refus, application automatique, JNI et SAF.
  Modules de la branche Core testés en archives locales ; aucun faux catalogue publié.
- Atlas Android : 201 contrôles de coordonnées, unicité et rendu PASS.
- Revue indépendante UI/JNI/SAF : collage refusé conservant la sélection corrigé
  et couvert en JVM ; aucun autre finding P0/P1/P2.
- Première CI frontend au commit `533985b` verte, run `41928302967`.
  L'état du dernier commit est accessible dans les contrôles de PR32.

Aucun téléphone physique qualifié. Aucune fusion ni nouvelle release publiée.
Les archives de cœurs restent à qualifier/publier par CorvusRepository après les
PR dépendantes ; le catalogue courant n'est pas modifié.
