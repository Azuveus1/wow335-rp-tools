GMBlueprintUI = {}
local B=GMBlueprintUI
local en={
 ["Gizmo : deplacer"]="Gizmo: move", ["Gizmo : rotation"]="Gizmo: rotate",
 ["Rotation incomplete."]="Incomplete rotation.", ["Rotation invalide."]="Invalid rotation.",
 ["Gizmo : axes, plans XY/XZ/YZ ou centre XYZ.\nBouton Gizmo : deplacer / rotation. Ctrl : precision.\nMaj + souris : sol ; molette : tourner ; haut/bas : Z."]="Gizmo: axes, XY/XZ/YZ planes or XYZ center.\nGizmo button: move / rotate. Ctrl: fine movement.\nShift + mouse: ground; wheel: rotate; up/down: Z.",
 ["Glisser X/Y/Z : deplacer ; Ctrl : precision x10.\nMaj + souris : placer ; molette : pivoter.\nMaj + haut/bas : hauteur continue."]="Drag X/Y/Z: move; Ctrl: 10x precision.\nShift + mouse: place; wheel: rotate.\nShift + up/down: continuous height.",
 ["Recherche : nom ou identifiant"]="Search: name or entry ID",
 ["Placement GM"]="GM placement",
 ["Identifiant du modele (entry, pas GUID)"]="Object template (entry, not GUID)",
 ["Choisis un identifiant et clique sur Apercu."]="Choose an object in the catalog, or enter an ID.",
 ["Maj + souris : placer ; molette : pivoter de 1 deg.\nMaj + haut/bas : hauteur continue.\nAjouter Ctrl : precision x10 (0.1 deg)."]="Shift + mouse: place; wheel: rotate by 1 degree.\nShift + up/down: continuous height.\nAdd Ctrl: 10x precision (0.1 degree).",
 ["Lance le nouveau WowClient64.exe pour utiliser cet outil."]="Run the updated WowClient64.exe to use this tool.",
 ["Identifiant invalide."]="Invalid entry ID.",
 ["Verification de l'objet et des droits GM..."]="Checking the object and GM permissions...",
 ["Apercu"]="Preview", ["Rotation -1 deg"]="Rotate -1 deg", ["Rotation +1 deg"]="Rotate +1 deg",
 ["Hauteur -0.1"]="Height -0.1", ["Hauteur +0.1"]="Height +0.1", ["Valider"]="Place object",
 ["Creation et sauvegarde par le serveur..."]="Creating and saving on the server...",
 ["Annuler / Fermer"]="Cancel / Close",
 ["Objet sauvegarde. GUID : "]="Object saved. GUID: ",
 ["\nTu peux preparer un autre placement."]="\nYou can prepare another placement.",
 ["Objet cree et sauvegarde. GUID : "]="Object created and saved. GUID: ",
 [". Suppression : .gobject delete "]=". Delete: .gobject delete ",
 ["Reponse absente : verifie les objets proches avant de recommencer (.gobject near)."]="No reply: check nearby objects before trying again (.gobject near).",
 ["Pas de reponse. Verifie les droits GM et que le nouveau worldserver est lance."]="No reply. Check GM permissions and that the updated worldserver is running.",
 ["Apercu annule (changement de carte ou de session)."]="Preview cancelled (map or session changed).",
 ["Chargement trop long : verifie les donnees du client et l'identifiant."]="Loading timed out: check client data and the entry ID.",
 ["Chargement du modele..."]="Loading model...",
 ["Maintiens Maj sur le sol, a moins de 100 m."]="Hold Shift over the ground, within 100 yards.",
 ["Position valide. Ctrl : reglages dix fois plus fins."]="Valid position. Ctrl: ten times finer adjustments.",
 ["Maj detectee : place le curseur hors de l'interface."]="Shift held: move the cursor outside the interface.",
 ["Maj detectee : aucun sol valide a moins de 100 m."]="Shift held: no valid ground within 100 yards.",
 [" (WMO : apercu opaque)"]=" (WMO: opaque preview)",
 ["Catalogue"]="Catalog", ["Fermer"]="Close",
 ["Rechercher un nom, un identifiant ou un modèle"]="Search by name, entry ID or model",
 ["%d résultats — page %d / %d"]="%d results — page %d / %d",
 ["Objet introuvable ou personnage hors du monde."]="Object not found or character outside the world.",
 ["Cet objet ne possede pas de modele utilisable."]="This object has no usable model.",
 ["Impossible de charger le modele."]="Unable to load the model.",
 ["Modele absent des fichiers du client."]="Model missing from client files.",
 ["Identifiant inconnu ou modele absent."]="Unknown entry or missing model.",
 ["Les transports ne sont pas pris en charge par cet outil."]="Transports are not supported by this tool.",
 ["Carte differente, personnage hors du monde ou sur un transport."]="Different map, character outside the world or on a transport.",
 ["Coordonnees non finies."]="Coordinates must be finite numbers.",
 ["Position invalide ou a plus de 100 metres du personnage."]="Invalid position or more than 100 yards from the character.",
 ["Le serveur ne peut pas creer cet objet."]="The server cannot create this object.",
 ["Sauvegarde effectuee (GUID %s), chargement impossible. Verifie cet objet avant de recommencer."]="Saved (GUID %s), but loading failed. Check this object before trying again.",
}
function B.language()
 local saved=GMBlueprintSettings and GMBlueprintSettings.language
 if saved=="frFR" or saved=="enUS" then return saved end
 return GetLocale()=="frFR" and "frFR" or "enUS"
end
function B.T(key)
 if B.language()=="enUS" then return en[key] or key end
 return key
end
function B.error(text)
 local guid=text:match("^Sauvegarde effectuee %(GUID (%d+)%), chargement impossible%. Verifie cet objet avant de recommencer%.$")
 if guid then return string.format(B.T("Sauvegarde effectuee (GUID %s), chargement impossible. Verifie cet objet avant de recommencer."),guid) end
 return B.T(text)
end
