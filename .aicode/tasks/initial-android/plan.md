# Première base Android

Objectif : application maison C11 + Java/JNI, signature CorvusSoftware, AGPL-3.0-only.
Périmètre : démo interactive native, commandes tactiles, pause/reset, licence, build reproductible.
Pas de code RetroArch repris ; pas de moteur d'émulation à cette étape.
Plan : noyau portable → interface Android → tests natifs → APK/lint → review.
P0 : déplacement, limites, pause/reset, protection du framebuffer via make test et sanitizers.
P1 : compilation JNI et APK arm64-v8a/x86_64 ; lint Android.
P2 : interaction/lifecycle sur appareil si disponible.
Risque : frontière JNI, lifecycle Android, absence éventuelle d'appareil.
