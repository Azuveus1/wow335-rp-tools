# Installation sur une copie de développement

Le bundle vise Windows x64, Visual Studio 2022, le client 3.3.5a build 12340 et les **révisions exactes** de `upstream.json`. Utiliser un serveur et des bases de test séparés. Les outils de préparation modifient des tables ; sauvegarder ses propres bases avant leur utilisation. Aucun fichier de configuration d'une installation existante n'est fourni.

## 1. Récupérer les bases

Dans PowerShell, depuis un dossier de travail sans espaces (ou adapter les anciens CMake de test) :

```powershell
git clone https://github.com/SatyPardus/wowdecomp.git C:/dev/wowdecomp
git -C C:/dev/wowdecomp checkout c74494faee3cebeb29843030f61d9204ed35e701
git clone --branch 3.3.5 https://github.com/TrinityCore/TrinityCore.git C:/dev/TC/TrinityCore
git -C C:/dev/TC/TrinityCore checkout f77768faad1668b73dee05e5ae62740a583cdfed
```

Les dépendances, les données world/DBC/MMAP/VMAP et la configuration de connexion restent celles des projets d'origine. Pour un core différent, revoir et porter chaque correctif ; ne pas forcer son application.

## 2. Appliquer les sources

Depuis la racine de ce bundle, après lecture des deux correctifs :

```powershell
python scripts/apply.py --client C:/dev/wowdecomp --server C:/dev/TC/TrinityCore
python scripts/apply.py --client C:/dev/wowdecomp --server C:/dev/TC/TrinityCore --apply
Copy-Item local.example.json local.json
# Adapter local.json à ses propres chemins.
python scripts/configure_tools.py local.json
```

Le premier appel vérifie seulement les révisions, l'état vierge des checkouts et l'application des correctifs. Le second copie les nouveaux fichiers et `Tools/` dans le client. Il refuse les fichiers déjà présents. L'outil de configuration adapte les chemins Windows historiques dans cette copie de `Tools/`, sans stocker de mot de passe. Les anciens scripts `PatchSources.py`, `InstallSources.py`, `Patch*.py` sont conservés pour comprendre le développement ; **ne pas les relancer après le correctif consolidé**. Certains utilisent des ancrages propres à l'installation d'origine.

Le répertoire `server_home` doit contenir `bin/worldserver.conf` et `data/dbc/`. Les scripts utilisent le client MySQL indiqué dans `local.json`. Créer la configuration avec ses propres identifiants, selon la documentation TrinityCore. `configure_tools.py` ne crée ni cette configuration ni les bases.

## 3. Préparer les données

Après installation d'une base world compatible avec la révision et des bases auth/characters standard, **worldserver arrêté**, exécuter dans la copie du client :

```powershell
Set-Location C:/dev/wowdecomp
python Tools/GMTransport/Prepare.py --schema
python Tools/HunterPrototype/Prepare.py
python Tools/HunterPrototype/PrepareBoard.py
python Tools/RoleplayNames/Prepare.py --apply
python Tools/CivilRegistry/Prepare.py --apply
python Tools/CivilRegistry/PrepareBirthNames.py --apply
python Tools/CivilRegistry/GuardDirections.py --apply
python Tools/GMBlueprint/ExportCatalog.py --config C:/dev/TC/bin/worldserver.conf --dbc C:/dev/TC/data/dbc/GameObjectDisplayInfo.dbc --mysql 'C:/Program Files/MySQL/MySQL Server 8.0/bin/mysql.exe'
```

Adapter explicitement les trois chemins du dernier appel. `Prepare.py` de GMTransport régénère aussi sa bibliothèque de modèles. Les deux catalogues livrés sont vides volontairement : ils ne contiennent aucun export des données de l'installation d'origine.

Les scripts lisent les identifiants localement dans `worldserver.conf`, puis les transmettent au processus MySQL via son environnement. Ils ne les écrivent pas dans les sources. Les sauvegardes et journaux qu'ils produisent dans `build/` sont locaux et ne doivent pas être publiés. Les installations avec des IDs réservés déjà utilisés nécessitent une adaptation préalable.

## 4. Compiler et installer

```powershell
cmake -S C:/dev/wowdecomp -B C:/dev/wowdecomp/build/x64 -G 'Visual Studio 17 2022' -A x64
cmake --build C:/dev/wowdecomp/build/x64 --config Release --target WowClient --parallel 2
```

Configurer et compiler TrinityCore selon son guide, avec **Custom activé** (par exemple `-DSCRIPTS=static`), puis construire `worldserver`. Le loader consolidé enregistre uniquement les cinq scripts de cette extension. Le petit CMake Custom historique est conservé avec son point d'entrée ; les sources sont également recueillies par le mécanisme standard des modules de TrinityCore.

WoW et worldserver arrêtés, sauvegarder les exécutables installés puis copier le nouveau client **avec ses DLL nécessaires**, et le nouveau worldserver avec ses dépendances. Utiliser leurs vrais chemins de sortie de compilation. Les scripts `Install.ps1` historiques sont destinés à l'organisation Windows décrite par `local.json` ; les lire avant de les utiliser. Ils ne remplacent pas une procédure de déploiement adaptée à un autre serveur.

Copier les addons suivants dans `Interface/AddOns/` :

| Source dans le client modifié | Dossier de destination |
| --- | --- |
| `Tools/GMBlueprint/addon/GMBlueprint` | `GMBlueprint` |
| `Tools/GMTransport/addon/GMTransport` | `GMTransport` |
| `Tools/HunterPrototype/addon/HunterContracts` | `HunterContracts` |
| les trois fichiers `CivilRegistry.toc`, `CivilRegistry.lua`, `WeddingSlot.lua` de `Tools/CivilRegistry` | `CivilRegistry` |

Les scripts de création des personnages et les marqueurs de dialogue sont intégrés au client natif ; ils ne sont pas un addon supplémentaire. Relancer WoW pour détecter les nouveaux addons et utiliser le client modifié chez tous les testeurs qui doivent voir les fonctionnalités propres au client.

## 5. Essai fonctionnel

Créer deux personnages jetables avec prénom et nom distincts, vérifier le registre à Hurlevent, les réponses à `/saluer`, puis les addons GM et le panneau d'Orgrimmar. Les commandes GM exigent les autorisations TrinityCore habituelles. Les guides détaillés et les identifiants réservés figurent dans chaque README de module.

L'installation de cette copie complète n'a pas encore été rejouée sur une machine vierge. Les correctifs sont vérifiés contre leurs sources de base et les règles indépendantes sont testées ; les données world et les bibliothèques propres au destinataire restent à valider.
