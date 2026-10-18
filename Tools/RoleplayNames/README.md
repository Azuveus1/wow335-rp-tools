# Prénom et nom de famille à la création

Le client natif affiche deux champs sur l'écran de création, en français ou en
anglais selon sa langue. Le second champ est obligatoire pour une nouvelle
création. Tab passe d'un champ à l'autre ; Entrée dans le prénom passe au nom,
Entrée dans le nom valide. Le bouton de création reste celui du jeu.

Chaque partie suit les règles habituelles de WoW : lettres, accents selon la
langue, longueur maximale de 12 caractères (limites locales d'origine pour les
alphabets asiatiques). Le nom complet ne dépasse pas 47 octets UTF-8, pour rester
compatible avec les tampons de noms du client 3.3.5a. Une seule espace sépare les
deux parties ; aucun champ ne peut contenir d'espace supplémentaire.

Le nom enregistré est `Prénom Nom`. TrinityCore capitalise chaque partie. Son
index unique existant sur `characters.name` garantit l'unicité du nom complet,
y compris les variantes de casse selon la collation de la base. Le prénom seul
et le nom de famille seul ne sont pas uniques. L'affichage normal et les titres
utilisent ce nom complet ; les personnages gardent leur GUID.

`Prepare.py --apply` élargit `characters.name` et `deleteInfos_Name` à 25 caractères
sans effacer les personnages. Un instantané du schéma et des noms d'origine est
enregistré dans `build/rp-names-database-backup-*.json`. Les anciens personnages
sans nom de famille restent acceptés à la connexion. Leur conversion n'est pas
automatique. Aucun mécanisme de mariage n'est ajouté.

Le nom complet doit être utilisé pour désigner précisément un nouveau personnage
dans les outils qui acceptent un nom (courrier, panneau de contrats, etc.). Les
commandes MJ dont les arguments sont séparés par des espaces peuvent nécessiter
un nom entre guillemets ou la sélection du personnage. Cette version ne propose
pas de recherche interactive pour départager des prénoms identiques.

`PatchSources.py` installe les modifications ciblées de TrinityCore et sauvegarde
les fichiers modifiés. L'interface `Creation.lua` est intégrée au client dans
`RoleplayNamesLua.inc`, chargée après le GlueXML d'origine, avant la connexion.
Aucun addon ni remplacement des archives MPQ n'est nécessaire.

Vérifications : `test_creation.lua` avec le Lua 5.1 du client ; `test_server.py`
sur un worldserver isolé avec trois personnages jetables. Le test serveur contrôle
la validation réelle du core, les accents, la normalisation, les doublons en base
et la résolution du nom complet dans le cache. Les personnages jetables sont
supprimés après arrêt propre. La création réelle via les clics du client reste
le test final à effectuer en jeu.

Installation : compiler WowClient et worldserver en Release x64, exécuter
`Install.ps1` après fermeture de WoW et de worldserver. Les deux exécutables
existants sont sauvegardés avant copie et les copies sont vérifiées par SHA-256.

## Marqueurs des textes RP

Dans le parseur de quêtes et dialogues du client :

| Marqueur | Valeur pour Martin Ventdacier |
|---|---|
| `$P` ou `$p` | Martin |
| `$F` ou `$f` | Ventdacier |
| `$N` ou `$n` | Martin Ventdacier |

`Bonjour $P !` donne « Bonjour Martin ! » ; `Bonjour monsieur $F.` donne
« Bonjour monsieur Ventdacier. » Le texte peut combiner les marqueurs.

Le parseur commun est utilisé notamment par les quêtes, les conversations gossip
et les paroles de PNJ déjà traitées par `QuestParserParseText`. `$N` conserve son
comportement. Les descriptions de sorts emploient un autre parseur : ses codes
`$P`/`$F` ne sont pas modifiés. `%t` dans le chat et les macros reste inchangé.

Les noms sont résolus par la même source que `$N` : personnage présent, cache
des noms ou nom explicitement fourni. Pour un ancien personnage à nom simple,
`$P` renvoie son nom et `$F` reste vide. Un nom de créature ne reçoit pas de nom
de famille artificiel. Sans nom disponible, les marqueurs restent littéraux,
comme `$N` auparavant. Aucun choix automatique de tutoiement ou de familiarité
n'est ajouté : le rédacteur du texte choisit son marqueur.

Vérification des données locales : aucun `$P`/`$F` dans les tables de quêtes,
gossip et `creature_text` contrôlées ; certains textes de `broadcast_text` et leurs
traductions contiennent déjà `$P`/`$p`, non interprétés par l'ancien parseur du
client. Ils prennent désormais le sens « prénom ». Aucun `$F`/`$f` trouvé dans
ces tables. Il n'existait pas de branche P/F dans le parseur de quêtes.

`rp_dialogue_test` compile les deux fonctions réelles du parseur, extraites de la
source courante avec `GenerateParserTest.py`, et remplace seulement les services
du monde par des doublures de test. Il contrôle les trois sources de noms,
les accents, les alias, les anciens marqueurs, la ponctuation, les noms simples,
les créatures, les données absentes et les tampons courts. Compiler et installer
seulement WowClient ; `Install.ps1 -ClientOnly` laisse worldserver en exécution.
# Commandes GM et noms complets

`PatchLiveNames.py` corrige aussi l'affichage d'un nom changé pendant la session : après `SMSG_INVALIDATE_PLAYER`, le client recharge le nom des personnages présents, puis utilise le callback natif pour régénérer le nom au-dessus du personnage et signaler `UNIT_NAME_UPDATE` aux portraits. L'infobulle est également actualisée. Le serveur et les noms stockés ne sont pas modifiés par cette correction. `rp_live_names_test` compile les vraies fonctions de réception, d'invalidation et de rafraîchissement avec des doublures pour le cache, le réseau et le rendu ; il vérifie le chemin asynchrone ainsi que les personnages absents, les requêtes refusées et les invalidations de guilde.

`PatchCommands.py` adapte le lecteur historique des noms utilisés par les commandes GM. Il reconnaît une identité complète existante avant de chercher un ancien personnage portant seulement le prénom, accepte les noms entre guillemets et conserve le curseur de lecture des paramètres suivants. Les liens de joueur conservent leur traitement natif. La commande `.summon` traite explicitement tout son argument comme une identité : un nom de famille inexistant ne doit jamais invoquer par erreur un ancien personnage portant le seul prénom.

Exemple : `.summon Bertrand Clairbois` (ou `.summon "Bertrand Clairbois"`). Les permissions et les règles habituelles de téléportation restent celles de TrinityCore. Cette adaptation concerne le lecteur historique partagé ; elle ne remplace pas tous les parseurs particuliers des autres commandes.

Les tests serveur exécutent le vrai `ChatHandler::extractPlayerTarget` avec des identités jetables partageant un prénom, dont une identité ancienne sans nom de famille. Ils vérifient aussi les guillemets, les liens et les paramètres numériques suivants. `InstallCommands.ps1` installe seulement worldserver, avec sauvegarde et contrôle SHA-256. WoW peut rester ouvert ; worldserver doit être arrêté.

