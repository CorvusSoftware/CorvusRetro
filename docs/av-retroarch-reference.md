# Audio, vidéo et gestion des cœurs

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

Référence consultée : RetroArch, révision
[`901ebff805dba264bc418d55e95ef743d7e80135`](https://github.com/libretro/RetroArch/tree/901ebff805dba264bc418d55e95ef743d7e80135).
Les principes ci-dessous sont réimplémentés pour CorvusAPI et le frontend C11.
Aucun code ni dessin de RetroArch n'est incorporé.

## Audio

Le [pilote AAudio](https://github.com/libretro/RetroArch/blob/901ebff805dba264bc418d55e95ef743d7e80135/audio/drivers/aaudio.c#L207)
sépare le callback temps réel et la réinitialisation du flux. Il emploie une
file avec un producteur et un consommateur, complète les manques par du silence
et vise un tampon matériel de deux blocs (`framesPerBurst`, ligne 769).

CorvusRetro utilise une file stéréo bornée avec compteurs atomiques 32 bits,
sans verrou ni allocation dans le callback. Le tampon matériel vise deux blocs. Avant le démarrage, la file logicielle
attend aussi deux lots PCM livrés par le cœur (budget de deux fois le plus grand
lot reçu moins une trame, borné par la capacité de la file). Cela accepte les
lots GBA alternant 548 et 549 trames stéréo sans attendre un troisième lot.
Cette amorce absorbe l'intervalle d'affichage plus long quand la cadence GBA
59,727 Hz est présentée sur un écran 60 Hz. Les dépassements jettent des nouvelles trames complètes, sans
modifier le curseur du consommateur. Une sous-alimentation détectée par AAudio
peut agrandir progressivement le tampon matériel, dans sa capacité et au plus
jusqu'à huit blocs. Une erreur déclenche la fermeture et une nouvelle tentative
sur le thread du frontend, avec un délai de 250 ms. La pause ferme le flux avant
la remise à zéro des compteurs. Les callbacks d'erreur portent un identifiant
de flux et ne touchent qu'un état atomique persistant ; un ancien callback
ne peut ni accéder à un propriétaire détruit ni invalider son remplaçant.

## Cycle Android et vidéo

Les [événements Android de RetroArch](https://github.com/libretro/RetroArch/blob/901ebff805dba264bc418d55e95ef743d7e80135/input/drivers/android_input.c#L1023)
et son [état de focus](https://github.com/libretro/RetroArch/blob/901ebff805dba264bc418d55e95ef743d7e80135/input/drivers/android_focus.h#L64)
distinguent plusieurs états de l'activité et des fenêtres. RetroArch peut
suspendre son exécution lorsque le focus est perdu. La politique demandée pour
CorvusRetro est adaptée : perdre seulement le focus d'entrée, par exemple avec
le volet de notifications, efface les touches maintenues sans suspendre un jeu
encore visible et repris. Une véritable pause Android ou la destruction de la
surface conserve la pause audio et la sauvegarde batterie.

Après un échec EGL, CorvusRetro réessaie de créer le contexte et la surface toutes
les 500 ms lorsque la fenêtre existe et que l'activité est reprise. Il conserve
le cœur et efface les entrées. Le rattrapage reste limité à trois trames émulées
avant chaque rendu, puis une accumulation excessive de retard recale l'échéance.

Le système Android peut encore atténuer ou couper l'audio selon sa politique,
notamment pour un appel. L'application ne demande pas de modifier le mode
Ne pas déranger et ne bloque pas les notifications du téléphone.

## Suppression des cœurs

Le [menu de RetroArch](https://github.com/libretro/RetroArch/blob/901ebff805dba264bc418d55e95ef743d7e80135/menu/cbs/menu_cbs_ok.c#L9387)
propose une action explicite de suppression du cœur.

Dans CorvusRetro, ouvrir **Charger un cœur** ou **Informations**, puis
**Supprimer un cœur téléchargé**. Choisir un cœur et confirmer sa suppression.
L'entrée Informations permet aussi cette opération lorsqu'un cœur est chargé.
Si le cœur concerné est utilisé, sa sauvegarde et sa fermeture doivent réussir
avant la suppression. Un autre cœur en cours d'utilisation reste chargé.

L'opération retire les métadonnées et les fichiers privés dont la propriété est
enregistrée, sans effacer les ROMs, l'historique, les sauvegardes ou les états.
Les fichiers partagés avec un autre cœur et les anciens fichiers sans propriété
fiable sont conservés. Un échec de nettoyage est signalé. Les anciens modules
installés par split APK ne sont pas supprimés par cette action.

Le registre conserve les indices des autres cœurs et réassocie la bibliothèque
par identifiant permanent après la réception du nouvel instantané. Réinstaller
un cœur permet de retrouver les contenus déjà importés.

## Contrôle numérique du son, préparation 0.22.1

Le pilote précédent pouvait commencer dès le premier lot GBA : 548 trames PCM
pour un seuil matériel de 384 trames (deux blocs de 192). La simulation exacte
GBA/écran 60 Hz produit alors 10 callbacks partiellement vides et 733 trames de
silence sur 60 s, sans augmentation du compteur de sous-alimentation matériel.
Un callback AAudio peut donc fonctionner normalement tout en lisant une file
logicielle vide.

L'amorce attend maintenant le deuxième lot PCM. Sur les cinq simulations de
60 s avec blocs matériels de 64, 128, 192, 256 et 512 trames, aucune trame de
silence n'est ajoutée. Un motif stéréo non identique couvre toute la plage
16 bits et vérifie l'ordre exact à travers les tours de la file. Le gain est
comparé à une référence 64 bits pour plusieurs volumes, sans saturation.
Le tampon matériel conserve sa taille initiale de deux blocs ; le démarrage
attend un lot supplémentaire dans les cas qui commençaient trop tôt.

Ce contrôle ne prouve pas la qualité perçue sur le téléphone du mainteneur.
La ROM Pokémon Émeraude n'était pas disponible et aucune écoute physique ou
campagne Bluetooth n'a été réalisée. Il prouve la suppression d'une cause
mesurée de silences parasites dans le frontend, avec un PCM inchangé.

## Dérive des horloges et écritures pendant le jeu

L'amorce de 0.22.1 absorbe la différence de présentation GBA/60 Hz, mais une
horloge audio légèrement plus rapide peut encore vider progressivement la file.
Le test à +100 ppm reproduit 1 597 trames stéréo de silence sur 600 s avec la
cadence fixe précédente. Le compteur matériel de sous-alimentation ne détecte
pas forcément ce silence ajouté par le callback logiciel.

Le frontend ajuste maintenant les échéances d'émulation à partir de l'occupation
lissée de la file audio. La cible est l'amorce plus un demi-lot PCM, plafonnée à
75 % de la capacité. Un filtre de 0,5 s évite de suivre chaque bloc matériel ;
la correction proportionnelle reste bornée à +/-0,5 %. Chaque trame garde les
cycles déclarés par le cœur, ses entrées et son évaluation RetroAchievements.
La cadence réelle peut donc varier légèrement pour suivre l'horloge audio,
sans modifier les échantillons ni rééchantillonner le PCM. Une pause ou un
changement de cadence ou de fréquence audio réinitialise le contrôleur.

Les sauvegardes batterie périodiques, toutes les cinq secondes lorsque le jeu
avance, écrivent désormais sur un travailleur dédié. Le frontend copie au plus
deux batteries, puis continue à émuler pendant l'écriture atomique et les `fsync`.
Un seul instantané est en vol ; un acquittement est transmis au cœur uniquement
sur le thread propriétaire et après durabilité de tous les joueurs. Une génération
plus récente reste sale. Fermeture, pause, remplacement, importation et
restauration attendent les écritures concernées ; un échec de sauvegarde empêche
toujours une fermeture ou un remplacement destructif. Les formats et chemins
des sauvegardes ne changent pas.

Les régressions hôtes utilisent le pilote AAudio réel avec des callbacks simulés :
38 scénarios contrôlés sur 600 ou 3 600 s, affichages 60/90/120 Hz et blocs
64/128/192/256/512 trames. Le PCM stéréo reste exact, sans silence ajouté ni
débordement pour ces scénarios. Des coûts CPU de 6, 10 et 15 ms par trame passent
aussi avec des swaps d'affichage bloquants simulés. Une émulation à 25 ms par
trame dépasse le temps disponible et produit du silence, comme attendu.
Les tests de sauvegarde bloquent volontairement `fsync` pendant que le cœur GBA
continue à avancer, puis couvrent échecs, nouvelles générations et câble link.

Ces tests isolent deux sources possibles de parasites du frontend. Ils ne
reproduisent pas une écoute de Pokémon Émeraude sur le Redmi Note 13 5G et son
casque filaire. Des retards Android ou un cœur trop lent peuvent encore épuiser
la réserve audio ; aucun résultat d'écoute physique n'est revendiqué.
