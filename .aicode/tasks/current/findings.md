Le refspec cloud ne met à jour que origin/HEAD. Pour contrôler main explicitement :
git fetch origin refs/heads/main:refs/remotes/origin/main.
Un simple git fetch origin main laisse origin/main potentiellement obsolète.

main54cb1b7 conserve les évolutions0.9 et migre vers CorvusAPI0 :
registry dynamique, identités stables, base sans modules et pack explicite.
Le prototype interrompu est sauvegardé dans un stash nommé
resume backup: native subsystem work before CorvusAPI migration.
Ne pas réappliquer engine.c ou les tests directs de ce prototype.

CorvusCore8433980 inclut CorvusAPI188a1a5. corvus_core_api expose load_game,
un seul callback vidéo et aucun sous-système. Les adaptateurs officiels annoncent
max_players1 et ignorent player!=0. Le SDK interne sait relier les consoles, mais
son usage direct dans le frontend violerait la frontière publique actuelle.

La ligne cœur suit engine.handle, et non engine.loaded : le chargement sans jeu
doit aussi afficher Décharger. Un jeu ajoute seulement Menu rapide avant la ligne.
Test ROM GBA originale : vraie écriture SRAM, obstruction saves, échec de
déchargement sans perdre la session, suppression de l’obstruction et nouvelle tentative.
