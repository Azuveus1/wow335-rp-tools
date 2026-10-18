# Registre civil, alliances et familles

Cette extension ajoute deux PNJ humains dans le **Centre d'accueil de Hurlevent**, quartier commerçant, dans la pièce d'Aldwin Laughlin (le responsable des guildes). C'est le lieu retenu pour la mairie RP.

Les gardes qui donnent des directions proposent aussi **Centre d'accueil / Registre civil**, avec un repère de carte sur l'officier. `GuardDirections.py --apply` ajoute cette destination aux menus 435 et 3506, en français et en anglais, en conservant les autres options et la réponse Blizzard du centre d'accueil. Les quatre tables concernées peuvent être rechargées à chaud avec `ReloadGuardDirections.py <PID-worldserver>` ; aucune recompilation n'est nécessaire.

L'officier d'état civil inscrit gratuitement le premier nom de famille des personnages qui n'en avaient pas. Un changement ultérieur coûte **10 po par dossier**, avec un délai de **30 jours glissants**. Pour des époux, le paiement et la validation changent les deux noms ensemble. Les deux noms complets doivent être disponibles et le délai doit être écoulé pour chaque époux. Le prénom et le GUID restent inchangés. Les noms suivent les règles de création déjà installées : prénom et nom de famille de 12 lettres maximum chacun, nom de famille sans espace.

## Essai du mariage

1. Les deux personnages doivent avoir un nom de famille et être célibataires.
2. Chez le notaire matrimonial, acheter le **contrat de mariage pour 25 po**. Il apparaît dans les sacs ; un clic droit rouvre le document.
3. Aller chez **Qixdi à Baie-du-Butin**. Sa boutique habituelle est conservée. Choisir l'achat de deux alliances gravées, puis saisir le **nom complet actuel du partenaire**. La paire coûte **10 po supplémentaires** et arrive dans les sacs de l'acheteur. Il faut deux places libres.
4. Lire la gravure dans l'infobulle. Donner au partenaire l'alliance portant le nom de l'acheteur ; garder celle qui porte le nom du partenaire. Les alliances sont échangeables et sans statistiques.
5. Chacun équipe son alliance dans un emplacement de doigt. Se placer à moins de 10 mètres, hors combat, dans la même phase et à portée de vue.
6. L'acheteur sélectionne son partenaire, ouvre le contrat et clique sur **Demander la signature**. Le partenaire reçoit un formulaire montrant le nom commun et accepte explicitement. Les alliances sont vérifiées à la demande puis à l'acceptation.
7. L'acheteur rapporte le document au notaire, puis choisit **Enregistrer le mariage**. C'est à cet instant que les noms changent. Le document est consommé et les alliances restent aux époux.

Pour un couple homme/femme, le nom commun est celui de l'homme. Pour deux hommes ou deux femmes, c'est celui de l'acheteur du contrat. Les deux personnages ne peuvent jamais obtenir un même nom complet. Les identités sont revérifiées au dépôt : modifier un nom après la signature exige un nouveau document signé. Le mariage commence un nouveau délai de 30 jours pour le changement payant du nom familial.

Le document est lié à l'acheteur. Une seule participation à un contrat et une seule famille sont autorisées par personnage. Une demande de signature expire après deux minutes. Une validation ne peut pas être rejouée. L'annulation du contrat non enregistré se fait chez le notaire, sans remboursement. Si le document a été perdu, consulter le dossier chez le notaire permet de le récupérer sans repayer ; un document en banque doit être repris dans les sacs pour signer ou enregistrer.

## Tabards familiaux

Depuis la version 1.1, le client redessine également le tabard familial lorsque seul son blason change. Il n'est plus nécessaire de retirer puis de rééquiper le tabard pour actualiser son apparence.

Après le mariage, chaque époux peut retirer gratuitement un **tabard familial** chez le notaire. Un exemplaire maximum par personnage, banque comprise. Le blason comporte les mêmes cinq réglages que le tabard de guilde : emblème, couleur, bordure, couleur de bordure et fond. L'un ou l'autre époux peut enregistrer le blason commun gratuitement. Équiper le tabard familial pour l'afficher ; équiper celui de guilde pour afficher le blason de guilde. Le jeu conserve un seul emplacement de tabard. L'appartenance à la guilde et son blason ne sont pas modifiés.

L'éditeur présente les textures du tabard à plat. Le rendu porté, la sélection des personnages et les cadavres utilisent le blason familial grâce au client modifié. Tous les clients qui doivent voir cette apparence doivent utiliser cette version. Les tabards d'un époux déconnecté sont actualisés lors de sa prochaine connexion.

## Emplacement d'alliance dédié (version 1.1)

La fiche du personnage propose un bouton **Alliance**, sous les flèches de rotation, à gauche du modèle du personnage. Il est placé hors du panneau de statistiques ; le libellé est sous l'icône. L'inspection reprend ce même emplacement. Un clic sur l'emplacement vide place votre alliance gravée dans cet emplacement cosmétique, depuis les sacs ou depuis l'un des deux doigts de combat. Les deux emplacements d'anneaux ordinaires sont ainsi disponibles pour les statistiques. Aucun achat supplémentaire n'est nécessaire pour les alliances existantes.

Le véritable objet, son GUID et sa gravure sont conservés côté serveur dans `rp_registry_worn_ring` et `item_instance`. Pendant qu'il est porté, il n'occupe ni une place dans les sacs ni un emplacement d'anneau de combat et n'apporte aucune statistique. Un clic droit remet la même alliance dans les sacs ; si les sacs sont pleins, elle reste portée. Pour la signature d'un mariage, cet emplacement est reconnu comme une alliance portée, en complément des anciens emplacements de doigt.

