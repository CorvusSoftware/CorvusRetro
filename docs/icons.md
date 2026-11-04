# Icônes de l'interface

Le jeu comprend 49 pictogrammes originaux de CorvusSoftware. Les ressources
référencées dans `MainActivity.ICONS`, sous `res/drawable`, utilisent une grille 24 x 24,
un contour de 1,75 unité, des extrémités arrondies et des fonds transparents
légers. Le corbeau frontal reprend le modèle de l'[organisation CorvusSoftware](https://github.com/CorvusSoftware)
et apparaît aussi dans la puce des cœurs : front anguleux, yeux triangulaires,
plumes latérales et long bec. Les petits pictogrammes sont des déclinaisons
vectorielles. Le lanceur utilise le fichier PNG officiel de l'avatar, intact,
[récupéré sur GitHub](https://avatars.githubusercontent.com/u/346545957?v=4).

Le même dessin prend les couleurs du thème natif : violet et lavande pour
CorvusRetro, noir/blanc pour ASUS, vert pour NVIDIA Shield. Les proportions des
écrans et les cibles tactiles ne dépendent pas des dessins des icônes.

| Groupe | Pictogrammes |
| --- | --- |
| Navigation | Accueil, grille, bibliothèque, dossier, recherche, plus, retour, suivant, menu |
| Cœurs et contenu | Puce avec corbeau, corbeau, cartouche, fichier, sous-systèmes, déchargement |
| Réglages | Engrenage, palette, écran, vidéo, haut-parleur, volume, manette, toucher, compte, informations |
| Sauvegardes | Disquette, ajout, copies, restauration, crayon, duplication, partage, corbeille |
| Actions et suivi | Lecture, redémarrage, sortie, fermeture, téléchargement, validation, historique, cloche, trophée |
| Codes de triche | Chevrons et barre oblique sur une carte légère |

Renommer, dupliquer, partager et supprimer ont leurs propres symboles. La
création d'un état utilise un ajout, tandis que les états existants gardent la
disquette. Les menus C conservent les actions et identifiants existants.

## Correspondance des entrées

| Entrée ou action | Symbole visible |
| --- | --- |
| Accueil / Réglages | Maison / engrenage |
| Bibliothèque / présentation en liste | Grille / liste à puces |
| Cœur / décharger le cœur | Puce Corvus / éjection |
| Sous-système | Deux consoles reliées par un câble |
| Système / options du cœur | Équipement système / curseurs de réglage |
| Audio / volume | Haut-parleur / haut-parleur avec ondes |
| Filtrage de l'image | Pixels |
| Importer ou charger une sauvegarde | Fichier avec flèche entrante |
| Exporter en .sav ou .srm | Fichier avec flèche sortante |
| Sauvegarder / copies de secours | Disquette / disquettes superposées |
| Charger l'état automatique | Disquette avec horloge |
| Réinitialiser, redémarrer ou actualiser | Flèche circulaire |
| Identifiant / mot de passe | Personne / cadenas |
| Succès / notifications | Trophée / cloche |
| Codes de triche | Symbole de code `</>` |
| Renommer / dupliquer / partager / supprimer | Crayon / deux documents / partage / corbeille |

Les écrans vides utilisent le symbole de leur catégorie : grille pour les jeux,
disquette pour les sauvegardes, puce pour les cœurs et loupe pour une recherche
sans résultat. Les consoles et couvertures restent réservées aux jeux et aux
fiches de cœur concernés. Elles ne remplacent plus le bouton de déchargement.

## Contrat de l'atlas

`gfx/render.h` déclare les identifiants : les 48 identifiants existants restent stables et
`CR_ICON_CHEATS` est ajouté à l'indice 48. Leur ordre correspond au tableau `ICONS`
de `MainActivity`. Android rasterise les ressources ; les menus et leur rendu
restent en C11/OpenGL ES2.

L'atlas reste un bitmap 1024 x 2048, avec des cellules de 64 pixels et un inset
d'icône de 48 pixels. Les icônes occupent le bloc réservé 463..511, dont les 49
cellules sont utilisées. Les 233 caractères fixes sont conservés et
le cache Unicode dynamique utilise 233..462 avec éviction LRU. Le marqueur blanc
des primitives demeure hors de l'inset des icônes.

Une modification de cette allocation doit rester cohérente dans
`CR_ICON_ATLAS_BASE`, les tableaux/boucles de `gl2`, `ICON_ATLAS_BASE` côté Java
et l'instrumentation. Les gardes de compilation empêchent de dépasser le bloc
réservé ; la texture ne grossit pas avec ce nouveau jeu.

## Vérification visuelle

Construire l'APK et son runner avec
`-PcorvusTestRunner=com.corvussoftware.corvusretro.IconAtlasInstrumentation`,
puis lancer cette instrumentation sur Android. Elle utilise les ressources
réelles : formes distinctes et non vides, marges propres, lecture à 24 pixels,
marqueur blanc préservé et planche avec les trois palettes. Le PNG de contrôle
reste dans les fichiers privés du profil de test et hors du dépôt.

`InterfaceRedesignInstrumentation` vérifie ensuite les pictogrammes dans les
écrans réels, notamment accueil, réglages, fiches des cœurs et actions d'états.
