Fermer un jeu doit garder le cœur selon demande utilisateur, même si RetroArch ferme ensuite parfois son instance : ne pas revendiquer une identité exacte de ce détail amont.
cr_engine_close reste le déchargement complet ; nouveau cr_engine_unload_game préserve handle/module/output/API/system et refuse sans mutation si sauvegarde impossible.
Les gardes sont dans le frontend, pas un changement du contrat bas niveau cr_engine_open : import SAF tardif doit être refusé via finish_import(false) si cœur absent ou incompatible, sans commit du contenu.
Le logo inline n'a pas de fichier original local accessible : préparation via imagegen. Détourage refusé après rendu dégradé ; version propre sur fond noir conservée pour launcher adaptatif.

Instrumentation Android : attendre hasWindowFocus et état natif active=1 après rotation ; préférences thème sont des String et audio des Boolean, utiliser activity.preference pour assertions. Attendre une vraie frame GB avant de chercher un popup afin de ne pas analyser une capture encore sur Historique.
