État : ACTIVE, qualification locale terminée, publication à effectuer.
Demande : nettoyer le dépôt, organisation inspirée de RetroArch, RetroAchievements fonctionnel.
Autorisation utilisateur : publication main et CI/APK ; extensions publiques CorvusAPI/CorvusCore approuvées.

Amont50105de intégré sans perdre la migration du header unique corvusapi.h.
Branche temporaire work/retroarch-layout-achievements ; checkpoints locaux cba7bd0/50c28ff à consolider avant publication.
Pins : CorvusCore2d7bdd2020894fa27d153cbecd421da47bcb37f4 ; CorvusAPIbfdbed337ed7bc7334ed2d8e1f510ab503fa475d ; rcheevos1433173220a7eaede6a9ed7a18e94117be1821e0.

Réalisé : menu/cores/gfx/input/audio/frontend/cheevos/tasks/deps aux racines, Android sous pkg/android.
Anciennes features APK, producteurs obsolètes et captures supprimés ; anciens catalogues clients conservés.
Base sans moteur, six modules directs, archives individuelles ; app0.13.0/code14, moteurs0.14.1.
RetroAchievements normal : interface C, clavier Android, protocole officiel rcheevos, lecture RAM publique sur le même handle.
HTTPS strict et borné, session et file de succès AES-GCM Keystore ; déconnexion invalide les tâches anciennes.
Revue indépendante : ordre des écritures, remplacement IME, reprise après fermeture du jeu et blocage crypto corrigés.

Preuves locales : 8CTest normaux et8ASan/UBSan ;78contrôles client ; base sans cœur ; sixELF16K ; Gradle/lint passent.
Android15 x86_64 :14contrôles compte/chiffrement/nonblocage, restauration processus neuf1, outbox13, parcoursUI15.
Le vrai cœur GB exécute un programme original : A modifie WRAM et déclenche le succès ;503 puis fermeture et reprise sans doublon.
Téléchargeur :39contrôles avec les nouveaux moteurs ; ancienne sauvegarde GBA SHA256d5b703aea92f05004a0d761ab80aecf958f950f96bd1cafc27bbc7fc935fc3f1 conservée.
Transport réseau de test synthétique ; aucun compte réel ou jeu commercial. Pas de téléphone physique ni exécutionARM64.

Prochain : recontrôler amont, consolider les commits locaux, pousser main et observer la CI exacte.
Après CI et preview signé verts : tagv0.13.0, vérifier les sept assets publics et promouvoir uniquement leur catalogue réel.
Ne pas redemander les autorisations déjà accordées ; ne pas publier de métadonnées fictives.

CI source2248329 réussie40392940891. Preview signé40393518093 échoué à la validation des modules.
Cause reproduite : AGP utilise RelWithDebInfo mais les consommateurs attendent native-cores/Release.
Correction : normaliser le répertoire des variantes optimisées sans changer les options de compilation.
La CI principale construit maintenant aussi Release et empaquette ses six archives pour éviter la régression.
Builds Debug et Release corrigés réussis ; six archives réelles de chaque variante validées, ABIs/16Ko/export CorvusAPI. Aucun changement de code runtime après les tests Android finaux.
