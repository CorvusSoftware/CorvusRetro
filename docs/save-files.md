# Sauvegardes du jeu

Les fichiers `.sav` et `.srm` contiennent ici la mémoire batterie brute du cœur.
Les deux extensions exportent les mêmes octets. Ce ne sont pas des états
instantanés : renommer un état ne permet pas de l'importer comme batterie.
La sauvegarde doit correspondre au jeu, au cœur et à la taille de sa mémoire.
Les formats particuliers d'autres émulateurs ne sont pas convertis.

## Exporter

1. Charger le cœur et ouvrir le jeu.
2. Ouvrir **Menu rapide > Sauvegardes du jeu**.
3. En câble link, choisir **Joueur de la sauvegarde** : Joueur 1 ou Joueur 2.
4. Choisir **Exporter en .sav** ou **Exporter en .srm**.
5. Choisir la destination avec le sélecteur de documents Android.

L'export prend une copie de la mémoire actuelle du joueur choisi, même si elle
n'a pas encore été enregistrée dans le stockage permanent. Il ne modifie pas
la sauvegarde interne. Le nom proposé en câble link indique le joueur 2 lorsque
celui-ci est choisi ; Android permet de modifier le nom de destination.

## Importer

1. Ouvrir **Menu rapide > Sauvegardes du jeu** et choisir le joueur en câble link.
2. Choisir **Importer une sauvegarde**, puis un document `.sav` ou `.srm`.
3. Vérifier la confirmation, puis choisir **Importer et redémarrer le contenu**.

La partie reste suspendue durant le transfert et la confirmation. Annuler
conserve la session. L'import vérifie la taille exacte attendue par le cœur,
avec une limite de 1 Mio. Le contenu redémarre pour utiliser la nouvelle batterie ;
en câble link les deux contenus redémarrent, les données de l'autre joueur sont
conservées. Les options du cœur et les identités de contenu restent identiques.

Une erreur de lecture, de validation ou d'écriture doit conserver la session
et ses données. Une ancienne réponse de sélecteur ne peut pas s'appliquer à un
autre jeu ou joueur. Si le cœur ne fournit pas de batterie pour cette session,
les actions de transfert sont désactivées.

Les batteries internes conservent leurs chemins historiques. La copie de
sécurité créée avant remplacement est privée à l'application ; elle ne remplace
pas un export personnel. Les états instantanés restent dans leur menu distinct.

## Vérification

Les tests natifs utilisent des programmes originaux GB, GBC et GBA et les moteurs
réels, en solo et en câble link. Les tests Android de `SaveTransfersInstrumentation`
utilisent un fournisseur de documents réservé aux tests pour vérifier les octets
exportés, les fichiers importés, les annulations et les résultats périmés.
Les tests de fichiers bornent la taille et vérifient le nettoyage des transferts
incomplets. Aucun jeu commercial n'est utilisé ou distribué par ces tests.

## Copies et états automatiques

Dans Réglages > Sauvegardes, conserver 1 à 3 copies des batteries précédant
un import réussi. Depuis Sauvegardes du jeu > Copies de secours, exporter
une copie disponible en `.sav`, puis l'importer pour la restaurer. Les
copies d'un joueur restent distinctes de celles de l'autre joueur. Une copie
absente ou illisible est désactivée dans cette liste.

L'état automatique, désactivé par défaut, est une préférence distincte : il
s'enregistre à la fermeture ou au remplacement d'un contenu sérialisable,
sans écraser l'état manuel. Son chargement reste explicite dans Sauvegardes
d'état. Voir le [guide des paramètres](settings-plan.md).
