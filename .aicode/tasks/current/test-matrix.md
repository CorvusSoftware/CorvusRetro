CorvusSoftware Test Matrix
Risk: DEEP

P0 : native_audio (pilote réel avec faux AAudio) : FIFO stéréo, ordre producteur/consommateur, silence, volume, overflow, amortissement des erreurs, reprise, fermeture et callbacks tardifs.
P0 : native_core_removal : parcours confirmation/annulation, accès Informations pendant jeu, suppression autre cœur, refus si transfert/import, sauvegarde batterie en échec conserve moteur, fermeture avant suppression.
P0 : native_registry : zéro cœur, slots stables après suppression/réinstallation, contenu absent conservé sans réaffectation, moteur actif préservé.
P0 : CoreStoreTest : fichiers propriétaires seulement, versions/historique bornés, fichiers partagés, traversal/symlink, échec commit, absence de blocage du menu.
P0 : native_controls/media : croix portrait abaissée avec bornes et séparation, autres boutons/vidéo/paysage conservés.
P1 : 17 suites host strictes et ASan/UBSan, frontend sans cœur, Gradle JVM/lint/build deux ABI, signature/alignement APK zéro cœur.
P1 : émulateur Android15 x86_64 : volet notifications (focus perdu mais activité reprise, frames/AAudio continuent), HOME pause et retour EGL/audio, suppression via menus puis réinstallation avec ROM/history/batterie conservés, captures portrait/paysage.
P1 : revue indépendante, correction de chaque finding avéré, CI sur commit exact de PR.
Limites : pas de téléphone physique ni d'écoute sur émulateur sans sortie audio ; politiques audio Android/appel restent gérées par l'OS. La réinitialisation après erreur AAudio est injectée dans le test pilote host ; HOME/rotation exercent la surface Android réelle.
