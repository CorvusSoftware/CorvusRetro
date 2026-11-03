# Constats de refonte

Les données d'origine n'avaient pas de jaquettes persistées, description/taille native des cœurs ou collection d'états nommés. Ces frontières sont étendues sans modifier les identités ni les chemins historiques.

Trois findings P2 résolus et revus indépendamment :
- Prévalider version et intégrité de l'état avant de remplacer une autre partie ; les refus version/corruption/absence conservent le handle, l'identité et la machine du jeu courant (test108contrôles).
- Identifier chaque saisie Android par génération C/JNI ; invalider mailbox et callbacks de l'ancien éditeur, même pour un nouveau champ de même numéro.
- Relire les états après un échec final de renommage/duplication/suppression/création, car le stockage peut avoir déjà appliqué l'opération avant un fsync échoué.

Le premier scénario Android a révélé une garde NativeActivity limitée aux champs RA : Entrée était absorbée par le menu pour recherche/renommage. La garde des touches et des axes inclut maintenant ui_editing. Une nouvelle exécution physique est en cours.

Les réglages d'affichage reprennent uniquement les fonctions effectives (natif/entier, filtre, écran allumé). Aucune fausse option de ratio forcé ou plein écran n'est affichée.
Les cœurs officiels actuels ne permettent pas les états en câble link ; l'interface expose ce refus. Les autres outils de liaison sont conservés.
