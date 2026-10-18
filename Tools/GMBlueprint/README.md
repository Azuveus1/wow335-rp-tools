# Placement GM de GameObjects

Extension locale du client wowdecomp 3.3.5a et de TrinityCore 3.3.5.
Le panneau est un addon utilisant les fonctions natives de placement du client.
Il ne cree aucun objet serveur tant que **Valider** n'est pas clique.

## Utilisation

1. Lancer le nouveau `C:\TC\bin\worldserver.exe` avec la configuration habituelle.
2. Lancer `C:\Jeux\WoW335a\WowClient64.exe`.
3. Activer **GM Blueprint** dans la liste des addons et se connecter avec un compte ayant le droit `.gobject add`.
4. Saisir `/gobuild 12345`, en remplacant 12345 par un **entry** existant de `gameobject_template` (pas un GUID de spawn, ni un displayID).
5. Maintenir **Maj** et deplacer la souris sur le sol pour placer l'apercu. Relacher Maj pour figer la position.
6. **Maj + molette haut/bas** : rotation animee de +1/-1 degre par cran. Ajouter **Ctrl** pour 0,1 degre. **Maj + fleche haut/bas maintenue** : hauteur continue a 1 unite/seconde, ou 0,1 unite/seconde avec Ctrl. La position horizontale reste fixe pendant ces reglages jusqu'au prochain mouvement de souris. Les boutons utilisent 1 degre et 0,1 unite, dix fois moins avec Ctrl.
7. Cliquer **Valider**. Le serveur confirme le GUID sauvegarde. **Annuler / Fermer** ou Echap retire l'apercu local.

On peut aussi ouvrir `/gobuild` sans identifiant et utiliser le champ du panneau.

### Gizmo de déplacement (version 1.2)

Dès que le blueprint est chargé, trois flèches apparaissent à son point d'origine :
**X rouge**, **Y vert**, **Z bleu**. Maintenir le bouton gauche sur une flèche et
la tirer déplace uniquement cet axe. Il n'est pas nécessaire de maintenir Maj.
Relâcher le bouton fige la position. Maintenir Ctrl pendant le glissement réduit
le déplacement par dix, sans saut lors du changement de précision.

Les axes suivent le repère du monde, indépendamment de la rotation de l'objet.
X et Y déplacent horizontalement ; Z règle l'altitude. La taille des flèches
s'adapte à la distance de la caméra. Un axe regardé presque de face peut être
masqué : tourner légèrement la caméra pour le rendre utilisable. Les flèches
restent visibles à travers le modèle pour sélectionner les axes des grands objets.
Les limites de placement et la confirmation **Valider** s'appliquent toujours.
Le gizmo disparaît pendant le chargement, la validation, le combat ou à la fermeture.

Cette version nécessite le client recompilé et les fichiers de l'addon mis à jour.
Installation du client uniquement : `Tools/GMBlueprint/Install.ps1 -ClientOnly`.

### Plans et rotation 3D (version 1.3)

Le bouton **Gizmo : déplacer / Gizmo : rotation**, à côté de Catalogue, alterne
entre les deux modes. Cliquer-glisser une poignée suffit, sans maintenir Maj.
Ctrl affine les déplacements et rotations par dix, sans saut à son activation.

En déplacement :
- Flèches X rouge, Y verte, Z bleue : un seul axe.
- Carrés XY, XZ, YZ : deux coordonnées changent ; la troisième reste fixe.
- Centre XYZ : déplacement libre dans le plan de la vue. Selon la caméra, les
  trois coordonnées peuvent changer. La souris définit deux directions à la fois ;
  les axes/plans permettent ensuite de régler la profondeur. Maj + souris conserve
  le placement sur le sol comme auparavant.

En rotation :
- Anneau rouge X et vert Y : inclinaison du décor.
- Anneau bleu Z : rotation horizontale.
- Les angles X/Y/Z sont affichés dans le panneau. Les anneaux suivent les axes du
  monde. Vu de profil, un anneau reste manipulable par glissement le long de sa projection.
- Les boutons de rotation et la molette continuent de tourner autour de Z en
  conservant l'inclinaison choisie.

**Valider** transmet et sauvegarde l'orientation complète (quaternion) dans les
champs de rotation existants de `gameobject`. TrinityCore normalise la rotation
avant création. Les anciennes requêtes limitées à l'angle horizontal restent
acceptées. Les collisions des décors statiques inclinés utilisent également cette
orientation ; le fonctionnement des collisions des transports est conservé.
Cette version nécessite le client, l'addon et worldserver mis à jour.

### Langue et catalogue (version 1.1)

- Le menu en bas du panneau choisit **Français** ou **English**, immédiatement. Le choix est conservé dans `GMBlueprintSettings` entre les sessions ; par défaut, la langue du client est utilisée (anglais hors client français).
- **Catalogue** ouvre une bibliothèque compacte de 280 × 355 unités à gauche, avec six lignes par page. Cliquer sur une ligne (nom et identifiant) remplace l'aperçu courant, après le contrôle des droits serveur habituel. Seul **Valider / Place object** sauvegarde un objet.
- Recherche par nom français ou anglais, entry ou chemin du modèle. Plusieurs mots sont combinés ; les accents usuels sont ignorés. Les catégories sont aussi recherchables dans les deux langues. Les flèches changent de page (6 résultats).
- Le menu des catégories propose bâtiments, arbres, végétation, chaises/bancs, tables, feux/torches, barrières/murs, récipients, rochers/cristaux, éclairages, panneaux/bannières et autres. Le survol d'une ligne affiche le nom complet et le fichier modèle.
- Le catalogue fourni contient **20 944 templates** issus de la base locale et possédant une référence de modèle dans `GameObjectDisplayInfo.dbc`. Les transports sont exclus. Ce ne sont pas 20 944 visuels distincts : plusieurs templates partagent un modèle. Les noms français absents utilisent le nom anglais.
- Le classement est déduit des noms anglais et chemins de modèles : il peut être imparfait. La recherche globale conserve tous les candidats, même classés dans Autres. Les fichiers réellement disponibles sont vérifiés lors de l'aperçu. Les éléments de terrain sans template GameObject ne sont pas inclus.
- Le catalogue est un instantané local. Après ajout/modification de templates, exécuter `python Tools/GMBlueprint/ExportCatalog.py` puis recopier `CatalogData.lua` dans l'addon. L'export lit uniquement les templates, traductions et DBC ; aucune écriture SQL. Les paramètres `--config`, `--dbc`, `--mysql` et `--output` permettent d'adapter les chemins. Aucun identifiant de connexion n'est exporté.
- Pour installer cette version, copier les quatre fichiers Lua et le `.toc` du dossier addon. Comme le manifeste ajoute des fichiers et une variable sauvegardée, redémarrer complètement le client une fois après installation. Aucun changement de binaire client ou serveur n'est nécessaire.

