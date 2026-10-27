RetroArch : le badge est seul pendant la descente. Le panneau texte se déploie ensuite, le badge se déplace de la moitié de la largeur révélée afin que le total reste centré. Le texte glisse derrière un clip horizontal.
Chaque transition dure330ms, maintien4s, durée totale5.32s. Fermeture horizontale puis remontée verticale.
Le premier dt peut inclure le chargement du jeu : ne pas le compter dans une notification encore jamais présentée.
Le painter GL possède déjà le scissor nécessaire. Le caller menu rétablit le clip complet après les cartes animées.
