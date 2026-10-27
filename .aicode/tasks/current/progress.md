État : DONE pour le code et la distribution, contrôler la CI de ce commit documentaire après push.
Demande : notifications inspirées de RetroArch, lancement avec icône, succès et progression avec badges.
Source publiée a4a76e71e98f12ffca88e05bc3e0cba0e64e7bab, tagv0.15.0, app0.15.0/code16.
Les changements utilisateur6f00c2d sont conservés : découverte multi-fournisseur CorvusRepository, aucun catalogue JSON local réintroduit, SDK/pins inchangés.

Cartes C de lancement/déblocage/complétion : file8, maintien4s, animation330ms. Progression mesurée avec badge verrouillé et durée2s, HIDE/reset/fermeture.
PNG officiels téléchargés et décodés hors rendu, caches bornés, fallback corbeau et texturesGL16. Placement hors commandes, choix de l'espace le plus lisible en paysage court.
Code original CorvusSoftware, rcheevos MIT et référence RetroArchdda31fef21aeb472be9de7ee53eb386cc8db1417.

Preuves :9CTest et9ASan/UBSan,136contrôles client,1682widgets, géométrie6tailles x3systèmes. Builds Debug/Release/lint et base sans moteur/sixELF16Ko passent.
Android15 :29contrôles images,16popups portrait et16paysage,51téléchargeur multi-fournisseur. Vrai cœurGB et programme original A=33 puisB=66, progression33/66 et un succès ; pixels des images, expiration, pause/reprise et déchargement vérifiés, captures inspectées.
Batterie GBA historique SHA256d5b703aea92f05004a0d761ab80aecf958f950f96bd1cafc27bbc7fc935fc3f1 inchangée.
Revue indépendante : crash rcheevos différé à l'unload corrigé sans modifier la dépendance ; P2 de lisibilité du corridor corrigé avec régression ; aucun P1/P2 bloquant ouvert connu.

CI40460984893, build signé40461309741, publication40461656784 réussis sur la source exacte.
Sept assets publics vérifiés indépendamment : APK0.15.0/code16, signataire durable, absence de moteurs et notice MIT ; sixZIP0.14.1 avec ABI/16Ko/extension mémoire.
Les sixZIP sont identiques aux modules0.14.1 déjà qualifiés, aucun catalogue externe modifié.
Mise à jour du vrai APK signé sur Android10 acceptée ; thème ASUS et historique conservés, capture finale inspectée.

Limites : protocole et images synthétiques dans les tests, aucun compte réel, téléphone physique ou exécutionARM64 revendiqué.
Observer la CI du commit documentaire. Reçu détaillé : /workspace/setup/CorvusRetro-0.15-qualification.txt.
Ne pas remplacer les assets immuables et ne pas modifier CorvusRepository pour des moteurs inchangés.
