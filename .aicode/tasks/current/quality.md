AICode+ Quality Gate
Status: PASS
Risk: DEEP

Criteria:
- [PASS] Extensions publiques optionnelles publiées sans changement de l'ABI0.
- [PASS] Options réelles, choix mémorisés et erreurs disque avec restauration testées.
- [PASS] Deux contenus, contrôles indépendants et batteries distinctes, même ROM comprise.
- [PASS] Imports annulés/périmés, échecs de chargement et fermeture gardant le cœur.
- [PASS] Capacités de session respectées, états solo et compte/envois RA conservés.
- [PASS] Base signée durable et archives réelles publiées, octets publics contrôlés.
- [PASS] Catalogue promu après contrôle des vrais fichiers, CI exacte réussie.

Evidence:
- CorvusAPI : headers C11/C++20, contrat, packaging et CI qualifiés avant fusion #5.
- CorvusCore :35 tests Release ; ASan/UBSan et six jobs CI verts, injection d'échec reset et transferts série synthétiques.
- Frontend : make test12/12 ; SANITIZE=ON12/12 ; corefree, Gradle Debug/Release/lint/instrumentation réussis.
- Android API29 x86_64 : six parcours GB/GBC/GBA portrait/paysage,782 contrôles ; ZIP publics puis GBA paysage,134 contrôles supplémentaires.
- Mise à jour signée0.17.1->0.18.0 sans désinstallation : même UID, ASUS et historique conservés, aucun crash applicatif observé sur profil sans cœur.
- APK et six ZIP publics identiques aux fichiers qualifiés ; signature, SHA256, ABIs, exports et segments16Ko vérifiés.
- Catalogue :13 tests, six entrées, métadonnées GitHub et CI main40743649380 réussis.

Review:
- Revues indépendantes API, fournisseur/reset, engine/persistance/états et menus/contrôleurs ; correctifs P2 revus, aucun restant.
- Échec Android link() diagnostiqué et corrigé sans contournement SELinux ; tests de régression et parcours Android repassés.

Open risks:
- Link GBA expérimental ; états groupés et succès pendant le link non exposés par les modules officiels, actions désactivées et limites documentées.
- Aucun runtime ARM64, appareil physique ou compte RA personnel revendiqué ; ces essais ne sont pas requis pour la qualification cloud retenue.

Decision:
- Critères couverts par des preuves de contrat, persistance, runtime et distribution ; livraison terminée.
