# Prototype du mercenaire : navigation et contrats de test

Le serveur contient deux modèles de PNJ réservés au prototype. Aucun PNJ existant
n'a été remplacé. Le premier teste la navigation à Orgrimmar ; le second gère les
contrats persistants et le combat exclusif, décrits dans la section « Version 2 ».
La version actuelle ajoute les primes, les courriers et le butin privé (section Version 3).
Les pistes automatiques de voyage et les vols restent à ajouter.

## Test en jeu

1. Relancer worldserver et connecter le personnage GM à Orgrimmar.
2. `.hunterproto spawn` : crée un seul mercenaire près de Gryshka, dans l'auberge.
3. Parler au mercenaire et choisir **Recevoir une piste : rejoindre le maître de vol**,
   ou saisir `.hunterproto alert`. Il rejoint la zone de Doras par un chemin calculé.
4. Il attend à l'arrivée. `.hunterproto rest`, ou **Retourner à l'auberge** dans son
   dialogue, lance le retour. Il s'assoit à l'arrivée.
5. `.hunterproto status` indique son état et le motif d'un éventuel arrêt.
6. `.hunterproto remove` retire le prototype. Il n'est pas recréé automatiquement au
   redémarrage du serveur ; utiliser spawn pour un nouvel essai.

Le dialogue propose **Marcher / Courir**. Changer l'allure lorsqu'il est à l'arrêt.
Les commandes exigent le même droit que `.gobject add`. Le menu est réservé à ces
comptes GM. Ce PNJ de test est passif et protégé contre les attaques.

## Choisir les deux emplacements

Les positions par défaut sont une place près de Gryshka et un point accessible sur
la plateforme de Doras. Le repos par défaut est une assise au sol.

Pour choisir un vrai banc :

1. `.hunterproto remove`.
2. S'asseoir avec son personnage sur le banc souhaité dans l'auberge.
3. `.hunterproto setrest` mémorise la position, l'orientation et le type d'assise.
4. Rejoindre à pied la place d'attente souhaitée près du maître de vol et saisir
   `.hunterproto setfly`.
5. `.hunterproto spawn`, puis refaire l'aller-retour.

Les points sont conservés dans `hunter_proto_point`, même après redémarrage.
Ils doivent rester à Orgrimmar. Le PNJ parcourt réellement le chemin avant de s'asseoir ;
seul un petit ajustement local peut aligner le personnage sur la place assise.

## Navigation et vérifications

- Départ et destination sont les seuls repères enregistrés : aucun circuit de waypoints
  intermédiaires n'est dessiné à la main.
- Les MMAP calculent les passages sur le terrain. Les chemins incomplets peuvent être
  poursuivis par portions. Les trajets à travers les obstacles, les raccourcis de secours
  et les mouvements sans navmesh sont refusés.
- Si le chemin n'est pas praticable, le PNJ s'arrête et explique qu'il ne trouve pas de
  passage. Il ne se téléporte pas au maître de vol.
- Le PNJ reste actif sans joueur à côté. Un seul prototype créé par la commande existe
  à la fois ; remove permet de le retirer.
- Le test hors serveur a vérifié un corridor de 48 polygones, environ 191 m, dans les
  deux sens. Le test serveur a ensuite fait parcourir l'aller-retour par le vrai PNJ,
  avec attente auprès du maître de vol et assise à l'auberge, puis arrêt propre.
- `.hunterproto selftest`, depuis la console ou un compte GM, permet de rejouer le
  test automatique en courant si aucun prototype n'est déjà présent.

## Construction et installation

Sources : `server/custom_hunter_navigation.cpp`. `InstallSources.py` copie le script
et enregistre son chargement en préservant les scripts existants. Reconfigurer CMake,
puis compiler worldserver en Release. `Prepare.py` crée uniquement le modèle de PNJ
réservé au prototype et la table de points. `Install.ps1` exige worldserver arrêté,
sauvegarde l'exécutable existant, copie le nouveau et vérifie son empreinte.

Le modèle de créature réservé est 9100100 ; il ne faut pas utiliser `.npc add` pour
ce test, mais `.hunterproto spawn`, qui gère l'unicité et le retrait du prototype.

## Version 2 : contrats de test et combat exclusif (étape précédente)

Cette étape a ajouté le modèle élite **9100101**, distinct du figurant de navigation
9100100. Le client et l'addon restent inchangés. Les essais initiaux étaient gratuits ;
la version 3 ci-dessous prélève désormais la prime et envoie les courriers.
Les pistes de voyage et les vols réels ne sont pas encore raccordés à ce mercenaire.

### Premier essai

1. Redémarrer worldserver après installation. Le client peut rester ouvert.
2. Depuis un compte GM : `.huntercontract contract NomDuJoueur`.
   Le joueur peut être hors ligne ; son contrat reste alors en réserve.
