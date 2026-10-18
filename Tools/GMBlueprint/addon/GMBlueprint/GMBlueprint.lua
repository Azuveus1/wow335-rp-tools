local B = GMBlueprintUI
local T = B.T
local localized = {}
-- Requires the native GMBlueprint functions and the matching TrinityCore commands.
local panel = CreateFrame("Frame", "GMBlueprintPanel", UIParent)
panel:SetSize(360, 425)
panel:SetPoint("CENTER", UIParent, "CENTER", 360, 0)
panel:SetFrameStrata("DIALOG")
panel:SetClampedToScreen(true)
panel:SetMovable(true)
panel:EnableMouse(true)
panel:RegisterForDrag("LeftButton")
panel:SetScript("OnDragStart", panel.StartMoving)
panel:SetScript("OnDragStop", panel.StopMovingOrSizing)
panel:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background", edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border", tile=true, tileSize=32, edgeSize=32, insets={left=8,right=8,top=8,bottom=8}})
panel:Hide()
tinsert(UISpecialFrames, "GMBlueprintPanel")

local pending, active, requestedEntry, lastSpawn, loadingStarted
local gizmoMode,gizmoModeButton=0,nil
local gizmoHide=function() end
local gizmoUpdate=function() end
local lastToken = 0
local function syncBindings()
    if type(GMBlueprintControls) == "function" then
        GMBlueprintControls(panel:IsShown() and active and not pending and not InCombatLockdown() or false)
    end
