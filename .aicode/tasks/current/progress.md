# Codes de triche

État : VERIFY.

- RetroArch examiné à 665064a6f4867945bd2598e43cb7c731bafb2ae1, sans copie de source.
- CorvusAPI PR7, commit38928026e9b592eb1414ab6de04f060fa9893365 : 4 jobs CI verts.
- CorvusCore PR13, commitc23943ad2629a5f3e7e3ed5f190f50504d052def : 6 jobs CI verts.
- CorvusRetro : listes par contenu/joueur, édition, application, import/export .cht,
  indicateur d'indisponibilité des anciens cœurs, atlas49icônes.
- 24/24 tests natifs ; 24/24 ASan/UBSan/leaks puis 7/7 ciblés après corrections.
- Android Debug, lint, 37 tests JVM, contrôle APK sans cœur et deux ABI16Ko : PASS.
- Revue indépendante UI/JNI/SAF : P2collage refusé supprimant la sélection corrigé,
  avec régression JVM ; aucun autre findingP0/P1/P2.
- Qualification Android portrait/paysage en cours : codes natifs et IME déjà
  parcourus ; moniteur de test du sélecteur SAF corrigé et relance en cours.

Aucun appareil physique qualifié, aucune fusion, aucune nouvelle release publiée.
