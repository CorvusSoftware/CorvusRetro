État : IN_PROGRESS pour la livraison de la ligne unique, BLOCKED pour le câble link.

Reprise sur main54cb1b7, prototype direct sauvegardé et sous-modules récursifs à jour.
Ligne unique Charger/Décharger implémentée sur CorvusAPI sans changer le téléchargeur.
Tests5/5 normaux et ASan/UBSan, test zéro cœur et catalogue5/5 réussis.
APK de base + lint construits, signature et alignement16Ko vérifiés ; seuls les deux
libcorvusretro.so sont présents. Pack explicite construit et vérifié : les trois
modules officiels présents pour les deux ABIs ; signature et alignement valides.
Revue indépendante : aucun finding concret. Android15 : base zéro cœur et téléchargeur,
pack trois cœurs, informations, portrait/paysage, jeu GB existant ouvert puis déchargé.
Les ANR de Quickstep/SystemUI ont été fermées avant les essais valides. Publication et CI à suivre.

Contrat du câble proposé dans docs/subsystems-corvusapi-proposal.md.
Pas d’entrée factice ni de contournement CorvusAPI. L’ABI publique manque réellement.
Avant implémentation du link : proposer l’extension dans CorvusAPI puis adapter CorvusCore
dans son dépôt ; les instructions interdisent de modifier ces dépendances ici.