3. Si la cible est connectée, son mercenaire apparaît dans l'auberge d'Orgrimmar
   (point défini par `.hunterproto setrest`). Il ne rejoint pas magiquement la cible.
4. Amener la cible à proximité : le mercenaire repère sa cible dans un rayon de
   25 mètres si elle est visible et dans sa ligne de vue, puis engage le combat.
   La cible peut également commencer l'affrontement elle-même.
5. Avec une cible GM, désactiver le mode GM et l'invulnérabilité (`.gm off`,
   `.cheat god off`) pour tester réellement le combat. Ses protections normales
   empêchent le mercenaire de l'attaquer sinon.
6. Un troisième personnage doit être incapable de blesser le mercenaire, même
   avec un sort de zone ; le familier de la cible peut participer.
7. Déconnecter réellement la cible : le mercenaire est sauvegardé et retiré dans
   les deux secondes. Reconnecter : il est restauré à sa position sauvegardée,
   avec sa proportion de vie restante. Il ne reçoit pas les coordonnées du joueur.
8. Refaire le test après un arrêt propre de worldserver : le contrat est restauré.

Commandes GM (également disponibles dans la console sans point initial) :

- `.huntercontract contract Nom` : nouveau contrat de test ; refus si un contrat
  est déjà en cours sur cette cible, y compris si elle est déconnectée.
- `.huntercontract contracts` : liste les identifiants de cible, niveaux, présence
  ou réserve et issues (0=en cours, 1=victoire mercenaire, 2=victoire cible,
  3=annulation).
- `.huntercontract cancel GUID` : annule un contrat de test et retire son acteur.
- `.huntercontract selftest` : vérifie réellement sur deux créatures temporaires
  les protections d'attaque, dégâts directs et mort instantanée, et le modèle élite.

Le mercenaire prend le niveau de la cible à sa création et à sa restauration ;
il se réajuste hors combat si elle gagne un niveau. Ses statistiques proviennent
 des statistiques de créature du niveau concerné, avec un coefficient de vie de 2.
L'équilibrage en fonction des classes et équipements reste à tester en jeu.

### Persistance et limites

La table CharacterDatabase `hunter_proto_contract` conserve une ligne par cible :
commanditaire, coordonnées du mercenaire, niveau, proportion de vie et issue.
Sa clé primaire interdit plusieurs contrats simultanés pour le même joueur.
Une ligne terminée peut être réutilisée par un nouveau contrat ; ce prototype
ne constitue pas un historique financier. Les identifiants ne dépendent pas du nom.

Au maximum huit mercenaires sont présents simultanément ; les autres contrats
restent en réserve. Les positions et la vie sont enregistrées toutes les dix secondes,
à la suspension et à l'arrêt propre du serveur. Un arrêt brutal peut perdre le dernier
intervalle de sauvegarde. Les contrats en réserve n'ont pas de PNJ actif.

Seuls la cible et ses unités contrôlées peuvent endommager ou tuer le mercenaire.
Le mercenaire ne peut endommager qu'elles. La faction du joueur reste inchangée.
Le PNJ apparaît hostile à sa cible et amical aux autres joueurs. Ces derniers
peuvent lui parler hors combat et choisir « Qui traques-tu ? ». Il leur révèle
le nom actuel de sa cible en chuchotement. Les gardes ne peuvent pas le tuer.
Le lien de traque utilise une donnée privée de son IA, sans champ de créateur :
aucune mention « Serviteur de [cible] » n'est affichée. Les contrats déjà sauvegardés
conservent leur lien et leur état après la mise à jour.
Les protections normales des GM, les contrôles de visibilité, la mort et les immunités
restent applicables. Le joueur conserve ses relations normales avec le reste du monde.

La victoire termine le contrat et retire le mercenaire. Un autre monstre qui tue
la cible ne termine pas ce contrat. Aucun paiement n'est déclenché à cette étape.

### Vérifications de cette version

- `hunter_policy_test` vérifie les relations exclusives et l'absence d'effet sur
  les relations ordinaires (cibles différentes, propriétaires de familiers, acteurs
  sans cible et deux mercenaires).
- `tests/server_contracts.py` lance le vrai worldserver sur un port de test,
  contrôle les protections sur le vrai PNJ, crée un contrat temporaire hors ligne,
  refuse son doublon, redémarre le serveur, vérifie la restauration et l'annulation.
  Il retire ensuite uniquement la ligne temporaire qu'il a créée.
- Les combats avec deux vrais joueurs, les familiers et la déconnexion en combat
  nécessitent encore la validation en jeu.

## Version 3 : prime réservée, butin privé et courriers

Cette version est uniquement côté serveur. Exemple de création depuis le compte
commanditaire GM :