Les raccourcis sont temporaires pendant l'apercu et hors combat ; les touches sans Maj conservent leurs fonctions habituelles. Ils sont retires a la fermeture ou pendant la validation.
La molette et les fleches sont traitees directement par le client, avant les commandes de camera/deplacement. Cette version requiert le nouvel executable et l'addon correspondant.
L'outil ajoute une commande distincte ; `.gobject add` garde son comportement d'origine.
Apres validation, `.gobject delete GUID` permet de supprimer le placement avec les droits correspondants.
Fermer le panneau apres avoir clique Valider n'annule pas une requete deja envoyee.

## Comportement et limites

- Modeles M2 : apercu translucide, cyan si valide, rouge sinon.
- Batiments WMO : apercu local **opaque**, sans collision. La transparence WMO n'est pas implementee.
- La taille vient du template serveur ; pas de redimensionnement arbitraire, de tangage ou de roulis.
- Distance maximale de placement : 100 unites depuis le personnage, sur sa carte et dans sa phase.
- Les transports et le placement depuis un transport sont exclus.
- Les objets integres aux cartes ADT/WMO ne sont pas edites : il faut un template GameObject existant.
- Le point sous la souris utilise les collisions du monde ; aucun calcul automatique d'enfouissement des pieds du modele n'est applique. Ajuster la hauteur si necessaire.
- Droits controles par TrinityCore avec le RBAC existant de `.gobject add`, y compris pour obtenir l'autorisation d'apercu.
- L'aperçu est uniquement local, sans spawn temporaire dans la base.
- En cas de reponse absente apres Valider, verifier `.gobject near` avant de recommencer. Aucune retransmission automatique.
- Les rechargements d'interface et sorties de carte nettoient l'apercu.

## Fichiers

- `WoW/Source/Ui/GMBlueprint.cpp` : chargement du modele, position, rotation, rendu et API Lua.
- `WoW/Source/Ui/GameUI*.cpp` : enregistrement et nettoyage de l'API.
- `WoW/Source/WorldClient/Map.cpp` : nettoyage avant decharger une carte.
- `addon/GMBlueprint/` : interface et protocole des commandes.
- `server/trinitycore-blueprint.patch` : ajout dans `src/server/scripts/Commands/cs_gobject.cpp`, prepare pour le depot local `C:\TC\TrinityCore`.
- `Install.ps1` : copie des binaires et de l'addon, avec sauvegardes datees et controle des empreintes. Refuse si WowClient64 ou worldserver tourne. Utiliser `-ClientOnly` pour mettre a jour uniquement `C:\Jeux\WoW335a`, sans recopier le serveur (seul WowClient64 doit alors etre ferme).

La commande serveur conserve la meme procedure Create / SaveToDB / LoadFromDB que `.gobject add`, avec une position explicite. Aucun schema SQL ni configuration serveur n'est modifie.

## Verification

Compilation Release x64 du client et de worldserver, plus tests du panneau avec le Lua 5.1 du client :

```powershell
cmake --build build/x64 --config Release --target WowClient --parallel 4
cmake --build C:/TC/build --config Release --target worldserver --parallel 4
cmake -S Tools/GMBlueprint/tests -B build/blueprint-tests -G "Visual Studio 17 2022" -A x64
cmake --build build/blueprint-tests --config Release
./build/blueprint-tests/Release/blueprint_lua_test.exe Tools/GMBlueprint/tests/ui_smoke.lua
```

Les tests automatises simulent l'interface et les messages serveur ; ils ne valident ni le rendu 3D ni les ecritures sur la base du serveur actif. Le test en jeu doit verifier un objet M2, un batiment WMO, l'annulation, la rotation, la hauteur, puis la persistance apres reconnexion. Tester aussi un compte sans droit GM.

Si le shell presente la collision Windows `Path` / `PATH`, conserver sa valeur puis supprimer les deux variantes et restaurer une seule variable `Path` dans ce processus avant CMake. Aucune modification du PATH systeme n'est necessaire.

## Retour arriere

Fermer le jeu et le serveur, puis restaurer leurs executables depuis les dossiers de sauvegarde dates annonces par `Install.ps1`. Desactiver GM Blueprint dans la liste des addons.
Le source serveur initial est conserve dans `C:\TC\blueprint-backup-20261016-015205\cs_gobject.cpp`.
Le manifeste de la derniere installation, avec fichiers sauvegardes et empreintes, est dans `build/x64/GMBlueprint-install.json`.
Les installations avec `-ClientOnly` utilisent `build/x64/GMBlueprint-client-install.json`.
Ne pas utiliser un reset global des depots : TrinityCore contient d'autres modifications personnelles.
