local function T(fr,en) return GMBlueprintUI.language()=="frFR" and fr or en end
local draft,selected,playing,upload,elapsedPreview,activeEntry
local incoming={}
local nativeIncoming={}
local loading
local catalogue={}
local library,refreshLibrary,saveButton
local loadRequested,loadStarted
local undoHistory={}
local dragging=false
local function copyNodes(nodes)
 local result={};for i,n in ipairs(nodes) do result[i]={};for k,v in pairs(n) do result[i][k]=v end end;return result
end
local function checkpoint()
 undoHistory[#undoHistory+1]={nodes=copyNodes(draft.nodes),speed=draft.speed,map=draft.map,source=draft.source,selected=selected,display=draft.display,scale=draft.scale,savedEntry=draft.savedEntry,dirty=draft.dirty}
 if #undoHistory>30 then table.remove(undoHistory,1) end
end
local function say(s) DEFAULT_CHAT_FRAME:AddMessage("|cff50dfffTransport:|r "..s) end
local panel=CreateFrame("Frame","GMTransportPanel",UIParent)
panel:SetSize(340,552); panel:SetPoint("RIGHT",UIParent,"RIGHT",-20,0); panel:SetFrameStrata("DIALOG")
panel:SetClampedToScreen(true); panel:SetMovable(true); panel:EnableMouse(true); panel:RegisterForDrag("LeftButton")
panel:SetScript("OnDragStart",panel.StartMoving); panel:SetScript("OnDragStop",panel.StopMovingOrSizing)
panel:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border",tile=true,tileSize=32,edgeSize=32,insets={left=8,right=8,top=8,bottom=8}})
panel:Hide(); tinsert(UISpecialFrames,"GMTransportPanel")
local overlay=CreateFrame("Frame",nil,UIParent); overlay:SetAllPoints(UIParent); overlay:SetFrameStrata("LOW"); overlay:Hide()
local markers,lines={},{}
local widgets={}
local function label(fr,en,y,height)
 local f=panel:CreateFontString(nil,"OVERLAY","GameFontNormal"); f:SetPoint("TOPLEFT",20,y); f:SetSize(300,height or 22); f:SetJustifyH("LEFT")
 widgets[#widgets+1]={f,fr,en}; f:SetText(T(fr,en)); return f
end
local function button(fr,en,x,y,w,fn)
 local b=CreateFrame("Button",nil,panel,"UIPanelButtonTemplate"); b:SetSize(w,24); b:SetPoint("TOPLEFT",x,y)
 b:SetScript("OnClick",fn); b:SetText(T(fr,en)); widgets[#widgets+1]={b,fr,en}; return b
end
local function edit(name,x,y,w)
 local b=CreateFrame("EditBox",name,panel,"InputBoxTemplate"); b:SetPoint("TOPLEFT",x,y); b:SetSize(w,24); b:SetAutoFocus(false); b:SetMaxLetters(16)
 b:SetScript("OnEscapePressed",function(self) self:ClearFocus() end); return b
end
label("Trajet de transport","Transport route",-18)
local modelLabel=label("","",-45,30)
local nodeLabel=label("","",-135)
label("X                     Y                     Z","X                     Y                     Z",-160)
local xBox,yBox,zBox=edit(nil,25,-183,88),edit(nil,125,-183,88),edit(nil,225,-183,88)
label("Arrêt (secondes)            Vitesse (unités/s)","Wait (seconds)              Speed (units/s)",-214)
local waitBox,speedBox=edit(nil,25,-239,120),edit(nil,185,-239,125)
local status=label("","",-414,66)
local function model()
 if draft.display then return {draft.source,draft.name or ("Transport "..(draft.editEntry or draft.source)),draft.display,draft.scale or 1} end
 for _,m in ipairs(GMTransportModels) do if m[1]==draft.source then return m end end
 draft.source=GMTransportModels[1][1]; return GMTransportModels[1]
end
local function refresh()
 for _,w in ipairs(widgets) do if w[2]~="" then w[1]:SetText(T(w[2],w[3])) end end
 for _,w in ipairs(widgets) do
    local locked=(w[2]=="Annuler modif." and #undoHistory==0) or (w[2]=="Se téléporter au point" and not draft.nodes[selected or 0]) or (draft.native and (w[2]=="Activer" or w[2]=="Retirer transport")) or
       (draft.editEntry and (w[2]=="< Modèle" or w[2]=="Modèle >"))
    if w[1].Enable then if locked then w[1]:Disable() else w[1]:Enable() end end
 end
 local m=model(); modelLabel:SetText(T(m[5] or m[2],m[2]).." ["..m[1].."]")
 nodeLabel:SetText(string.format(T("Point %d / %d — boucle fermée","Node %d / %d — closed loop"),selected or 0,#draft.nodes))
 if draft.nodes[selected or 0] then nodeLabel:SetText(string.format(T("Point %d / %d — carte %d","Node %d / %d — map %d"),selected or 0,#draft.nodes,draft.nodes[selected or 0] and (draft.nodes[selected][5] or draft.map or 0) or 0)) end
 if saveButton then saveButton:SetText(draft.editEntry and T("Mettre à jour","Update") or T("Sauvegarder","Save")) end
 local n=draft.nodes[selected or 0]
 if n and not draft.native then
    local nextNode=draft.nodes[selected%#draft.nodes+1]
    if (n[5] or draft.map)~=(nextNode[5] or draft.map) then
       nodeLabel:SetText(string.format(T("Point %d / %d — carte %d → %d","Node %d / %d — map %d → %d"),selected,#draft.nodes,n[5] or draft.map,nextNode[5] or draft.map))
    end
 end
 xBox:SetText(n and string.format("%.3f",n[1]) or ""); yBox:SetText(n and string.format("%.3f",n[2]) or ""); zBox:SetText(n and string.format("%.3f",n[3]) or "")
 waitBox:SetText(n and tostring(n[4]) or "0"); speedBox:SetText(tostring(draft.speed))
end
local function rebuild(changed)
 playing=false; elapsedPreview=0
 if changed then draft.savedEntry=nil; draft.dirty=true end
 if #draft.nodes>0 then draft.map=draft.nodes[1][5] or draft.map end
 local m=model(); local valid,duration
 if draft.native and GMRouteNativeDraft then valid,duration=GMRouteNativeDraft(draft.path,draft.speed,draft.accel,draft.nodes,m[3],m[4])
 else valid,duration=GMRouteDraft(draft.map or 0,draft.speed,draft.nodes,m[3],m[4]) end
 draft.valid=valid
 status:SetText(valid and string.format(T("Parcours prêt : %.1f s par tour.\nSauvegarder, puis Activer pour créer le transport.","Route ready: %.1f s per loop.\nSave, then Activate to create the transport."),duration) or T("Au moins 3 points ; 2 par portion de carte.\nEspacement sur une carte : 1 à 2000 m.","At least 3 nodes; 2 per map section.\nSame-map spacing: 1–2000 yards."))
 if draft.native and valid then status:SetText(T("Trajet DBC : ajout et suppression disponibles.\nMise à jour appliquée au redémarrage du serveur.\nTransitions et événements conservés.","DBC route: add and delete nodes.\nUpdates apply after a server restart.\nTransitions and events are preserved.")) end
 if not draft.native and draft.valid and selected then
    local n=draft.nodes[selected];local nextNode=draft.nodes[selected%#draft.nodes+1]
    if (n[5] or draft.map)~=(nextNode[5] or draft.map) then status:SetText(string.format(T("Transition : carte %d → carte %d.\nLe bateau et les passagers changent de carte.","Transition: map %d → map %d.\nThe ship and passengers transfer together."),n[5] or draft.map,nextNode[5] or draft.map)) end
 end
 refresh()
end
local function writable() return not upload and not loadRequested and not (GMTransportAnnouncements and GMTransportAnnouncements.busy) end
local function chooseModel(delta)
 if not writable() or draft.native or draft.editEntry then return end
 checkpoint();draft.display=nil
 local index=1; for i,m in ipairs(GMTransportModels) do if m[1]==draft.source then index=i end end
 draft.source=GMTransportModels[(index-1+delta)%#GMTransportModels+1][1]; rebuild(true)
end
button("< Modèle","< Model",20,-80,140,function() chooseModel(-1) end)
button("Modèle >","Model >",180,-80,140,function() chooseModel(1) end)
button("Ajouter / Ctrl insérer","Add / Ctrl insert",20,-110,140,function()
 if not writable() then return end
 local map,x,y,z=GMRoutePosition(); if not map then return end
 if #draft.nodes>=(draft.native and 1024 or 128) then say(T("Nombre maximal de points atteint.","Maximum node count reached.")); return end
 checkpoint()
 if #draft.nodes==0 then draft.map=map end
 local index=IsControlKeyDown() and selected and selected+1 or #draft.nodes+1
 for i,n in ipairs(draft.nodes) do draft.nextUid=math.max(draft.nextUid or 0,n[9] or i) end
 draft.nextUid=math.max(draft.nextUid or 0,math.floor(time()))+1
 table.insert(draft.nodes,index,{x,y,z,0,map,0,0,0,draft.nextUid}); selected=index; rebuild(true)
end)
button("Déplacer ici","Move here",180,-110,140,function()
 if not writable() or not selected then return end
 local map,x,y,z=GMRoutePosition(); local n=draft.nodes[selected]; if map~=(n[5] or draft.map) then return end
 checkpoint();n[1]=x;n[2]=y;n[3]=z; rebuild(true)
end)
local function apply()
 if not writable() then return end
 local n=draft.nodes[selected or 0]; local speed=tonumber(speedBox:GetText()); local wait=tonumber(waitBox:GetText())
 local x,y,z=tonumber(xBox:GetText()),tonumber(yBox:GetText()),tonumber(zBox:GetText())
 if not speed or speed<1 or speed>40 or not wait or wait<0 or wait>600 or (n and (not x or not y or not z or math.abs(x)>17000 or math.abs(y)>17000 or math.abs(z)>10000)) then
    say(T("Coordonnées, attente ou vitesse invalides.","Invalid coordinates, wait or speed.")); return
 end
 if draft.native and (speed~=math.floor(speed) or wait~=math.floor(wait) or (n and n[6]==1 and wait>0)) then
    say(T("Valeurs entières ; pas d’arrêt sur un marqueur de transition.","Whole numbers; transition markers cannot be stops."));return
 end
 checkpoint();draft.speed=speed; if n then n[1]=x;n[2]=y;n[3]=z;n[4]=wait;if draft.native and wait>0 then n[6]=2 end end
 for _,box in ipairs({xBox,yBox,zBox,waitBox,speedBox}) do box:ClearFocus() end
 rebuild(true)
end
for _,box in ipairs({xBox,yBox,zBox,waitBox,speedBox}) do box:SetScript("OnEnterPressed",apply) end
button("Appliquer","Apply",20,-270,90,apply)
button("Annuler modif.","Undo change",115,-270,100,function()
 if not writable() then return end
 local old=table.remove(undoHistory)
 if not old then refresh();return end
 draft.nodes=old.nodes;draft.speed=old.speed;draft.map=old.map;draft.source=old.source;draft.display=old.display;draft.scale=old.scale;selected=old.selected
 draft.savedEntry=old.savedEntry;draft.dirty=old.dirty
 dragging=false;rebuild(false)
end)
StaticPopupDialogs.GMTRANSPORT_DELETE={text="Supprimer ce point ? Ses annonces seront désactivées à la sauvegarde. Vérifie les transitions et arrêts. / Delete this node? Its announcements will be disabled on save. Check transitions and stops.",button1=YES,button2=NO,timeout=0,whileDead=true,hideOnEscape=true,
 OnAccept=function() if not writable() or not selected then return end;checkpoint();table.remove(draft.nodes,selected);selected=#draft.nodes>0 and math.min(selected,#draft.nodes) or nil;rebuild(true) end}
button("Supprimer point","Delete node",220,-270,100,function()
 if not writable() or not selected then return end
 StaticPopup_Show("GMTRANSPORT_DELETE")
end)
button("< Point","< Node",20,-300,140,function() if #draft.nodes>0 then selected=((selected or 1)-2)%#draft.nodes+1; refresh() end end)
button("Point >","Node >",180,-300,140,function() if #draft.nodes>0 then selected=(selected or 0)%#draft.nodes+1; refresh() end end)
button("Aperçu / Stop","Preview / Stop",180,-332,140,function() if not upload and draft.valid then playing=not playing; if not playing then GMRouteClear() end end end)
local function send(command) SendChatMessage(command,"SAY") end
button("Se téléporter au point","Teleport to node",20,-332,140,function()
 if not writable() or not selected then return end
 local n=draft.nodes[selected]; if not n then return end
 local map=n[5] or draft.map
 local function finite(v) return type(v)=="number" and v==v and math.abs(v)<math.huge end
 if not finite(map) or map<0 or map~=math.floor(map) or not finite(n[1]) or not finite(n[2]) or not finite(n[3]) then return end
 playing=false;GMRouteClear()
 send(string.format(".go xyz %.6f %.6f %.6f %d",n[1],n[2],n[3],map))
end)

local lastToken=0
saveButton=button("Sauvegarder","Save",20,-364,140,function()
 if not writable() or not draft.valid then return end

 playing=false; GMRouteClear()
 lastToken=math.max(lastToken+1,math.floor(GetTime()*1000)%2000000000)
 upload={token=lastToken,index=0,started=GetTime()}
 if draft.editEntry then
    send(string.format(".gmroute beginupdate %d %d %d %d %.3f %d",upload.token,draft.editEntry,draft.revision or 0,draft.map or 0,draft.speed,#draft.nodes))
 else send(string.format(".gmroute begin %d %d %d %.3f %d",upload.token,draft.source,draft.map,draft.speed,#draft.nodes)) end
 status:SetText(T("Enregistrement… Attends la confirmation.","Saving… Wait for confirmation."))
end)
button("Activer","Activate",180,-364,140,function()
 if draft.native then say(T("Transport Blizzard : mise à jour au redémarrage du serveur.","Blizzard transport: updates apply after server restart."));return end
 if upload or not draft.savedEntry then say(T("Enregistre d'abord ce trajet.","Save this route first.")); return end
 send(".gmroute activate "..draft.savedEntry)
end)
label("Maj + souris : déplacer à hauteur fixe.","Shift + mouse: move at fixed height.",-390)
button("Nouveau","New",20,-517,85,function()
 if not writable() then return end
 -- Keep the previous draft until the user confirms replacing it.
 StaticPopup_Show("GMTRANSPORT_NEW")
end)
button("Retirer transport","Remove transport",110,-517,125,function()
 if draft.native then say(T("Ce bouton concerne les transports personnalisés.","This button is for custom transports."));return end
 local entry=draft.activeEntry or draft.savedEntry
 if not upload and entry then send(".gmroute deactivate "..entry) end
end)
button("Fermer","Close",240,-517,80,function() panel:Hide() end)
StaticPopupDialogs.GMTRANSPORT_NEW={text="Remplacer le brouillon ? / Replace the draft?",button1=YES,button2=NO,timeout=0,whileDead=true,hideOnEscape=true,
 OnAccept=function() if not writable() then return end; GMTransportDraft={nodes={},speed=10,source=GMTransportModels[1][1]};draft=GMTransportDraft;undoHistory={};dragging=false; activeEntry=nil;selected=nil; rebuild(true) end}
panel:SetScript("OnHide",function() playing=false; overlay:Hide(); if library then library:Hide() end; if GMTransportAnnouncements then GMTransportAnnouncements.close() end; if GMRouteClear then GMRouteClear() end end)

local function line(i,a,b,width,height,control)
 local t=lines[i]; if not t then t=overlay:CreateTexture(nil,"BACKGROUND"); t:SetTexture("Interface\\AddOns\\GMTransport\\Line"); t:SetVertexColor(0,0.9,1,0.85); lines[i]=t end
 if not a or not b then t:Hide(); return end
 -- Clip links to the viewport; both endpoints may be outside the screen.
 local dx,dy=b[1]-a[1],b[2]-a[2]
 local lo,hi=0,1
 local ps={-dx,dx,-dy,dy};local qs={a[1],1-a[1],a[2],1-a[2]}
 for k=1,4 do
    if math.abs(ps[k])<0.000000001 then if qs[k]<0 then t:Hide();return end
    else
        local v=qs[k]/ps[k]
        if ps[k]<0 then lo=math.max(lo,v) else hi=math.min(hi,v) end
        if lo>hi then t:Hide();return end
    end
 end
 b={a[1]+dx*hi,a[2]+dy*hi};a={a[1]+dx*lo,a[2]+dy*lo}
 if control then t:SetVertexColor(0.65,0.65,0.65,0.35) else t:SetVertexColor(0,0.9,1,0.85) end
 local x1,y1,x2,y2=a[1]*width,a[2]*height,b[1]*width,b[2]*height
 t:ClearAllPoints(); t:SetPoint("BOTTOMLEFT",overlay,"BOTTOMLEFT",math.min(x1,x2),math.min(y1,y2))
 t:SetSize(math.max(2,math.abs(x2-x1)),math.max(2,math.abs(y2-y1)))
 if (x2-x1)*(y2-y1)>=0 then t:SetTexCoord(0,1,0,1) else t:SetTexCoord(1,0,0,1) end
 t:Show()
end
local events=CreateFrame("Frame"); events:RegisterEvent("CHAT_MSG_SYSTEM"); events:RegisterEvent("PLAYER_ENTERING_WORLD"); events:RegisterEvent("PLAYER_LEAVING_WORLD")
events:SetScript("OnEvent",function(self,event,message)
 if event=="PLAYER_LEAVING_WORLD" then panel:Hide(); return end
 if event=="PLAYER_ENTERING_WORLD" then self.syncAt=GetTime()+3; return end
 if type(message)~="string" then return end
 local data=message:match("^%[TR:(.+)%]$")
 if data then
    local f={}; for word in data:gmatch("[^:]+") do f[#f+1]=word end
    local entry=tonumber(f[2])
    if f[1]=="LISTBEGIN" then catalogue={}
    elseif f[1]=="ITEM" then catalogue[#catalogue+1]={entry=entry,kind=tonumber(f[3]),revision=tonumber(f[4]),name=f[5]}
    elseif f[1]=="LISTEND" then if refreshLibrary then refreshLibrary() end
    elseif f[1]=="EDIT" and entry==loadRequested then
        loading={editEntry=entry,source=tonumber(f[3]),native=tonumber(f[4])==1,revision=tonumber(f[5]),map=tonumber(f[6]),speed=tonumber(f[7]),display=tonumber(f[8]),scale=tonumber(f[9]),count=tonumber(f[10]),activeEntry=tonumber(f[11])==1 and entry or nil,accel=tonumber(f[12]),path=tonumber(f[13]),nodes={}}
        for _,item in ipairs(catalogue) do if item.entry==entry then loading.name=item.name end end
    elseif f[1]=="ENODE" and loading and entry==loading.editEntry then
        loading.nodes[tonumber(f[3])]={tonumber(f[4]),tonumber(f[5]),tonumber(f[6]),tonumber(f[7]),tonumber(f[8]),tonumber(f[9]),tonumber(f[10]),tonumber(f[11]),tonumber(f[12]) or tonumber(f[3])}
    elseif f[1]=="EEND" and loading and entry==loading.editEntry then
        if #loading.nodes==loading.count then
            loading.nextUid=tonumber(f[3]) or 0;draft=loading;undoHistory={};dragging=false;GMTransportDraft=draft;draft.savedEntry=entry;draft.dirty=false;selected=1;activeEntry=draft.activeEntry
            local map=GMRoutePosition(); for i,n in ipairs(draft.nodes) do if n[5]==map then selected=i;break end end
            rebuild(false);library:Hide()
        end
        loading=nil;loadRequested=nil
    elseif f[1]=="NATIVE" then nativeIncoming[entry]={path=tonumber(f[3]),speed=tonumber(f[4]),accel=tonumber(f[5]),period=tonumber(f[6]),count=tonumber(f[7]),nodes={}}
    elseif f[1]=="NNODE" and nativeIncoming[entry] then nativeIncoming[entry].nodes[tonumber(f[3])]={tonumber(f[4]),tonumber(f[5]),tonumber(f[6]),tonumber(f[7]),tonumber(f[8]),tonumber(f[9]),tonumber(f[10]),tonumber(f[11])}
    elseif f[1]=="NEND" and nativeIncoming[entry] then
        local r=nativeIncoming[entry];if #r.nodes==r.count and GMRouteNativeDefine then GMRouteNativeDefine(entry,r.path,r.speed,r.accel,r.period,r.nodes) end;nativeIncoming[entry]=nil
    elseif f[1]=="BEGIN" then incoming[entry]={map=tonumber(f[4]),speed=tonumber(f[5]),count=tonumber(f[7]),nodes={}}
    elseif f[1]=="NODE" and incoming[entry] then incoming[entry].nodes[tonumber(f[3])]={tonumber(f[4]),tonumber(f[5]),tonumber(f[6]),tonumber(f[7]),tonumber(f[8]) or incoming[entry].map}
    elseif f[1]=="END" and incoming[entry] then local r=incoming[entry]; if #r.nodes==r.count and GMRouteDefine then GMRouteDefine(entry,r.map,r.speed,r.nodes) end; incoming[entry]=nil
    elseif f[1]=="ACK" and upload and entry==upload.token then
        local index=tonumber(f[3]); if index~=upload.index then return end
        upload.started=GetTime()
        if index==#draft.nodes then upload.index=-1; send(".gmroute commit "..entry)
        else
            index=index+1; upload.index=index; local n=draft.nodes[index]
            send(string.format(".gmroute node %d %d %.6f %.6f %.6f %.3f %d %d %d %d %d",entry,index,n[1],n[2],n[3],n[4],n[5] or draft.map,n[6] or 0,n[7] or 0,n[8] or 0,n[9] or index))
        end
    elseif f[1]=="SAVED" and upload and entry==upload.token then
        draft.savedEntry=tonumber(f[3]);draft.editEntry=draft.savedEntry;draft.revision=0;draft.dirty=false;undoHistory={}; upload=nil;refresh()
        status:SetText(T("Trajet sauvegardé. Clique sur Activer. Entry : ","Route saved. Click Activate. Entry: ")..draft.savedEntry)
    elseif f[1]=="UPDATED" and upload and entry==upload.token then
        draft.savedEntry=tonumber(f[3]);draft.editEntry=draft.savedEntry;draft.revision=tonumber(f[4]);draft.dirty=false;undoHistory={};upload=nil
        status:SetText(tonumber(f[5])==1 and T("Enregistré. Redémarre le serveur pour appliquer.\nLe transport actuel conserve son trajet jusque-là.","Saved. Restart the server to apply.\nThe live transport keeps its current route until then.") or T("Transport mis à jour sous le même identifiant.","Transport updated under the same entry."));refresh()
    elseif f[1]=="ACTIVE" then activeEntry=entry; if draft then draft.activeEntry=entry end; say(T("Transport actif, entry : ","Transport active, entry: ")..entry); status:SetText(T("Transport actif : ","Transport active: ")..entry)
    elseif f[1]=="INACTIVE" then activeEntry=nil; if draft then draft.activeEntry=nil end; status:SetText(T("Transport retiré. Le trajet reste sauvegardé.","Transport removed. The route remains saved.")) end
 elseif message:find("[TR:ERROR]",1,true)==1 then upload=nil;loading=nil;loadRequested=nil; say(message); status:SetText(message) end
end)
local drawElapsed=0
events:SetScript("OnUpdate",function(self,elapsed)
 if self.syncAt and GetTime()>=self.syncAt then self.syncAt=nil; if GMRouteDefine then send(".gmroute sync") end end
 if upload and GetTime()-upload.started>20 then upload=nil; status:SetText(T("Réponse absente. Ne réactive pas sans vérifier.","No response. Check before activating again.")) end
 if loadRequested and loadStarted and GetTime()-loadStarted>30 then loadRequested=nil;loading=nil;say(T("Chargement expiré. Réessaie.","Loading timed out. Try again.")) end
 if not IsShiftKeyDown() or GetMouseFocus()~=WorldFrame then dragging=false end
 if not panel:IsShown() or not GMRouteTick then return end
 elapsedPreview=(elapsedPreview or 0)+elapsed; drawElapsed=drawElapsed+elapsed; if drawElapsed<0.04 then return end; drawElapsed=0
 if not upload and not loadRequested and not playing and selected and IsShiftKeyDown() and GetMouseFocus()==WorldFrame and GMRouteCursor then
    local n=draft.nodes[selected];local map=GMRoutePosition();local x,y
    if map==(n[5] or draft.map) then x,y=GMRouteCursor(n[3]) end; if x then local n=draft.nodes[selected]; if math.abs(x-n[1])+math.abs(y-n[2])>0.1 then if not dragging then checkpoint();dragging=true end;n[1]=x;n[2]=y;rebuild(true) end end
 end
 local points,curve=GMRouteTick(elapsedPreview,playing); elapsedPreview=0
 if not points then overlay:Hide(); return end
 overlay:Show(); local w,h=overlay:GetWidth(),overlay:GetHeight()
 for i=1,math.max(#markers,#points) do
    local b=markers[i]
    if not b then b=CreateFrame("Button",nil,overlay,"UIPanelButtonTemplate"); b:SetSize(30,24); b.index=i; b:SetScript("OnClick",function(self) selected=self.index; refresh() end); markers[i]=b end
    local p=points[i]
    if p and p[1]>=0 and p[1]<=1 and p[2]>=0 and p[2]<=1 then b:ClearAllPoints(); b:SetPoint("CENTER",overlay,"BOTTOMLEFT",p[1]*w,p[2]*h); b:SetText((selected==i and "|cffffff00" or "|cff00ffff")..i.."|r"); b:Show() else b:Hide() end
 end
 local used=0
 for i=1,#points do
    if #points>1 and (draft.nodes[i][5] or draft.map)==(draft.nodes[i%#points+1][5] or draft.map) and (not draft.native or (i<#points and draft.nodes[i][6]~=1)) then used=used+1;line(used,points[i],points[i%#points+1],w,h,true) end
 end
 for i=1,#curve-1 do used=used+1;line(used,curve[i],curve[i+1],w,h,false) end
 for i=used+1,#lines do lines[i]:Hide() end
end)
-- Catalogue shows database templates, including native routes and animated elevators.
library=CreateFrame("Frame","GMTransportLibrary",UIParent);library:SetSize(330,390)
library:SetPoint("RIGHT",panel,"LEFT",-6,0);library:SetFrameStrata("DIALOG");library:SetClampedToScreen(true);library:EnableMouse(true)
library:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border",tile=true,tileSize=32,edgeSize=32,insets={left=8,right=8,top=8,bottom=8}});library:Hide()
local title=library:CreateFontString(nil,"OVERLAY","GameFontNormal");title:SetPoint("TOPLEFT",16,-18)
local search=CreateFrame("EditBox","GMTransportSearch",library,"InputBoxTemplate");search:SetPoint("TOPLEFT",20,-48);search:SetSize(285,24);search:SetAutoFocus(false);search:SetMaxLetters(60)
local page=1;local rows={};local pageLabel=library:CreateFontString(nil,"OVERLAY","GameFontNormal");pageLabel:SetPoint("BOTTOMLEFT",18,48)
local function requestLoad(entry)
 if not writable() then return end
 loadRequested=entry;loadStarted=GetTime();loading=nil;send(".gmroute edit "..entry)
end
StaticPopupDialogs.GMTRANSPORT_LOAD={text="Remplacer le brouillon par ce trajet ? / Replace the draft with this route?",button1=YES,button2=NO,timeout=0,whileDead=true,hideOnEscape=true,
 OnAccept=function(self,data) requestLoad(data or (self and self.data)) end}
local pendingSelection
-- Store only the selected entry in the dialog, never discard a draft before a complete response.
StaticPopupDialogs.GMTRANSPORT_LOAD.OnAccept=function() if pendingSelection then requestLoad(pendingSelection) end end
for i=1,8 do
 local b=CreateFrame("Button",nil,library,"UIPanelButtonTemplate");b:SetSize(295,29);b:SetPoint("TOPLEFT",17,-80-(i-1)*31);rows[i]=b
 b:SetScript("OnClick",function(self)
    if not self.item or upload then return end
    if self.item.kind==2 then say(T("Ce modèle n'a pas de trajet TaxiPath éditable (ascenseur animé ou modèle sans trajet).","This template has no editable TaxiPath (animated elevator or model without a route)."));return end
    pendingSelection=self.item.entry
    if draft.dirty and #draft.nodes>0 then StaticPopup_Show("GMTRANSPORT_LOAD") else requestLoad(pendingSelection) end
 end)
end
refreshLibrary=function()
 local filtered={};local query=string.lower(search:GetText() or "")
 for _,item in ipairs(catalogue) do if query=="" or string.find(string.lower(item.name),query,1,true) or string.find(tostring(item.entry),query,1,true) then filtered[#filtered+1]=item end end
 local pages=math.max(1,math.ceil(#filtered/8));page=math.min(page,pages)
 title:SetText(T("Transports — rechercher nom / entry","Transports — search name / entry"))
 for i,b in ipairs(rows) do local item=filtered[(page-1)*8+i];b.item=item
    if item then b:SetText((item.kind==0 and "[GM] " or item.kind==2 and "[—] " or "[WoW] ")..item.name.." ["..item.entry.."]");b:Show() else b:Hide() end
 end
 pageLabel:SetText(string.format("%d / %d — %d",page,pages,#filtered))
end
search:SetScript("OnTextChanged",function() page=1;refreshLibrary() end)
search:SetScript("OnEscapePressed",function(self) self:ClearFocus() end)
local function libButton(text,x,w,fn) local b=CreateFrame("Button",nil,library,"UIPanelButtonTemplate");b:SetPoint("BOTTOMLEFT",x,16);b:SetSize(w,24);b:SetText(text);b:SetScript("OnClick",fn);return b end
libButton("<",17,40,function() page=math.max(1,page-1);refreshLibrary() end)
libButton(">",62,40,function() page=page+1;refreshLibrary() end)
local reload=libButton("",112,95,function() send(".gmroute list") end)
local closeLibrary=libButton("",217,95,function() library:Hide() end)
button("Catalogue des transports","Transport catalogue",20,-482,185,function()
 if library:IsShown() then library:Hide();return end
 library:Show();reload:SetText(T("Actualiser","Refresh"));closeLibrary:SetText(T("Fermer","Close"));refreshLibrary();send(".gmroute list")
end)
ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM",function(self,event,message) return message:match("^%[TR:[A-Z]+:")~=nil end)
SLASH_GMTRANSPORT1="/gmroute"
SlashCmdList.GMTRANSPORT=function()
 if not GMRouteDraft then say(T("Lance le client mis à jour.","Run the updated client.")); return end
 if #GMTransportModels==0 then say("No transport models."); return end
 if GMBlueprintPanel then GMBlueprintPanel:Hide() end
 GMTransportDraft=GMTransportDraft or {nodes={},speed=10,source=GMTransportModels[1][1]}
 if draft~=GMTransportDraft then undoHistory={};selected=nil end
 draft=GMTransportDraft; selected=#draft.nodes>0 and math.min(selected or 1,#draft.nodes) or nil
 for i,n in ipairs(draft.nodes) do if n[5]==nil then n[5]=draft.map end;n[9]=n[9] or i;draft.nextUid=math.max(draft.nextUid or 0,n[9]) end
 panel:Show(); rebuild(false); send(".gmroute sync")
end

GMTransportUI={context=function() return draft,selected end}
button("Annonces PNJ","NPC speech",210,-482,110,function() if not writable() then return end;if library then library:Hide() end;GMTransportAnnouncements.open() end)
