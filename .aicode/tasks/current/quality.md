# Quality gate

Preuves locales : PASS pour compilation Android ARM64/x86_64, lint sans erreur,
signature/ZIP/ELF 16 Ko, tests C/ASan/UBSan et Java/JNI des trois moteurs.
Revue indépendante : aucun finding bloquant restant après corrections.
Essais Android 10 x86_64 : voir docs/validation-android.md pour le périmètre exact.
Publication et construction distante : le workflow Android APK contrôle chaque
push sur main ; résultat et artefact disponibles dans GitHub Actions.

Limites : pas de téléphone physique ou d'exécution ARM64 ; ni qualité audio,
ni compatibilité commerciale, ni performances mobiles démontrées. GBA expérimental.
