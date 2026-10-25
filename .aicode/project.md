# CorvusRetro

Android d'abord, application originale Java/JNI avec signature AGPL-3.0-only dans AGENTS.md.
Version 0.4 : listes MaterialUI, menus de chargement et déchargement réels, icônes originales avec tête de corbeau.
Thème NVIDIA Shield par défaut (palette étudiée dans MaterialUI), thème sombre bleu sélectionnable et persistant.
CoreManager : propriétaire de handle idle, transfert exclusif au lecteur, un HandlerThread FIFO pour moteurs et sauvegardes.
EmulationSession : rendu ARGB sur UI, PCM16 AudioTrack, commandes, pause Android, états et batterie AtomicFile avec acquittement.
Fermer le contenu libère le cœur et retourne à Aucun cœur ; le choix chargé est un état de processus, pas une préférence simulée.
CorvusCore 0.14.0 figé au commit 38829d4935bbdcf4b13261714352362065b78551, API C version 0 préliminaire. Sources du sous-module inchangées.
Notices embarquées avec crédits conservés ; tirets longs remplacés par ASCII dans leur ponctuation. Aucun code ou graphisme RetroArch copié.
Validation : make test (alias core-jni-test), assembleDebug et lintDebug ; commande CMAKE explicite si absent du PATH.
Pins : Gradle 8.13, AGP 8.9.1, SDK 35, min 26, NDK 27.2.12479018, CMake 3.22.1, Java17/JDK21. ABIs ARM64 et x86_64.
Rotation normale préserve le lecteur. Preuves Android dans docs/validation-android.md ; aucun téléphone physique testé.
Les tirets Unicode longs sont interdits dans le dépôt parent et contrôlés par la CI.
