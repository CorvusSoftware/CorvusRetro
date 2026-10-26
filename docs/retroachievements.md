# RetroAchievements

CorvusRetro utilise rcheevos, la bibliothèque officielle MIT également employée
par RetroArch. La navigation, le rendu, la gestion du compte et l'adaptation
CorvusAPI sont du code original CorvusSoftware. Aucun code du frontend RetroArch
n'est copié. La notice MIT de rcheevos est conservée dans deps/rcheevos et dans
les licences accessibles depuis l'application.

## Parcours

1. Mettre les cœurs à jour dans Charger un cœur > Téléchargeur de cœurs.
2. Ouvrir Réglages > RetroAchievements et saisir le compte via le clavier Android.
3. Se connecter, puis ouvrir un jeu reconnu par RetroAchievements.
4. Consulter les succès depuis Menu rapide > Succès ou les réglages du compte.

Le mode normal est pris en charge. Le mode Hardcore n'est pas proposé. Le
chargement d'un état réinitialise l'évaluation des conditions ; les succès déjà
validés sur le compte restent acquis. Un ancien module sans extension mémoire
reste jouable et affiche clairement son absence de support des succès.

## Frontières et sécurité

- cheevos/ utilise rc_client pour login, hash ROM, session, liste, évaluations et envois.
- cores/ négocie l'extension optionnelle de corvusapi.h sur le même handle ABI0.
  Les accès mémoire sont synchrones, bornés et sans effet de bord, sur le thread
  propriétaire du cœur. Aucune lecture fictive remplie de zéros n'est utilisée.
- Le hash est calculé à l'ouverture du contenu ou à l'activation des succès,
  sans lecture du fichier ROM dans la boucle de frames.
- pkg/android/ fournit seulement l'IME, HTTPS et le stockage chiffré. Les champs
  et menus sont dessinés en C. Le mot de passe est éphémère et masqué à l'écran.
- Les jetons et requêtes de succès en attente utilisent AES-GCM avec Android
  Keystore. Aucun stockage en clair, redirection HTTP ou journal de credentials.
- tasks/ transporte des copies bornées des réponses au thread frontend. Les
  identifiants d'anciennes activités sont ignorés ; les workers ne touchent pas
  aux instances de cœurs ni au client rcheevos.
- Les succès sont persistés avant leur envoi, dédupliqués par compte/succès/mode,
  puis rejoués après récréation ou retour au premier plan. Les erreurs transitoires
  conservent la file ; la confirmation et les erreurs définitives la retirent.
  Une déconnexion invalide les tâches antérieures et efface les données restantes.

## Références vérifiées

RetroArch dda31fef21aeb472be9de7ee53eb386cc8db1417 : cheevos/cheevos.c,
les callbacks rc_client_read_memory/server_call, l'évaluation dans le cycle de
frames et la séparation des pilotes de plateforme. CorvusRetro adapte cette
organisation à CorvusAPI, avec les cœurs CorvusCore 0.14.1 et le header public
unique corvusapi.h. Les nouvelles déclarations mémoire n'altèrent pas l'ABI0.

## Vérifications

- Huit CTest natifs passent, avec ASan/UBSan pour le code original.
- Le client rcheevos a 78 contrôles de protocole, hash, RAM, succès, déconnexion,
  erreurs réseau, sauvegarde de progression et réponses tardives.
- Android 15 : 14 contrôles de compte, Keystore et absence de blocage du frontend,
  restauration dans un processus neuf et 13 contrôles de file persistante passent.
  Les 39 contrôles du téléchargeur passent avec les nouveaux cœurs.
- Le parcours clavier/compte est exercé avec un protocole synthétique. Un vrai
  cœur GB exécute un programme original qui attend A puis écrit en WRAM ; cette
  transition déclenche un succès. Après HTTP503 et fermeture du jeu, la file
  chiffrée envoie le succès au retour du réseau, sans doublon.

Les tests réseau utilisent des réponses synthétiques, aucun compte réel ni jeu
commercial. Ils ne constituent pas une qualification de connexion à un compte
personnel, de téléphone physique ou d'exécution ARM64.

Pour exécuter l'instrumentation dédiée :

```sh
./gradlew :app:assembleDebugAndroidTest \
  -PcorvusTestRunner=com.corvussoftware.corvusretro.RetroAchievementsInstrumentation
```

Le runner par défaut reste CoreRepositoryInstrumentation pour les régressions du
téléchargeur. Les artefacts de qualification ne sont pas embarqués dans la base.
