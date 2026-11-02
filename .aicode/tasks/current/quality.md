CorvusSoftware Quality Gate
Status: PASS (implémentation locale pour revue)
Risk: STANDARD
Criteria:
- PASS : MP3 utilisateur conservé octet pour octet, décodé et préchargé dans Android.
- PASS : seul le vrai événement de déblocage joue le son, selon cartes/audio/volume ; pas de doublon lancement/progression/collection.
- PASS : volume courant/mute propagés, pause/destruction arrêtent et libèrent le lecteur ; pas de reprise d'un ancien son.
Evidence:
-15suites natives et15ASan/UBSan réussies, test événementRAM/rc_client/volume/cartes ajouté au client natif.
- Androiddebug2ABI/lint/testAPK ; APKzeroCore/ELF16Ko ; hashassetAPK identique.
-26contrôlesAndroidSoundPool/décodage/volume/lifecycle et parcours succès natif/popups réussis. Le fixture son impose audio100 pour ne pas dépendre du profil.
Review:
- Indépendante sans défaut restant, correction du volume pendant lecture validée.
Open risks:
- Pas d'écoute sur téléphone physique. Aucun changement de moteur ni de compte réel ; protocole synthétique et jeu original de test.
Decision:
- PR prête pour revue ; consulter CI derniercommit avant fusion. Ni fusion ni release.
