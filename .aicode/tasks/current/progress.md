État : IN_PROGRESS, implémentation0.11 vérifiée localement, publication et catalogue en cours.

L’utilisateur a explicitement confirmé Cœur par cœur comme RetroArch. Sourceeb34629.
Base0.11.0/code12 sans moteur, trois featureAPKs coregb/coregbc/coregba même certificat/version.
Installation cumulative Android15 prouvée : GBA seul puisGB, ancienne batterieSHA-2inchangée et jeux GB existants conservés.
SixCTestsnormaux+ASan et zéro cœur passent ;15testsPython passent ; build base/features/androidTest et lint passent.
Instrumentation37contrôles, recréationActivity en attente et mortprocess avecsession persistante validées.
Revue indépendante : P1barrière recréation corrigée parCoreInstallState+getterJNI+syncresume ; aucunfindingrestant.
ConfirmationOSdifféréeaprèsbackgroundcorrigée; testUIDapp appliqué avecsuccès et état réconciliéauprochainlaunch (callbackpeutêtreperdulorsselfupdate).

Nouveaufeedformat2 cores-v2.json vide jusqu’aux vraisAPK signés et publiés. Legacyindex.json0.10conservé.
Preview signé automatique aprèsCIAndroid mainpush, publisher par tag v0.11.* réutilise mêmes APK et crée branchecatalogue candidate, pas d’autopromotionmain.
Prochainesétapes : commit/push source, CIexacte+previewsigné, tagprereleasev0.11.0, télécharger4assets réels, migrationrelease0.10→base0.11+cores sursecondeAVD29, revuecatalogue puispublicationmain.
AVD35TCP5557 agentAndroid ; secondeAVD29TCP5559 contientancienneAPKrelease11 déjàsignée, jeuGBAoriginal/historique/thèmeASUS préparés. CertificatpublicancrédepuisAPKinstallée:/workspace/setup/trusted-installed-release-cert.sha256 ; aucunecléprivéelue.
Câblelinktoujoursindépendant : propositionCorvusAPIavantadaptateur, pasimplémentédanscetâche.
