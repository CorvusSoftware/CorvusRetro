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
sans verrou ni allocation dans le callback. Le démarrage attend deux blocs
préremplis. Les dépassements jettent des nouvelles trames complètes, sans
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
le cœur et efface les entrées. La cadence est recalée après une trame lente pour
éviter une rafale de rattrapage après une interruption.

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
