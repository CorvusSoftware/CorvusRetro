# Quality gate

PARTIAL pendant la qualification Android et la CI frontend.

Contrat et fournisseur : tests/CI exacts verts dans les PR API7/Core13.
Frontend : tests natifs, sanitizers, build/lint/JVM, APK et revue indépendants
vérifiés. Le finding sur le collage a été corrigé et couvert en JVM.

Ne pas annoncer la fonctionnalité disponible dans les cœurs du catalogue actuel :
les nouveaux modules restent à qualifier/publier par CorvusRepository après les
PR dépendantes. APK de base toujours sans cœur. Aucun test physique de codes.