La fenêtre d'inspection affiche le même emplacement **Alliance** et sa gravure. Cette interface nécessite l'addon CivilRegistry chez l'observateur. Le serveur vérifie que le personnage inspecté est visible, dans la même phase et à portée d'inspection ; les réponses concernant une ancienne cible sont ignorées. Les modifications de l'équipement cosmétique sont bloquées en combat et utilisent un jeton valable deux minutes, distinct des formulaires de mairie.

`test_wedding_slot.lua` exécute le vrai module Lua, y compris le traitement des jetons, les échanges de cible et l'événement d'inspection propre au client 3.3.5a. Le test serveur utilise les mêmes fonctions de transfert et de chargement des objets que le jeu et vérifie le passage sacs → emplacement dédié → sacs avec le même GUID et la même gravure. `civil_tabard_refresh_test` compile le véritable début de la fonction native de changement d'équipement et vérifie le rafraîchissement d'un tabard dont l'identifiant ne change pas.

## Nom de naissance permanent

`rp_registry_birth_name` conserve le premier nom complet officiel de chaque personnage. Il est immuable : un mariage, un changement payant, une personnalisation ou un changement de faction ne le remplacent pas. Pour les anciens personnages sans nom de famille, la première inscription complète établit ce nom de naissance. Pour les personnages déjà renommés avant cette version, `PrepareBirthNames.py --apply` récupère le premier nom complet connu dans l'historique du registre, ou à défaut leur nom complet actuel. Le script ne change aucun nom porté.

La combinaison prénom + nom de naissance est réservée au GUID du personnage dans `rp_registry_name_claim`, même lorsqu'elle n'est plus portée. Le prénom ou le nom de famille seuls ne sont pas réservés. Le même propriétaire peut reprendre ce nom ; une autre identité ne peut l'utiliser ni à la création, ni à la mairie, ni pour un mariage, ni via les services natifs de renommage/personnalisation/changement de faction. Les permissions GM permettant normalement de contourner les mots réservés ne contournent pas cette propriété.

Les réservations et le changement effectif du nom appartiennent à la même transaction CharacterDB. Un conflit annule la transaction ; il ne remplace pas une réservation existante. La vérification préalable renvoie au client le refus « nom déjà utilisé ». Aucun déclencheur MySQL ou changement de privilèges n'est nécessaire. Les noms de naissance restent réservés après suppression d'un personnage ; la génération de GUID tient compte de ces identités conservées pour éviter leur réattribution.

Cette version prépare la restitution du nom de naissance et la teste avec des personnages jetables, mais **n'ajoute pas encore de procédure ni de bouton de divorce**. Le futur divorce pourra utiliser `BirthNames::Get(guid)` et la transaction commune de renommage. Le retour au nom de naissance libère l'ancien nom marital s'il n'est pas lui-même le nom de naissance de ce personnage.

Installation initiale de cette partie, worldserver arrêté : `PrepareBirthNames.py --apply`, `PatchBirthNames.py`, `PatchSources.py`, compilation puis installation du serveur. Il n'y a pas de nouveau client ni d'addon à installer pour cette protection. `test_server.py` vérifie le nom de naissance après mariage, la collision insensible à la casse, le refus atomique d'usurpation, le retour autorisé du propriétaire et la libération de l'alias marital ; il nettoie uniquement ses propres identités temporaires.

## Installation et vérification

- `Prepare.py --apply` : tables CharacterDB `rp_registry_*`, `rp_family`, `rp_family_member` ; modèles et apparitions des PNJ ; objets dédiés ; ajout du service à Qixdi. Aucun personnage existant n'est supprimé ou renommé.
- `PatchSources.py` : sauvegarde des sources modifiées, copie du script Custom et corrections natives. Les anciens scripts Custom sont conservés.
- `Install.ps1` : exige les deux programmes arrêtés ; sauvegarde puis installe les exécutables et l'addon dans `C:\Jeux\WoW335a\Interface\AddOns\CivilRegistry` ; vérifie les SHA-256.
- `test_policy.cpp` : vérifie les 15 033 780 combinaisons de blason et leur encodage, les limites de délai et les règles de nom commun.
- `test_ui.lua` : exécute le vrai addon dans le Lua 5.1 du client, avec le transport et le rendu simulés.
- `test_server.py` : lance une copie isolée du serveur avec deux personnages jetables `account=0`, GUID 9000300–9000301. Vérifie PNJ, hauteur du sol, modèles d'objets, historique, cache de noms, famille et enregistrement atomique. Le doublon volontaire du test de rollback produit une erreur SQL attendue et confirme l'annulation du nom, de l'or et de l'historique. Les fixtures sont ensuite supprimées ; les noms, l'or et les guildes des vrais personnages doivent rester inchangés.

Identifiants réservés : officier 9100300, notaire 9100301, document 9100302, tabard 9100303, alliance 9100304. Le service de gravure utilise le PNJ existant Qixdi, entry 2849. Les tables font autorité pour les signatures, les familles et la gravure ; le texte affiché sur un objet ne permet pas de contourner les conditions.

Les sauvegardes de sources et de structure se trouvent dans `C:\wowdecomp\build\civil-registry-*`. La sauvegarde des exécutables installés est dans `C:\TC\civil-registry-backup-*`. Un retour à un ancien client exige aussi le serveur correspondant : les trois objets dédiés et le rendu de tabard sont propres à cette extension.
