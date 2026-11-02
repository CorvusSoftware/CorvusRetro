CorvusSoftware Test Matrix
Risk: DEEP

P0 : native_cheevos avec rcheevos réel et mémoire synthétique : cycle5->0->5 pendant240frames sans réapparition après expiration, entrelacement de deux succès, vrais changements, reset/restore/reload et son uniquement au déblocage. Régression constatée sur base avant correction.
P0 : native_audio avec pilote réel/fauxAAudio : calendrier GBA59.727/écran60Hz, comparaison nombre de silences avant/après sur60s, fréquences/volumes/stéréo/pleineéchelle/amorce et cycle de vie. Aucun traitement PCM inventé.
P1 : suites strictes et ASan/UBSan, cœur absent, Gradle debug/lint/JVM/testAPK pour les deux ABI et contrôle de paquet.
P1 : Android API29 debug sur programme original : 3 réactivationsA identiques après expiration sanspopup, nouvellevaleurSelect visible, B débloque avecunseulson ; contrôles depause/reprise existants.
P1 : revue indépendante, CI exacte de PR et qualification documentée.
Limites : pas de ROM Pokémon disponible, ni écoute physique/Bluetooth/runtimeARM64. Le test de calendrier et l'intégrité numérique PCM ne prouvent pas à eux seuls la qualité perçue sur le téléphone du mainteneur.
