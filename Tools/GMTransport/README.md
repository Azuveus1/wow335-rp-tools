# Éditeur et bibliothèque de transports GM

Client wowdecomp modifié + TrinityCore 3.3.5 + addon `GMTransport` (dépend de `GMBlueprint`).
Interface français/anglais suivant la langue choisie dans GM Blueprint.

## Essai en jeu

1. Relancer worldserver et le client mis à jour. Activer les deux addons.
2. Avec un compte autorisé à `.gobject add`, ouvrir **`/gmroute`**.
3. Choisir un modèle avec les boutons Modèle. Se déplacer, éventuellement en vol,
   et cliquer **Ajouter ici** pour chaque repère. **Ctrl + Ajouter** insère après
   le point sélectionné. Commencer avec quatre points espacés sur la même carte.
4. Les boutons numérotés dans le monde sélectionnent les points. Les boutons
   Point précédent/suivant fonctionnent aussi pour ceux hors écran.
5. **Maj + souris**, curseur hors de l'interface, déplace le point sélectionné
   dans le plan horizontal de son altitude. **Déplacer ici** le ramène à la position
   du personnage. Les champs X/Y/Z et Arrêt permettent des réglages précis ;
   **Appliquer** ou Entrée valide les champs. Arrêt = durée en secondes.
6. Des fanions 3D locaux marquent les points, avec des numéros cliquables à leur
   pied. Le trait gris relie les repères, le trait cyan représente la courbe suivie.
   La liaison dernier → premier est incluse : il s'agit d'une boucle continue.
   Fanions, numéros et traits sont affichés dès la création des points, sans
   lancer l'aperçu animé, tant que l'éditeur reste ouvert. Les fanions suivent
   la distance de rendu du monde ; les numéros et traits sont projetés sans
   limite de distance dans le champ de la caméra. Les liaisons sont découpées
   au bord de l'écran, même lorsque leurs extrémités sont hors champ.
7. **Aperçu / Stop** anime le modèle local, sans collision ni passagers.
8. **Sauvegarder** sauvegarde le trajet complet ; attendre la confirmation.
   **Activer** crée ensuite un véritable transport sur le serveur. La même
   activation répétée ne crée pas de doublon.
9. **Retirer transport** désactive le dernier transport activé par l'éditeur,
   ou celui du brouillon sauvegardé. Tous les passagers doivent être descendus
   et le GM doit rester sur sa carte. Le trajet sauvegardé reste disponible.

Le brouillon est conservé par personnage dans les SavedVariables, écrites par WoW
au logout ou `/reload`. Fermer le panneau ne détruit pas le brouillon.
**Nouveau** demande confirmation avant de remplacer le brouillon.
Le bouton **Catalogue des transports** ouvre la bibliothèque, avec recherche
par nom ou entry. Cliquer sur une ligne charge ses points depuis le serveur. Un
brouillon modifié demande confirmation avant remplacement. La réception complète
du parcours est nécessaire avant de remplacer le brouillon.

**Mettre à jour** conserve l'entry du transport personnalisé chargé. S'il est actif,
le GM doit être sur sa carte et aucun passager ne doit être à bord : le transport
est recréé avec son nouveau parcours, sous la même entry. Une révision empêche
d'écraser les modifications plus récentes d'un autre GM ; recharger en cas de refus.

Les routes enregistrées sont chargées au redémarrage du serveur. Celles marquées
actives redémarrent automatiquement. Pour réactiver une ancienne entry :
`.gmroute activate ENTRY`. Pour la retirer : `.gmroute deactivate ENTRY`.
Les anciens parcours se rechargent depuis cette bibliothèque, même après remplacement
du brouillon local, changement de personnage ou redémarrage.

## Créer une boucle entre continents

1. **Nouveau**, puis choisir un modèle. Poser les points du port vers le large.
2. Se téléporter sur une autre carte, rouvrir `/gmroute` si nécessaire et continuer
   avec **Ajouter / Ctrl insérer** (sans Ctrl pour ajouter à la suite).
3. Chaque point enregistre sa carte. Deux points consécutifs sur des cartes
   différentes créent une transition instantanée, sans courbe entre continents.
   Le libellé du point de départ indique par exemple « carte 1 → 0 ».
4. Poser au moins deux points par portion de carte, pour définir une direction.
   Continuer vers le port d'arrivée, puis repartir au large. Revenir sur la carte
   initiale et poser la portion de retour. La fermeture au premier point est automatique.
