Objectif confirmé : téléchargement et installation individuelle CorvusGB/CorvusGBC/CorvusGBA comme RetroArch, base sans moteurs, ajouts cumulables.
Budget DEEP : build dynamic-feature, installation Android et registre/identités sont des frontières distinctes.

1. Prototyper trois APK split Android installables avec MODE_INHERIT_EXISTING, même package/signature/version que base0.11/code12. Pas de chargement de .so depuis le stockage modifiable.
2. Découvrir les bibliothèques installées par Android dans nativeLibraryDir, CorvusAPI0 inchangée. Préserver GB/GBC/GBA suffixes0/1/2 même quand GBA est le seul cœur installé.
3. Nouveau flux cores-v2.json (format2, entriescores) compatible avec ancien index.json0.10 ; valider chaque APKsplit et le certificat avant installation.
4. Tests natifs/sanitizers, outils/catalogue, build/lint APK, zéro/GBAseul/ajoutGB/retour sans perte, portrait/paysage et version/signature invalides.
5. Revue indépendante, CI et livrables signables/publish workflow. Ne pas annoncer de modules téléchargeables tant que leurs vrais APK signés ne sont pas publiés et catalogués.

Le sous-système câble link reste à traiter via sa proposition CorvusAPI, indépendamment de cette distribution.
