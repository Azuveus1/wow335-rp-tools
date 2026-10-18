# Architecture et périmètre

## Répartition

Le **client C++** apporte les aperçus 3D, le rendu des gizmos et repères, les fonctions Lua d'édition, les noms complets et le rendu des tabards familiaux. Les **addons Lua** présentent les formulaires et envoient les requêtes. Le **serveur** décide des permissions, positions enregistrées, contrats, changements de noms, équipements et paiements. Les tables SQL gardent l'état persistant.

Les contrôles côté interface sont pratiques, mais le serveur reste responsable de la validation. Les formulaires du registre et du panneau utilisent des sessions ou identifiants de requête ; les modifications financières et d'identité passent par des transactions. La revue du code de ces chemins reste nécessaire avant un déploiement public.

## Points d'entrée à lire

- `WoW/Source/Ui/GMBlueprint.*`, `GMTransport.*` dans l'overlay client : rendu et API Lua.
- `Tools/GMBlueprint/shared/GizmoMath.h` : mathématiques de manipulation et rotations.
- `Tools/GMTransport/shared/*`, `server/GMTransportRoute.*` : calcul de parcours et intégration des transports. `GMTransportAnnouncements.inc` gère les annonces.
- `Tools/HunterPrototype/server/custom_hunter_navigation.cpp`, `HunterContracts.inc`, `HunterTaxi.h`, `HunterCombat.h` : états du mercenaire, contrats et relations de combat.
- `Tools/RoleplayNames/NameParts.h`, `DialogueNames.h`, `CommandPlayerNames.h` : séparation des noms et lecture des marqueurs/commandes.
- `Tools/CivilRegistry/civil_registry.cpp`, `RegistryPolicy.h`, `BirthNames.h`, `WeddingSlot.inc` : registre, famille, réservations de noms et alliance cosmétique.
- `Tools/RPGreetings/rp_greetings.cpp` : réception des emotes et réponse des PNJ.

Les fichiers de `overlays/server/` sont les copies réellement intégrées au core, parfois renommées pour leurs includes. Ils font référence pour cette version consolidée. Les copies dans `Tools/` servent à l'étude, aux tests et aux anciens scripts d'intégration.

## Données

- GMTransport : tables de parcours et d'annonces de `Tools/GMTransport/server/schema.sql`, en base world. Les identifiants stables des nœuds et révisions permettent de retrouver un trajet et ses annonces.
- Hunter : tables `hunter_proto_*` dans world et characters ; contrat unique par cible, prime et reprise de la poursuite persistantes. Le prototype limite les acteurs actifs.
- Registre : tables `rp_registry_*`, `rp_family`, `rp_family_member` dans characters. Les objets possèdent des GUID réels, la gravure et le port cosmétique sont suivis séparément.
- Identités : le nom complet actuel et le nom de naissance sont protégés. Le nom de naissance reste réservé à son propriétaire après mariage, renommage ou suppression. Le prénom seul peut être partagé.

Les scripts `Prepare*.py` décrivent les schémas et les insertions initiales. Ils ne constituent pas un export d'une base de joueurs.

## Limites actuelles

- Le chasseur prend les vols **Horde sur un même continent**. Les traversées en bateau/zeppelin et le réseau Alliance ne sont pas implémentés. Une piste est une destination de vol connue, pas une position du joueur suivie en permanence.
- Les personnages doivent respecter les contraintes du client 3.3.5a : deux parties, 12 lettres maximum chacune côté serveur et limites UTF-8 des paquets. Les noms composés de plusieurs mots ne sont pas généralisés.
- Le mariage fonctionne, mais **la procédure de divorce n'est pas encore implémentée**. La conservation du nom de naissance prépare cette évolution.
- Le tabard familial et le tabard de guilde partagent l'emplacement de tabard du jeu. L'alliance utilise un emplacement cosmétique géré par l'extension ; elle n'ajoute pas un nouveau champ d'équipement natif aux paquets standards.
- Le type d'un GameObject et ses propriétés peuvent empêcher le rendu serveur d'un modèle visible en aperçu local. Le catalogue ne garantit pas que chaque candidat soit un décor statique utilisable.
- Le serveur de référence comporte une petite correction de compilation MSVC dans `MapDefines.h`. Les scripts personnels antérieurs sans rapport avec ces outils ne sont pas inclus.
- Cette version est un ensemble d'outils et de prototypes pour essais. Elle n'a pas fait l'objet d'un audit de sécurité ou de charge pour un serveur public.