end
local function say(text) DEFAULT_CHAT_FRAME:AddMessage("|cff50dfffBlueprint :|r " .. text) end
local function label(text, y, height)
    local fs = panel:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    fs:SetPoint("TOPLEFT", 20, y)
    fs:SetSize(320, height or 24)
    fs:SetJustifyH("LEFT")
    fs:SetText(T(text))
    localized[#localized+1] = {widget=fs, key=text}
    return fs
end
label("Placement GM", -18)
label("Identifiant du modele (entry, pas GUID)", -47)
local entryBox = CreateFrame("EditBox", nil, panel, "InputBoxTemplate")
entryBox:SetSize(155, 26)
entryBox:SetPoint("TOPLEFT", 25, -72)
entryBox:SetAutoFocus(false)
entryBox:SetNumeric(true)
entryBox:SetMaxLetters(10)
local status = label("Choisis un identifiant et clique sur Apercu.", -108, 62)
label("Gizmo : axes, plans XY/XZ/YZ ou centre XYZ.\nBouton Gizmo : deplacer / rotation. Ctrl : precision.\nMaj + souris : sol ; molette : tourner ; haut/bas : Z.", -176, 50)
local function button(text, x, y, width, callback)
    local b = CreateFrame("Button", nil, panel, "UIPanelButtonTemplate")
    b:SetSize(width, 25)
    b:SetPoint("TOPLEFT", x, y)
    b:SetText(T(text))
    localized[#localized+1] = {widget=b, key=text}
    b:SetScript("OnClick", callback)
    return b
end
local function supported()
    if type(GMBlueprintControls) ~= "function" then
        say(T("Lance le nouveau WowClient64.exe pour utiliser cet outil."))
        return false
    end
    return true
end
local function cancel()
    gizmoHide()
    if type(GMBlueprintCancel) == "function" then GMBlueprintCancel() end
    active = false
    requestedEntry = nil
    syncBindings()
end
local function begin()
    if not supported() or pending then return end
    local entry = tonumber(entryBox:GetText())
    if not entry or entry < 1 or entry > 2147483647 or entry ~= math.floor(entry) then
        status:SetText(T("Identifiant invalide."))
        return
    end
    cancel()
    entryBox:ClearFocus()
    requestedEntry = entry
    pending = {kind="info", started=GetTime(), entry=entry}
    status:SetText(T("Verification de l'objet et des droits GM..."))
    SendChatMessage(".gobject blueprint info " .. string.format("%d", entry), "SAY")
end
local previewButton = button("Apercu", 195, -73, 140, begin)
B.preview = function(entry)
    if pending then return false end
    entryBox:SetText(tostring(entry))
    begin()
    return true
end
entryBox:SetScript("OnEnterPressed", begin)
local function adjust(angle, height)
    if active and not pending then GMBlueprintAdjust(angle, height) end
end
button("Rotation -1 deg", 20, -236, 155, function() adjust(-(IsControlKeyDown() and 0.1 or 1)*math.pi/180, 0) end)
button("Rotation +1 deg", 185, -236, 155, function() adjust((IsControlKeyDown() and 0.1 or 1)*math.pi/180, 0) end)
button("Hauteur -0.1", 20, -264, 155, function() adjust(0, IsControlKeyDown() and -0.01 or -0.1) end)
button("Hauteur +0.1", 185, -264, 155, function() adjust(0, IsControlKeyDown() and 0.01 or 0.1) end)
local validate = button("Valider", 20, -302, 155, function()
    if not active or pending then return end
    local entry, valid, loading, map, x, y, z, facing, name, err, wmo, qx, qy, qz, qw = GMBlueprintState()
    if not valid or loading or entry == 0 then return end
    local token = math.max(lastToken + 1, math.floor(GetTime() * 1000) % 2000000000)
    lastToken = token
    pending = {kind="place", token=token, started=GetTime()}
    syncBindings()
    status:SetText(T("Creation et sauvegarde par le serveur..."))
    -- No automatic retries: a timeout does not prove that the server rejected it.
    local command=string.format(".gobject blueprint place %d %d %.6f %.6f %.6f %.6f %d",entry,map,x,y,z,facing,token)
    if qx and qy and qz and qw then command=command..string.format(" %.9f %.9f %.9f %.9f",qx,qy,qz,qw) end
    SendChatMessage(command,"SAY")
end)
button("Annuler / Fermer", 185, -302, 155, function() panel:Hide() end)
button("Catalogue", 20, -340, 155, function() if B.toggleCatalog then B.toggleCatalog() end end)
gizmoModeButton=button("Gizmo : deplacer",185,-340,155,function()
    gizmoHide();gizmoMode=1-gizmoMode;gizmoModeButton:SetText(T(gizmoMode==0 and "Gizmo : deplacer" or "Gizmo : rotation"))
end)
local languageMenu = CreateFrame("Frame", "GMBlueprintLanguageDropdown", panel, "UIDropDownMenuTemplate")
languageMenu:SetPoint("TOPLEFT", 95, -377)
UIDropDownMenu_SetWidth(languageMenu, 160)
local function refreshLanguage()
    for _, item in ipairs(localized) do
        if item.widget ~= status then item.widget:SetText(T(item.key)) end
    end
    if gizmoModeButton then gizmoModeButton:SetText(T(gizmoMode==0 and "Gizmo : deplacer" or "Gizmo : rotation")) end
    UIDropDownMenu_SetText(languageMenu, B.language() == "frFR" and "Français" or "English")
    if pending then
        status:SetText(T(pending.kind == "place" and "Creation et sauvegarde par le serveur..." or "Verification de l'objet et des droits GM..."))
    elseif not active then
        status:SetText(T("Choisis un identifiant et clique sur Apercu."))
    end
    if B.catalogRefresh then B.catalogRefresh() end
end
UIDropDownMenu_Initialize(languageMenu, function()
    for _, locale in ipairs({"frFR", "enUS"}) do
        local code = locale
        local info = UIDropDownMenu_CreateInfo()
        info.text = code == "frFR" and "Français" or "English"
        info.checked = B.language() == code
        info.func = function()
            GMBlueprintSettings = GMBlueprintSettings or {}
            GMBlueprintSettings.language = code
            refreshLanguage()
        end
        UIDropDownMenu_AddButton(info)
    end
end)
panel:SetScript("OnShow", refreshLanguage)
panel:SetScript("OnHide", function()
    cancel()
    if pending and pending.kind == "info" then pending = nil end
    -- A submitted placement cannot be cancelled by hiding the local preview.
end)

-- Projected world axes. Only the arrow handles capture clicks; the world remains usable.
local gizmoFrame=CreateFrame("Frame","GMBlueprintGizmoFrame",UIParent)
gizmoFrame:SetAllPoints(UIParent);gizmoFrame:SetFrameStrata("MEDIUM");gizmoFrame:Hide()
local axes={}
local draggingAxis
local colors={{1,0.18,0.18},{0.2,1,0.25},{0.25,0.6,1},{1,0.85,0.15},{1,0.35,0.8},{0.2,1,1},{1,1,1}}
local function endDrag()
 if draggingAxis and type(GMBlueprintGizmoDrag)=="function" then GMBlueprintGizmoDrag(0) end
 draggingAxis=nil
end
gizmoHide=function() endDrag();gizmoFrame:Hide() end
gizmoFrame:SetScript("OnHide",endDrag)
local function axisWidget(axis)
 local data={dots={},hits={}};axes[axis]=data
 data.label=gizmoFrame:CreateFontString(nil,"OVERLAY","GameFontNormalLarge")
 data.label:SetText(({"X","Y","Z","XY","XZ","YZ","XYZ","X","Y","Z"})[axis])
 local c=colors[axis] or colors[axis-7];data.label:SetTextColor(c[1],c[2],c[3])
 return data
end
local function hit(data,index,axis,x,y)
 local b=data.hits[index]
 if not b then
  b=CreateFrame("Button",nil,gizmoFrame);b.axis=axis;b:SetSize(16,16);b:EnableMouse(true)
  b:SetScript("OnMouseDown",function(self,button)
   if button=="LeftButton" and active and not pending and not InCombatLockdown() and GMBlueprintGizmoDrag(axis) then draggingAxis=axis end
  end)
  b:SetScript("OnMouseUp",endDrag);data.hits[index]=b
 end
 b:ClearAllPoints();b:SetPoint("CENTER",gizmoFrame,"BOTTOMLEFT",x,y);b:Show()
end
local function dot(data,index,x,y,r,g,b,size)
 local t=data.dots[index]
 if not t then t=gizmoFrame:CreateTexture(nil,"ARTWORK");t:SetTexture("Interface\\Buttons\\WHITE8X8");data.dots[index]=t end
 t:ClearAllPoints();t:SetPoint("CENTER",gizmoFrame,"BOTTOMLEFT",x,y);t:SetSize(size,size);t:SetVertexColor(r,g,b,1);t:Show()
end
gizmoUpdate=function()
 if not active or pending or InCombatLockdown() or type(GMBlueprintGizmo)~="function" then gizmoHide();return end
 if draggingAxis and not IsMouseButtonDown("LeftButton") then endDrag() end
 local p,nativeAxis=GMBlueprintGizmo(gizmoMode)
 if not p then gizmoHide();return end
 if draggingAxis and nativeAxis==0 then endDrag() end
 local w,h=gizmoFrame:GetWidth(),gizmoFrame:GetHeight()
 local ox,oy=p[1]*w,p[2]*h
 if ox<0 or ox>w or oy<0 or oy>h then gizmoHide();return end
 gizmoFrame:Show()
 for id=1,10 do
  local visible=(gizmoMode==0 and id<=7) or (gizmoMode==1 and id>=8)
  local data=axes[id];if visible and not data then data=axisWidget(id) end
  if data then
   local used,hits=0,0;local labelX,labelY
   local c=draggingAxis==id and {1,0.9,0.2} or colors[id] or colors[id-7]
   local function point(v) if type(v)=="table" then return {v[1]*w,v[2]*h} end end
   local function onScreen(v) return v and v[1]>=8 and v[1]<=w-8 and v[2]>=8 and v[2]<=h-8 end
   local function segment(a,b)
    if not a or not b then return end
    local dx,dy=b[1]-a[1],b[2]-a[2];local length=math.sqrt(dx*dx+dy*dy)
    if length<0.01 or length>800 then return end
    for d=0,length,2 do
     local x,y=a[1]+dx*d/length,a[2]+dy*d/length
     if x>=1 and x<=w-1 and y>=1 and y<=h-1 then used=used+1;dot(data,used,x,y,c[1],c[2],c[3],3) end
    end
   end
   local function handle(v)
    if onScreen(v) then hits=hits+1;hit(data,hits,id,v[1],v[2]) end
   end
   if visible and id<=3 then
    local tip=point(p[id+2]);local origin={ox,oy}
    if tip then
     local dx,dy=tip[1]-ox,tip[2]-oy;local length=math.sqrt(dx*dx+dy*dy)
     if length>=18 and length<=400 and onScreen(tip) then
      local ux,uy=dx/length,dy/length
      segment({ox+ux*7,oy+uy*7},tip)
      for side=-1,1,2 do segment(tip,{tip[1]-ux*11-uy*6*side,tip[2]-uy*11+ux*6*side}) end
      for d=18,length,10 do handle({ox+ux*d,oy+uy*d}) end
      handle(tip);labelX=tip[1]+ux*13;labelY=tip[2]+uy*13
     end
    end
   elseif visible and id<=6 then
    local plane=p[id+2];local center=point(plane)
    if plane and onScreen(center) then
     local a,b,d=point(plane[3]),point(plane[4]),point(plane[6])
     local area=a and b and d and math.abs((b[1]-a[1])*(d[2]-a[2])-(b[2]-a[2])*(d[1]-a[1])) or 0
     if area>=30 then
      for i=3,6 do segment(point(plane[i]),point(plane[i==6 and 3 or i+1])) end
      handle(center);data.hits[hits]:SetSize(24,20);labelX=center[1];labelY=center[2]
     end
    end
   elseif visible and id==7 then
    local center=point(p[9])
    if onScreen(center) then
     local x,y=center[1],center[2];segment({x-12,y-10},{x+12,y-10});segment({x+12,y-10},{x+12,y+10});segment({x+12,y+10},{x-12,y+10});segment({x-12,y+10},{x-12,y-10})
     handle(center);data.hits[hits]:SetSize(30,24);labelX=x;labelY=y
    end
   elseif visible then
    local ring=p[10] and p[10][id-7]
    if ring then
     for i=1,#ring-1 do segment(point(ring[i]),point(ring[i+1]));if i%2==1 then handle(point(ring[i])) end end
     local label=point(ring[1]);if onScreen(label) then labelX=label[1]+10;labelY=label[2]+10 end
    end
   end
   if labelX then data.label:ClearAllPoints();data.label:SetPoint("CENTER",gizmoFrame,"BOTTOMLEFT",labelX,labelY);data.label:Show() else data.label:Hide() end
   for i=used+1,#data.dots do data.dots[i]:Hide() end
   for i=hits+1,#data.hits do data.hits[i]:Hide() end
  end
 end
end

-- Keep listening while hidden so a server acknowledgement is never mistaken for
-- a failed placement. The timer also remains active when the panel is closed.
local events = CreateFrame("Frame")
events:RegisterEvent("CHAT_MSG_SYSTEM")
events:RegisterEvent("PLAYER_LEAVING_WORLD")
events:SetScript("OnEvent", function(self, event, message)
    if event == "PLAYER_LEAVING_WORLD" then
        panel:Hide()
        cancel()
        pending = nil
        return
    end
    if not pending or type(message) ~= "string" then return end
    local ready = message:match("^%[WB:READY:(%d+)%]$")
    if ready and pending.kind == "info" and tonumber(ready) == pending.entry then
        local entry = pending.entry
        pending = nil
        if panel:IsShown() then
            active = GMBlueprintBegin(entry)
            syncBindings()
            loadingStarted = GetTime()
            requestedEntry = nil
        end
        return
    end
    local token, guid = message:match("^%[WB:OK:(%d+):(%d+)%]$")
    if token and pending.kind == "place" and tonumber(token) == pending.token then
        pending = nil
        lastSpawn = guid
        cancel()
        status:SetText(T("Objet sauvegarde. GUID : ") .. guid .. T("\nTu peux preparer un autre placement."))
        say(T("Objet cree et sauvegarde. GUID : ") .. guid .. T(". Suppression : .gobject delete ") .. guid)
        return
    end
    local reason = message:match("^%[WB:ERROR%] (.+)$")
    if reason then
        pending = nil
        status:SetText(B.error(reason))
        say(B.error(reason))
    end
end)
events:SetScript("OnUpdate", function(self, elapsed)
    syncBindings()
    if not panel:IsShown() or not active or pending or InCombatLockdown() then gizmoHide() end
    if pending and GetTime() - pending.started > 15 then
        local wasPlace = pending.kind == "place"
        pending = nil
        cancel()
        local text = wasPlace and T("Reponse absente : verifie les objets proches avant de recommencer (.gobject near).") or T("Pas de reponse. Verifie les droits GM et que le nouveau worldserver est lance.")
        status:SetText(text)
        say(text)
    end
    if not panel:IsShown() then return end
    if pending then
        validate:Disable()
        previewButton:Disable()
        return
    end
    previewButton:Enable()
    if not active then
        validate:Disable()
        return
    end
    local shiftDown = IsShiftKeyDown()
    local overWorld = GetMouseFocus() == WorldFrame
    GMBlueprintTick(shiftDown and overWorld and not draggingAxis, elapsed)
    gizmoUpdate()
    local entry, valid, loading, map, x, y, z, facing, name, err, wmo, qx, qy, qz, qw = GMBlueprintState()
    if B.objectName then name = B.objectName(entry) or name end
    -- Do not disable then enable every frame: that clears a held mouse click.
    if entry ~= 0 and valid and not loading and err == "" then
        validate:Enable()
    else
        validate:Disable()
    end
    if entry == 0 then
        active = false
        status:SetText(T("Apercu annule (changement de carte ou de session)."))
    elseif err ~= "" then
        status:SetText(B.error(err))
    elseif loading then
        if GetTime() - loadingStarted > 30 then
            cancel()
            status:SetText(T("Chargement trop long : verifie les donnees du client et l'identifiant."))
        else
            status:SetText(T("Chargement du modele..."))
        end
    else
        local hint = T("Maintiens Maj sur le sol, a moins de 100 m.")
        if valid then
            hint = T("Position valide. Ctrl : reglages dix fois plus fins.")
        elseif shiftDown and not overWorld then
            hint = T("Maj detectee : place le curseur hors de l'interface.")
        elseif shiftDown then
            hint = T("Maj detectee : aucun sol valide a moins de 100 m.")
        end
        local rotation=""
        if qx and qy and qz and qw then
            local roll=math.atan2(2*(qw*qx+qy*qz),1-2*(qx*qx+qy*qy))*180/math.pi
            local pitch=math.asin(math.max(-1,math.min(1,2*(qw*qy-qz*qx))))*180/math.pi
            rotation=string.format("\nRot. X %.1f / Y %.1f / Z %.1f deg",roll,pitch,facing*180/math.pi)
        end
        status:SetText(string.format("%s%s\nX %.2f  Y %.2f  Z %.2f%s\n%s",name,wmo and T(" (WMO : apercu opaque)") or "",x,y,z,rotation,hint))
    end
end)
SLASH_GMBLUEPRINT1 = "/gobuild"
SlashCmdList.GMBLUEPRINT = function(text)
    if not supported() then return end
    panel:Show()
    local entry = text:match("^%s*(%d+)%s*$")
    if entry then entryBox:SetText(entry); begin() end
end
