# Vérification de la livraison

Résultat du contrôle de publication : **8 tests C++ et 9 tests Lua réussis**. Les tests de catalogue utilisent de petites données fictives indépendantes des catalogues locaux. Les 24 fichiers modifiés de chaque correctif ont été vérifiés contre la révision d'origine. Cela ne remplace pas une installation complète sur une nouvelle machine.

## Contrôles de ce bundle

Les révisions de base des deux projets sont épinglées. Les deux correctifs sont vérifiés avec `git apply --check` sur les fichiers d'origine, puis leur résultat est comparé aux sources de travail (à l'exception du loader Custom nettoyé de scripts sans rapport). Les overlays sont séparés des correctifs pour rendre les nouveaux fichiers identifiables.

Le bundle utilise un nouvel historique Git : les anciens commits, configurations, sauvegardes et journaux locaux ne sont pas publiés. Une liste SHA-256 des fichiers partagés est fournie dans `MANIFEST.sha256`. Elle décrit le contenu avant publication et n'inclut ni elle-même ni les métadonnées Git.

Le contrôle des données exclut les bases et configurations réelles, les exécutables/DLL, les archives et ressources extraites du jeu. Les catalogues dérivés de world/DBC sont remplacés par des tables vides à régénérer. Des tests contiennent des identités fictives ; ce ne sont pas des enregistrements de personnages.

## Tests sans base de données

Depuis la racine du bundle :

```powershell
cmake -S tests -B build/policies -G 'Visual Studio 17 2022' -A x64
cmake --build build/policies --config Release --parallel 2
ctest --test-dir build/policies -C Release --output-on-failure
```

Ce groupe couvre les règles du registre, les salutations, les gizmos, les parcours personnalisés et natifs, les règles de contrat, les événements de voyage et les données du panneau. Il ne simule pas une partie complète.

## Autres tests de développement

Les tests C++ et Lua des dossiers `Tools/` couvrent les fonctions natives extraites et les interfaces réelles avec des services simulés. Certains requièrent les bibliothèques déjà compilées du client et du serveur : exécuter ces projets après intégration et adaptation des chemins.

Les fichiers `test_server.py`, `server_*.py` et `ReloadGuardDirections.py` utilisent une configuration locale et peuvent lancer un serveur, créer des fixtures, modifier des tables ou envoyer des commandes. **Ils doivent être lus et exécutés uniquement contre des bases jetables isolées.** Ils proviennent du banc de développement ; ils ne sont pas des tests sans effet de bord.

Les essais déjà réalisés dans l'installation de développement ne garantissent pas le comportement d'une autre révision du core ou d'autres données world. Refaire au minimum les scénarios d'identité, double validation de paiement, butin privé, mariage et transitions de carte avant d'élargir les tests à plusieurs joueurs.
