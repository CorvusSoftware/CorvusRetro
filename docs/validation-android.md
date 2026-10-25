# Validation Android

## Version 0.4

Vérifications exécutées sur le code de la version 0.4 :

- Tests Java/JNI des trois vrais moteurs CorvusCore 0.14.0 avec `-Xcheck:jni`.
- Création sans contenu, refus d'opérations sur un cœur vide, chargement du contenu, vidéo ARGB, audio PCM, entrées, états et reset.
- Libération du handle, refus d'exécuter une frame après fermeture et rechargement du même moteur.
- Batterie GBA, buffers invalides et rejet du contenu vide.
- Construction APK ARM64/x86_64 et lint Android sans erreur.
- Revue indépendante de la propriété du moteur et des callbacks ; correction du snapshot d'état chargé pendant le rendu du menu.
- Signature APK, alignements ZIP et ELF 16 Ko vérifiés sur les deux architectures.
- Anciennes classes et symboles JNI absents de l'APK issu d'une construction propre ; fichiers source, entrée de menu et activité correspondants retirés.
- Politique de ponctuation exécutée sur les fichiers du dépôt parent ; sous-module CorvusCore inchangé.

Les essais Android utilisent un émulateur AOSP Android 10 x86_64 sans KVM,
480 × 960 pixels. Les contenus synthétiques proviennent de
`python tests/generate_roms.py` : GB modifie une palette à chaque pression A,
GBC exécute la variante couleur et GBA écrit `0x5a` en SRAM.
Aucun contenu commercial ou BIOS constructeur n'est nécessaire.

Scénarios exécutés sur Android pour la version 0.4 :

- Démarrage avec Aucun cœur ; chargement GB, remplacement par GBC, déchargement, chargement et déchargement GBA.
- Choix du thème sombre bleu puis Shield avec un cœur GBC chargé : état chargé conservé pendant les recréations d'activité.
- Lancement de GB depuis l'historique sans cœur chargé ; effet de A sur les pixels et restauration d'état avec comparaison d'image.
- Un second menu refuse de décharger le cœur encore possédé par le lecteur ; retour au lecteur avec image inchangée.
- Fermeture du contenu : retour à Aucun cœur ; rechargement du moteur et de l'état disque avec pixels identiques.
- GBA : suppression ciblée de la batterie de notre fixture avant lancement, puis vérification d'une nouvelle écriture disque `0x5a` avant libération du cœur.
- Listes et historique conservés après mise à jour depuis 0.3 ; aucun crash dans le buffer Android dédié.

Les captures du README sont issues de cette exécution de la version 0.4.

## Contrôles antérieurs du lecteur

La version 0.3 avait validé sur le même émulateur : import SAF GB/GBC/GBA,
commandes A avec effet réel sur les pixels, sauvegarde sur disque et restauration
d'état avec comparaison des images, reset GBC, batterie GBA lue sur disque,
arrière-plan/reprise, rotation, listes, historique, crédits, import invalide sans
fichier résiduel et commandes visibles sur une largeur de 320 dp.
Ces preuves restent identifiées comme antérieures à la modification du cycle de vie.

## Reproduire

```sh
python tests/generate_roms.py
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb push build/test-roms/. /sdcard/Download/
adb shell am start -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n com.corvussoftware.corvusretro/.MainActivity
```

Vérifier Aucun cœur au démarrage, charger chaque moteur, le remplacer puis le
décharger. Changer le thème dans les réglages avec un cœur chargé : il doit rester
chargé. Ouvrir GB, appuyer sur A, sauvegarder, appuyer de nouveau puis charger
l'état : la palette précédente doit revenir. Fermer le contenu : Aucun cœur doit
réapparaître. Ouvrir GBA et vérifier sa batterie, puis tenter le fichier invalide.
Vérifier aussi l'import, la rotation et l'arrière-plan.

## Limites

Les tests synthétiques ne prouvent pas la compatibilité commerciale ni les
performances sur matériel mobile. GBA reste expérimental. Aucun téléphone physique,
appareil ARM64 ou Android récent n'a été exécuté. La qualité audio à l'écoute,
TalkBack, les manettes physiques et le multitouch simultané restent à vérifier.
