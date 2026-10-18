# Outils RP pour WoW 3.3.5a

Sources des extensions développées pour un environnement local **TrinityCore 3.3.5 + wowdecomp x64**, destinées à la revue, aux essais et aux contributions. Ce dépôt contient les ajouts, les correctifs d'intégration et les tests ; les deux projets d'origine sont à récupérer séparément.

**Licence de nos ajouts originaux : GPL-2.0-or-later.** Les sources préexistantes conservent leurs notices. Le statut de licence du client amont doit encore être clarifié : cette publication ne présente donc pas l'ensemble client et dépendances comme intégralement sous licence libre. Voir [LICENSE](LICENSE) et [NOTICE.md](NOTICE.md).

| Module | Fonction |
| --- | --- |
| GMBlueprint | Placement de décors en aperçu fantôme, catalogue FR/EN, recherche, gizmos de déplacement et de rotation, validation persistante côté serveur. |
| GMTransport | Création et modification de parcours, catalogue des transports personnalisés et Blizzard, repères 3D, insertions/suppressions, transitions entre cartes, annonces liées aux PNJ. |
| HunterPrototype | Panneau de contrats, prime mise en réserve, mercenaire individuel de même niveau que sa cible, hostilité privée, courriers et butin ; pistes issues des vols et déplacement vers leur destination. |
| RoleplayNames | Prénom et nom de famille, unicité du nom complet, création des personnages, dialogues `$P` / `$F` / `$N`, rafraîchissement des noms et adaptation du lecteur de commandes. |
| RPGreetings | Réponse des PNJ amicaux ciblés à `/saluer`, avec délai de dix minutes par paire joueur/PNJ. |
| CivilRegistry | État civil à Hurlevent, changements de nom, contrats de mariage, alliances gravées, emplacement cosmétique inspectable, tabards familiaux et protection permanente des noms de naissance. |

## Pour commencer

- [Installation sur une copie de développement](docs/INSTALLATION.md)
- [Architecture et limites](docs/ARCHITECTURE.md)
- [Vérifications et données partagées](docs/VERIFICATION.md)
- [Origines et licences](NOTICE.md)
- Les guides d'utilisation détaillés sont dans chaque dossier `Tools/*/README.md`.

Les révisions exactes sont dans `upstream.json`. Les correctifs sont `patches/client.patch` et `patches/server.patch`. Les nouveaux fichiers natifs sont dans `overlays/`, les addons et outils dans `Tools/`.

Ce dépôt ne contient aucun compte de jeu, export de personnages, mot de passe, configuration locale, exécutable ou archive du jeu. Les noms présents dans les tests sont des exemples fictifs. Les catalogues générés depuis les données du jeu sont à régénérer sur l'installation du destinataire.

## Contributions

Ouvrir une branche et une pull request en décrivant le problème, la modification et les vérifications réalisées. Les mainteneurs d'un autre core doivent adapter les points d'intégration à leur révision : ces correctifs ne sont pas un module universel pour toutes les versions de TrinityCore.
