RetroArch est une référence d'organisation et de comportement, pas une source à recopier.
Référence examinée : dda31fef21aeb472be9de7ee53eb386cc8db1417, cheevos/cheevos.c et callbacks rc_client.
rcheevos officiel MIT assure protocole, hash et évaluation ; code frontend original AGPL.
L'extension facultative corvus_get_achievements_api est dans le header public unique corvusapi.h, ABI0 inchangée.
Tous les accès au client et à la RAM appartiennent au thread frontend ; HTTPS/crypto/disque appartiennent aux services.
Les succès doivent survivre à la fermeture du jeu ; file chiffrée persistée avant envoi et invalidée à la déconnexion.
Les lectures du frontend ne doivent pas attendre les verrous de chiffrement ou les écritures disque.
Le targetSDK35 accepte dlopen des modules privés vérifiés en lecture seule, prouvé sur Android15 ; aucune baisse de cible.
Conserver les identités et sauvegardes historiques, les anciens catalogues et le signataire durable de la base.