```
.huntercontract contract Testeur 250
```

Le montant est en pièces d'or entières (1 à 100000). Sans montant, la prime est
250 pièces d'or. Le commanditaire doit posséder la somme : même un MJ est débité.
Les commandes restent réservées aux comptes autorisés pour cette phase du prototype.
Une création payante depuis la console sans personnage est refusée.

La création enregistre dans une seule transaction le retrait d'or, le contrat,
son paiement réservé et le courrier d'alerte. La cible reçoit aussi un message
immédiat si elle est connectée. Elle peut lire la lettre dans une boîte aux lettres,
y compris après reconnexion. Aucun courrier supplémentaire n'est envoyé à chaque
connexion. Les contrats gratuits des versions précédentes restent gratuits ; on
ne prélève jamais rétroactivement leur commanditaire.

Le dialogue devient : « Je traque une personne nommee Testeur. »

### La cible gagne

- Le mercenaire laisse un corps contenant exactement la prime, sans butin annexe.
- Seule sa cible peut ouvrir et récupérer ce butin. Une victoire par son familier
  attribue le butin à son propriétaire. Le groupe et le maître du butin n'ont aucun
  droit sur le corps ; l'or n'est jamais partagé.
- Le montant est crédité et marqué comme payé dans une seule transaction.
  Un deuxième clic ou un paquet répété ne donne pas une deuxième prime.
- Après récupération de l'or, la fenêtre de butin se ferme et le corps disparaît.
- Si le joueur se déconnecte ou si le serveur redémarre avant récupération, le
  droit et la somme restent en base. Le corps est restauré à sa position de mort
  quand la cible est connectée, dans la limite de présence du prototype, sans
  rejouer un combat ni accorder à nouveau l'expérience de la mort.
- Si le joueur est à la limite d'or, la prime reste sur le corps au lieu d'être perdue.

Le commanditaire reçoit un courrier d'échec. La cible reçoit un courrier lui
indiquant qu'elle peut récupérer la prime. Cette lettre n'attache pas d'argent :
le paiement se fait exclusivement sur le corps.

### Le mercenaire gagne ou le contrat est annulé

Si le mercenaire tue sa cible, la prime est consommée et le commanditaire reçoit
un courrier de réussite. Il n'est pas remboursé.

Une annulation administrative avec `.huntercontract cancel GUID` rembourse la
prime par courrier au commanditaire, une seule fois. Les gardes et les autres
joueurs ne peuvent toujours pas tuer le mercenaire.

Pour cette version, une cible avec une prime gagnée mais pas encore récupérée
reste réservée : récupérer son butin avant de créer un nouveau contrat sur elle.
Les issues et états de paiement sont indiqués par `.huntercontract contracts`.

### Registre et vérification

`hunter_proto_payment` conserve chaque prime et son état : 0=réservée,
1=butin disponible, 2=récupérée, 3=consommée par le mercenaire, 4=remboursée.
Le contrat référence ce paiement par `payment_id`. Les migrations sont additives.
Les courriers suivent le fonctionnement normal de TrinityCore (expiration standard).

`tests/server_bounties.py` crée deux identités temporaires réservées, lance deux
fois un worldserver isolé, puis supprime uniquement leurs données. Il contrôle :

- les protections du vrai PNJ et un vrai corps contenant 250 pièces d'or ;
- le retrait de 250 pièces d'or, le courrier d'alerte et le refus d'un doublon ;
- le retour intégral à l'état initial après une transaction volontairement annulée,
  y compris l'absence du courrier contenu dans cette transaction ;
- la reprise d'une prime non récupérée après redémarrage ;
- le crédit unique de la prime, le refus d'un deuxième crédit et le remboursement
  unique à l'annulation ;
- le refus d'un nouveau contrat si le commanditaire n'a plus les fonds nécessaires.

`tests/server_contracts.py` redirige vers ce test pour éviter d'envoyer des lettres
de test à des personnages existants. La commande console `financialselftest 1/2`
requiert ces identités jetables précises et ne fonctionne pas depuis un joueur.
Les clics réels dans la boîte aux lettres et sur le corps avec deux joueurs restent
à vérifier en jeu, notamment avec un groupe et un familier.

## Version 4 : panneau permanent à l'hôtel des ventes

**La création passe maintenant par le panneau, sans commande dans le chat.**
L'ancienne sous-commande `huntercontract contract` a été retirée. Les commandes de
vérification et d'annulation restent des outils administratifs.

Un panneau en bois, nommé **Panneau des contrats**, se trouve devant l'entrée de
l'hôtel des ventes d'Orgrimmar, côté place. Il utilise un modèle de panneau déjà
présent dans le client (display 2491). Son template réservé est 9100102 et son
apparition est enregistrée dans `gameobject`, sur la carte 1. Il ne disparaît pas
après utilisation et réapparaît au chargement de la zone après un redémarrage.

