État : DONE. Options du cœur et sous-système local à deux joueurs implémentés, qualifiés et publiés.

Sources publiques : CorvusAPI #5, main39c927c5a1a55b78ab55758a41cd16b0971c4684 ; CorvusCore #11, main063cf1f674788aa5913cebe184457a10b5946edd ; CorvusRetro #10, release67385be702dbf146f18b4e529b8559c9e38d0b32. Pins des dépendances uniquement, aucune modification de leurs fichiers dans le frontend.
APK0.18.0/code22 : https://github.com/CorvusSoftware/CorvusRetro/releases/tag/v0.18.0 ; 1302756 octets, SHA256 ea0d3a5f7b0b027cc99b65c5d17df6ba9d723a6914df68b36063b9e950477821. Signature durable conservée ; base sans moteur et six ZIP CorvusCore0.15.0 publics pour ARM64/x86_64.
Catalogue : CorvusRepository #1 fusionnée en43218e491e181e4966f0a7a20748f340539e10c6 après vérification des octets téléchargés, tailles et SHA256 des sept assets. Schéma6 entrées,13 tests et contrôle en ligne réussis ; CI main40743649380 verte.

Preuves : Core35 tests Release, ASan et six jobs CI qualifiés (main40737673245). Frontend12/12 natifs,12/12 ASan/UBSan, corefree, build Debug/Release/lint/testAPK. Six parcours configuration Android API29 x86_64 GB/GBC/GBA portrait/paysage :782 contrôles. Trois ZIP publics installés dans le profil de test, parcours GBA paysage production rejoué :134 contrôles supplémentaires.
Base signée0.17.1/21->0.18.0/22 acceptée sans désinstallation, même UID, ASUS et historique conservés, sans crash applicatif observé ; ce profil signé ne contient aucun cœur installé. CI frontend40740763674, build signé40741288414 et publication40742906577 SUCCESS. Octets de l'APK et des six ZIP publics identiques aux fichiers qualifiés.
Revues indépendantes : contrat API, fournisseur/reset, engine/persistance/états, menus/imports/contrôleurs ; aucun P1/P2 restant. Refus SELinux de link() reproduit, corrigé par backup copié et renommé atomiquement, requalifié sur Android. Alertes ANR système de démarrage identifiées et capturées avant reprise des essais, sans masquage d'erreur applicative.

Limites publiées : link GBA expérimental ; pas d'états groupés ni succès RA en link officiel, compte et envois conservés. Aucun appareil physique, runtime ARM64 ni compte RA personnel revendiqué. Installer la nouvelle base puis actualiser les cœurs dans le téléchargeur pour les nouvelles capacités.
Quality Gate : PASS, voir quality.md. Aucune action fonctionnelle restante pour cette tâche.
