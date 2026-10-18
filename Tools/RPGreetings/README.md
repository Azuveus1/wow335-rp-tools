# Réponses des PNJ aux salutations

Sélectionner un PNJ amical ou neutre, se placer à dix mètres au maximum, puis
utiliser `/saluer` (ou les emotes équivalentes bonjour, salut militaire).
Le PNJ rend le geste et dit une courte phrase adressée au personnage :

- « Bonjour Alice ! »
- « Bonjour madame Ventdacier. »
- « Bonjour monsieur Ventdacier. »

La formulation est choisie aléatoirement entre prénom et nom de famille avec
civilité. La civilité dépend du sexe d'origine du personnage, indépendamment
des transformations visuelles. Le prénom est la première partie du nom ; le nom
de famille est la suite, avec les accents conservés. Sans nom de famille,
le PNJ emploie le prénom. Les sessions frFR reçoivent du français, les autres
de l'anglais. La parole et l'animation sont celles d'un PNJ normal ; les joueurs
proches peuvent les voir/entendre. Aucune fenêtre gossip n'est ouverte.

Le délai est de **dix minutes par couple joueur–PNJ**, partagé entre les cinq
emotes reconnues (GREET 48, HAIL 53, HELLO 55, SALUTE 78, WAVE 101). Répéter l'emote ne repousse pas
l'échéance. Un autre PNJ peut répondre immédiatement, et le même PNJ peut répondre
à un autre joueur. Le délai est conservé en mémoire serveur et reprend à zéro
au redémarrage du serveur. Les identités de spawn persistantes empêchent le simple
déchargement d'une zone de contourner le délai ; les instances sont distinguées.

Le joueur et le PNJ doivent être vivants, visibles l'un pour l'autre, hors combat,
avec ligne de vue. Les PNJ hostiles, morts, contrôlés par un joueur, familiers,
totems et les animaux ordinaires ne répondent pas. Les humanoïdes non hostiles et
les PNJ proposant des services/gossip/quêtes sont éligibles. Le joueur doit avoir
sélectionné exactement le PNJ indiqué dans son emote. Les salutations sans cible,
vers un joueur ou à trop grande distance ne provoquent rien. Les PNJ ne suivent
pas le joueur ; ils se tournent vers lui seulement s'ils sont immobiles.

Implémentation : un `PlayerScript::OnTextEmote`, sans remplacement des AI ni
modification des textes/du schéma de base de données. Le traitement d'origine
des emotes et les réactions déjà prévues par les scripts de quêtes continuent
d'être exécutés. Le délai limite la réponse générique de ce module ; il ne
désactive pas les événements spécifiques aux quêtes ou aux autres scripts.
La mémoire de délai est protégée contre les accès simultanés, limitée à 16384
couples et nettoyée périodiquement. Si cette limite est atteinte, aucune échéance
encore active n'est supprimée pour autoriser une réponse supplémentaire.

`PatchSources.py` installe les deux sources dans Custom et inscrit le module dans
le chargeur en sauvegardant les fichiers précédents. Compiler worldserver Release
x64. `Install.ps1` sauvegarde et remplace uniquement worldserver avec vérification
SHA-256. Le client reste identique et peut rester ouvert pendant l'installation.

Tests : `rp_greeting_test` vérifie les noms, sexes, langues, critères de réponse,
échéances exactes, répétitions, indépendance des couples/instances, mémoire bornée
et concurrence. `test_french_command.py` vérifie la chaîne réelle des fichiers du
client français : `/saluer` → EMOTE49 → GREET → paquet d'emote 48. Les numéros des
variables Lua EMOTE ne sont pas les identifiants du protocole. Les identifiants
du module sont également comparés aux constantes du core à la compilation.
`test_server.py` vérifie le chargement du vrai PlayerScript et le
démarrage/arrêt propre du serveur sur un port local isolé. Le geste, la bulle et
le texte vus depuis un joueur restent à tester en jeu.
