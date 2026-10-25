# Migration CorvusAPI (branche feature/corvusapi-frontend)

Copyright (C) 2026 CorvusSoftware. SPDX-License-Identifier: AGPL-3.0-only.

CorvusRetro charge les modules internes GB/GBC/GBA avec dlopen/dlsym et ne lie
plus directement son moteur d'execution a corvuscore_c_api. Chaque module
implante l'ABI MIT CorvusAPI 0, fournie comme sous-module de CorvusCore.

Les nouveaux modules sont embarques dans l'APK Android ; **aucun code
telecharge depuis Internet** n'est charge. Les trois cœurs connus restent
selectionnables. Le gestionnaire de sauvegardes conserve les chemins, noms de
fichiers, format des etats et l'acquittement atomique de la batterie.

Le frontend conserve un contexte de callbacks alloue sur le tas afin que la
migration atomique d'une instance provisoire vers l'instance active ne rende
pas les pointeurs de callbacks invalides.

Cette branche depend du commit CorvusCore 7ec23d0cbbdf032933fb243beb4f460cd08ed96d, inclus dans son
sous-module. Les tests natifs, sanitizers et l'APK doivent etre verifies
avant fusion. CorvusAPI ABI 0 est toujours experimentale.
