État : code et distribution 0.13.0 publiés, catalogue public vérifié prêt pour main.
Demande : nettoyage, organisation inspirée de RetroArch et RetroAchievements fonctionnel.
Autorisations : publication main et APK ; extensions CorvusAPI/CorvusCore explicitement approuvées.

La migration amont50105de du header unique corvusapi.h est conservée.
Source fonctionnelle2248329, correction des chemins de productionf87a0ca2544696d57992189c2ce1003230ceff3b.
CorvusCore2d7bdd2020894fa27d153cbecd421da47bcb37f4 (0.14.1), APIbfdbed337ed7bc7334ed2d8e1f510ab503fa475d, rcheevos1433173220a7eaede6a9ed7a18e94117be1821e0 (MIT).

Structure : menu/cores/gfx/input/audio/frontend/cheevos/tasks/deps, Android sous pkg/android.
Anciens producteurs et projets de splits, documentation et captures obsolètes supprimés ; catalogues historiques conservés.
RetroAchievements normal : interface C, clavier Android, rc_client officiel, lecture mémoire facultative publique ABI0.
HTTPS strict, session et file AES-GCM Keystore, reprise après fermeture du jeu, déconnexion invalidant les anciens travaux.

Qualification : 8CTest et8ASan/UBSan,78contrôles client ; builds Debug/Release, base sans moteur, sixELF16Ko par variante.
Android15 x86_64 :14compte/Keystore/nonblocage,1restauration processus,13file persistante,15parcoursUI,39téléchargeur.
Programme GB original exécuté par le vrai cœur : A modifie WRAM, succès obtenu,503 puis fermeture et reprise sans doublon.
Ancienne batterie GBA inchangée : SHA256d5b703aea92f05004a0d761ab80aecf958f950f96bd1cafc27bbc7fc935fc3f1.
Transport synthétique ; aucun compte réel, jeu commercial, téléphone physique ou exécutionARM64.

CI source40394372804, build signé40394816280 et publication40395188701 réussis.
Première construction signée40393518093 avait échoué : sortie AGP RelWithDebInfo contre Release attendu. Correction reproduite et désormais contrôlée par la CI principale.
Tagv0.13.0 immutable surf87a0ca : base0.13.0/code14 et sixZIP0.14.1 publics.
Signature de la vraie base vérifiée contre l'ancrage indépendant ; sixZIP réels vérifiés, extension mémoire et catalogue exact.
Mise à jour du vrai APK signé0.12->0.13 sur Android10 acceptée, thème ASUS et historique conservés, captures inspectées.
Branche candidateautomation/core-catalog-40395188701-1 modifie uniquement cores-v3.json et correspond exactement aux octets publics.

Observer la CI du commit de promotion après push. Reçu détaillé : /workspace/setup/CorvusRetro-0.13-qualification-receipt.txt.
Ne pas reconstruire ou remplacer les assets immuables ; ne pas réécrire les changements utilisateur.
