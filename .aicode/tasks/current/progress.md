État : ACTIVE, qualification locale terminée, publication en attente.
Demande : notifications inspirées de RetroArch pour lancement avec icône, déblocage et progression avec badges.
Amont6f00c2d intégré dans work/achievement-popups ; checkpoint local113649d conservé. Version0.14.0/code15 déjà publique, nouvelle feature0.15.0/code16.
Aucun ancien catalogue JSON réintroduit ; découverte multi-fournisseur de CorvusRepository conservée, SDK/pins inchangés.

Implémenté : événements rc_client typés, cartes natives file8/4s/330ms, progression2s et HIDE/reset, cartes de jeu terminé, marges hors commandes.
Images publiques async et caches bornés, fallbackCorvus, texturesGL16 et JNI sans IO sur le frontend.
Référence RetroArchdda31fef21aeb472be9de7ee53eb386cc8db1417 inspectée, code frontend original.

Preuves :9CTest normaux et9ASan/UBSan passent ;136contrôles client et1682widgets ; builds Debug/Release/lint/testAPK passent ; base sans moteur/sixELF16Ko/archives validés.
Android15 :29contrôles images ;16contrôles popups en portrait et16en paysage, captures inspectées. Vrai cœurGB et programme original : A écrit33 puisB66, progrès33/66 puis succès, expiration2s, pause/reprise et clear après unload.
Revue indépendante : aucun P1 ; P2 du corridor trop étroit corrigé, non-régression de taille et absence de chevauchement vérifiées sur6tailles x3systèmes.
Crash rcheevos de mastery différée à la fermeture corrigé par reset pendant que le jeu est attaché, avant unload ; régression passeASan sans modifier la dépendance.
Le test de pixels Android lit les buffers en bloc ; les images de qualification sont synthétiques et ne sont pas dans la base.

Régression finale :51contrôles multi-fournisseur passent, batterie GBA historique inchangée.
Prochain : consolider les commits locaux depuis origin/main sans perdre les changements utilisateur ; pousser main, observer CI/signature, tagv0.15.0 et vérifier les assets publics.
Autorisation de publication persistante, ne pas redemander. Ne pas modifier les catalogues externes pour des moteurs inchangés.