1. Relancer worldserver et WoW après installation.
2. Vérifier que **Hunter Contracts** est activé dans la liste des addons.
3. S'approcher du panneau et faire un clic droit.
4. Renseigner **Nom de la cible** et **Prime en pièces d'or**, puis **Valider**.
5. La réponse apparaît dans la fenêtre. Si la cible n'existe pas, a déjà un contrat,
   ou si les fonds sont insuffisants, aucun nouveau prélèvement n'est effectué.

Le panneau est utilisable par les comptes joueurs ordinaires, sans droits MJ.
Le joueur doit être vivant, hors combat et à portée du panneau. Il peut viser un
joueur de la même faction. Les contrôles financiers, courriers, relations de combat
et butin privé de la version 3 s'appliquent sans changement.

L'interface est dans `addon/HunterContracts`, installée dans
`C:/Jeux/WoW335a/Interface/AddOns/HunterContracts`. Elle utilise la conversation
standard du panneau ; aucun message de commande dans le chat n'est envoyé.
Le serveur consomme chaque requête avant traitement et renouvelle l'identifiant
 de menu après sa réponse. Les anciens menus ne permettent pas de rejouer un paiement.

`PrepareBoard.py` installe le template, le texte et une seule apparition permanente,
sans écraser un déplacement ultérieur du panneau par un MJ. Les positions initiales
sont calibrées sur le sol mesuré dans les MMAP/VMAP. `Install.ps1` installe à présent
worldserver et les deux fichiers de l'interface, avec sauvegardes et contrôles SHA-256.
Il exige les deux programmes arrêtés et les laisse arrêtés.

Vérifications supplémentaires : `hunter_board_input_test` teste les données reçues
par le serveur ; `board_ui.lua` utilise le Lua 5.1 du client pour vérifier les champs,
les montants, les doubles clics, les réponses, les fermetures et les autres dialogues.
`server_bounties.py` charge le panneau et son vrai script deux fois, contrôle sa
hauteur sur le sol et rejoue les tests de transactions. Le clic réel depuis un compte
joueur reste à valider en jeu.

## Version 5 : pistes de vols et voyage du mercenaire

Un départ réellement accepté par un maître de vol publie une piste pour le contrat
actif de la cible : aéroport de départ et destination finale sélectionnée, même
avec des escales. Une sélection refusée et les vols déclenchés par un sort ne
publient rien. La piste ne contient aucune position courante du joueur.

Le mercenaire rejoint à pied son propre maître de vol, avec navigation MMAP,
monte une wyverne et suit les points DBC des lignes aériennes Horde. Il connaît
toutes les destinations Horde du continent, indépendamment des vols appris par
le joueur. Il calcule son itinéraire depuis sa propre position ; il peut arriver
après la cible. À destination, il attend une nouvelle piste et ne poursuit le
joueur directement que lorsqu'il le voit à proximité. Une nouvelle piste reçue en
vol est traitée à l'escale suivante, sans virage brusque en plein ciel.

La table CharacterDB `hunter_proto_pursuit` conserve la dernière piste et la
progression du vol avec la position du mercenaire, par contrat. Un redémarrage
reprend le voyage depuis le dernier point sauvegardé. Si la cible quitte le monde,
le mercenaire finit l'étape aérienne en cours puis est suspendu ; au sol il est
suspendu immédiatement. Son état reste enregistré pour la reconnexion. La limite
de huit mercenaires actifs est conservée.

Pour tester : créer un contrat via le panneau, puis faire prendre à la cible un
vol des Pitons-du-Tonnerre vers les Mille Pointes ou Gadgetzan. Le mercenaire part
de l'auberge d'Orgrimmar, rejoint Doras et prend son propre vol. Le panneau et
l'interface n'ont pas changé ; seul le serveur nécessite cette mise à jour.
Un MJ peut consulter `.huntercontract contracts` pour voir la phase du voyage et
les identifiants de la dernière piste.

Périmètre de cette version : vols Horde sur le même continent. Les bateaux,
zeppelins, changements de continent et le réseau Alliance restent à ajouter.
Un itinéraire indisponible bloque le voyage en conservant le contrat et sa prime ;
une nouvelle piste permet une nouvelle tentative. Aucun saut instantané vers la
cible n'est utilisé.

`hunter_travel_events_test` vérifie les abonnements, les dernières pistes et le
rejet des événements d'un ancien contrat. `tests/server_taxi.py` lance un serveur
isolé : marche réelle auberge–Doras, vol DBC, reprise en vol, arrivée, piste
persistante après redémarrage et régression des transactions financières. Les
comptes de test sont jetables ; les données des joueurs existants sont préservées.