5. **Sauvegarder**, puis **Activer**. Le transport utilise le transfert de carte de
   TrinityCore avec conservation des joueurs à bord. Les trajets personnalisés
   restent à vitesse constante entre arrêts, sans l'accélération des trajets Blizzard.

Deux, trois ou davantage de cartes extérieures sont possibles dans la limite des
128 points. Les cartes instanciées sont refusées. Les repères et la courbe ne sont
visibles que sur leur carte. L'aperçu disparaît quand le bateau passe sur une autre
carte et réapparaît lorsqu'il revient ; il ne téléporte pas le personnage.
Les anciens trajets sans carte par point conservent automatiquement leur carte initiale.
Tous les joueurs utilisant ces trajets doivent avoir le client et l'addon mis à jour.

## Annuler une modification

**Annuler modif. / Undo change**, à côté d'Appliquer, restaure l'état précédent du
brouillon (coordonnées, réglages, ajout/suppression de point ou modèle). Un glissement
complet avec Maj compte pour une seule modification. Jusqu'à 30 états sont conservés
pendant la session d'édition. Sauvegarder, charger un autre trajet ou créer un nouveau
brouillon remet cet historique à zéro ; un `/reload` le remet aussi à zéro.

## Transports Blizzard

La liste comprend tous les modèles de transports type 15 et les animations type 11
présents dans la base (31 trajets et 89 modèles animés dans l'installation testée).
Les modèles animés, notamment les ascenseurs, sont signalés par `[—]` : ils n'ont
pas les mêmes waypoints et ne sont pas éditables avec cet outil.

Les trajets Blizzard `[WoW]` gardent le moteur de déplacement d'origine, les cartes,
les téléportations, l'accélération, l'équipage et les événements. Les coordonnées,
les délais et le nombre de points sont modifiables. **Ajouter** complète le trajet ;
**Ctrl + Ajouter** insère après le point sélectionné ; **Supprimer point** retire ce
point après confirmation. **Annuler modif.** permet de revenir en arrière avant
sauvegarde. Les points conservent leur identité quand leur numéro change.

Les transitions doivent conserver assez de points techniques pour les courbes.
Un trajet invalide est refusé avant enregistrement. Un trajet DBC instancié doit rester sur sa carte unique. Les arrêts utilisent un délai entier ; un délai
positif transforme un point ordinaire en arrêt. Les marqueurs de transition ne
peuvent pas devenir des arrêts. La vitesse utilise toujours le réglage global existant.

Le numéro de carte du point sélectionné est indiqué dans le panneau. Les fanions
et les portions de courbe sont visibles sur leur carte respective. Les portions
de continents différents ne sont pas reliées artificiellement.

**Mettre à jour** enregistre une surcharge propre à l'entry, appliquée au prochain
redémarrage de worldserver. Le trajet actif reste inchangé jusqu'à ce redémarrage.
Le chargement dans l'éditeur restitue la dernière version enregistrée, même si elle
attend encore ce redémarrage. Les DBC d'origine restent intacts. Les clients reçoivent
les surcharges appliquées depuis le serveur ; tous doivent utiliser le client modifié
et GMTransport. L'affichage et les passagers doivent encore être vérifiés en jeu.

## Annonces PNJ

1. Charger un transport depuis le catalogue et enregistrer les modifications du trajet.
2. Cliquer **Annonces PNJ** à côté du catalogue. Les annonces Blizzard reconnues
   sont affichées avec le GUID du PNJ. Précédent/suivant parcourt les annonces.
3. Pour une nouvelle annonce, choisir d'abord le point dans l'éditeur, cibler un
   PNJ permanent déjà créé avec les commandes GM, puis **Nouvelle**.
4. **Utiliser le point sélectionné** change le point de déclenchement ; **Lier le
   PNJ ciblé** remplace le PNJ. Le PNJ et le point doivent être sur la même carte.
5. Choisir **À l'arrivée / Au départ**, le délai en secondes (0 à 600), **Dire / Crier**
   et saisir le texte. Texte d'une ligne, 1000 octets maximum, sans codes couleur `|`.
6. **Enregistrer**, attendre la confirmation, puis redémarrer worldserver pour appliquer.
   **Annonce désactivée**, puis Enregistrer, permet de désactiver une annonce, y
   compris une annonce Blizzard remplacée par l'éditeur.

Les annonces personnalisées utilisent le GUID du spawn, pas l'entry du modèle de
créature. Elles restent attachées au même point lors d'une insertion en amont.
La suppression de ce point désactive ses annonces enregistrées. Les changements
restent en base au redémarrage ; les textes des tables Blizzard ne sont pas écrasés.
Les annonces concernent les cartes non instanciées. Le PNJ doit être vivant et
chargé sur sa carte pour parler.

La détection actuelle lit les annonces SmartAI déclenchées par un événement de
transport, directement ou via une liste d'actions temporisée à délai fixe, avec
un PNJ cible par proximité. Elle sélectionne le spawn le plus proche sur la carte
et le premier texte du groupe : vérifier ce choix avant de remplacer l'annonce.
Les scripts C++ spécifiques et les scénarios SmartAI plus complexes ne sont pas
importés automatiquement. Les actions autres que le dialogue restent exécutées.
Une annonce remplacée supprime uniquement l'action de dialogue d'origine concernée
pour ce transport, afin d'éviter deux messages.

Diagnostic depuis la console serveur : `gmroute ann audit ENTRY`.

## Fonctionnement

- Courbe Catmull-Rom fermée ; lookup de longueur par segment pour une vitesse
  de déplacement constante. Pas d'accélération progressive aux arrêts dans cette version.
- Même code de calcul dans le client et le serveur (`shared/RouteMath.h`).
- Les nouveaux templates sont créés en mémoire depuis un transport existant,
  avec une entry réservée entre 9000000 et 9999999 et un trajet indépendant.
- Tables InnoDB : `gm_transport_route`, `gm_transport_route_node`,
  `gm_transport_native`, `gm_transport_native_node`, `gm_transport_revision`,
  `gm_transport_history`, `gm_transport_route_node_map`, `gm_transport_node_meta`,
  `gm_transport_announcement`. L'historique archive les versions remplacées ; les
  données Blizzard initiales restent dans les DBC.
  Enregistrement transactionnel des points ; aucun DBC à éditer ou redistribuer.
- Les définitions sont envoyées au login, à la synchronisation et avant activation.
  **Chaque joueur devant utiliser ces transports doit avoir ce client modifié
  et l'addon GMTransport activé.** Un client standard ne connaît pas ces trajets.
- Les trajets d'origine et la modification existante spécifique au trajet 1221
  sont préservés. La correction d'embarquement spécifique au 1221 n'est pas
  appliquée aux nouveaux trajets ; ils utilisent les mécanismes habituels de
  collision et de passagers des transports. L'embarquement est à tester en jeu.

## Limites des nouveaux trajets personnalisés

- Cartes non instanciées ; au moins deux points par portion entre transitions.
- 3 à 128 points ; points consécutifs sur une même carte, y compris dernier/premier, distants de
  1 à 2000 unités ; longueur totale au plus 50000 unités.
- Vitesse 1 à 40 unités/s ; attente 0 à 600 secondes par repère.
- Au plus 128 routes personnalisées chargées.
- Fanions sans collision, non enregistrés sur le serveur. Numéros et lignes
  visibles uniquement dans l'éditeur, y compris à travers les obstacles ; pas
  d'évitement automatique des collisions.
- Le rayon de la souris doit rencontrer le plan horizontal du point : viser
  sous l'horizon si le point est plus bas que la caméra, au-dessus s'il est plus haut.
- Pas de PNJ d'équipage copiés automatiquement depuis le transport source.
- La courbe peut dépasser le polygone des repères : vérifier l'aperçu autour des obstacles.

## Vérification

`tests/route_math.cpp` : continuité aux points, fermeture, pauses, périodicité,
valeurs finies et rejets des trajets invalides.
La projection vérifie aussi les coordonnées relatives à une caméra éloignée
de l'origine, le rejet des points derrière elle et les extrémités hors écran.

`tests/ui_smoke.lua` (Lua 5.1 du client) : brouillon, carte, upload avec accusés de
réception, doublons, publication complète, activation, révision, délais et langues.

`tests/server_startup.py` : charge un trajet temporaire enregistré, crée un vrai
transport au démarrage, laisse tourner la simulation, arrête proprement le serveur
et supprime uniquement les données temporaires du test. Vérifie également une boucle
1 → 0 → 571 → 1 réellement exécutée et les trajets historiques sans carte par point.
Exiger worldserver arrêté. Le transfert avec un joueur embarqué reste à tester en jeu.

Le rendu, le clic des repères et l'embarquement réel nécessitent un essai en jeu.

## Retour arrière

Restaurer client et serveur depuis les sauvegardes annoncées à l'installation,
puis désactiver GMTransport. Les données de l'éditeur peuvent rester en base :
l'ancien serveur ne les lit pas. Les sources TrinityCore précédentes sont dans
`C:\TC\transport-source-backup-20261016-042010`.
