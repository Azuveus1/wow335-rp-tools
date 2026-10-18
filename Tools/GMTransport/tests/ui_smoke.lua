local frames,buttons,sent,definitions={},{},{},{}
local now=100
local px,py,pz,map=0,0,20,1
local locale="frFR"
local function noop() end
local methods={}
setmetatable(methods,{__index=function() return noop end})
function methods:SetScript(k,f) self.scripts[k]=f end
function methods:RegisterEvent(k) self.events[k]=true end
function methods:SetText(s) self.text=s; if self.kind=="Button" then buttons[s]=self end end
function methods:GetText() return self.text end
function methods:Show() self.shown=true end
function methods:Hide() self.shown=false; if self.scripts.OnHide then self.scripts.OnHide(self) end end
function methods:IsShown() return self.shown end
function methods:GetWidth() return 1000 end
function methods:GetHeight() return 700 end
function methods:SetSize(w,h) self.width=w;self.height=h end
function methods:CreateFontString() return CreateFrame("FontString") end
function methods:CreateTexture() return CreateFrame("Texture") end
function CreateFrame(kind,name)
 local f=setmetatable({kind=kind,text="",shown=true,scripts={},events={},syncAt=false},{__index=methods})
 frames[#frames+1]=f; if name then _G[name]=f end; return f
end
UIParent=CreateFrame("Frame"); WorldFrame=CreateFrame("Frame")
GMBlueprintUI={language=function() return locale end}
DEFAULT_CHAT_FRAME={AddMessage=noop}; UISpecialFrames={}; SlashCmdList={}; StaticPopupDialogs={}; YES="Yes";NO="No"
tinsert=table.insert
function GetTime() return now end
function time() return 1790000000 end
function IsControlKeyDown() return false end
function IsShiftKeyDown() return false end
function GetMouseFocus() return WorldFrame end
function ChatFrame_AddMessageEventFilter() end
function StaticPopup_Show(name) StaticPopupDialogs[name].OnAccept() end
function SendChatMessage(message,kind) assert(kind=="SAY");sent[#sent+1]=message end
function GMRoutePosition() return map,px,py,pz end
function GMRouteClear() end
function GMRouteDraft(m,s,n,d,z) return #n>=3,40 end
local testPoints,testCurve={},{}
function GMRouteTick() return testPoints,testCurve end
function GMRouteDefine(entry,m,s,n) definitions[entry]={map=m,speed=s,nodes=n}; return true end
dofile("Tools/GMTransport/addon/GMTransport/TransportModels.lua")
-- Authored UI fixture, independent of extracted game data.
GMTransportModels = {{20808,"Test boat",3015,1,"Bateau de test"}}
dofile("Tools/GMTransport/addon/GMTransport/GMTransport.lua")
local events; for _,f in ipairs(frames) do if f.events.CHAT_MSG_SYSTEM then events=f end end
local function event(text) events.scripts.OnEvent(events,"CHAT_MSG_SYSTEM",text) end
local function click(text) assert(buttons[text],text); buttons[text].scripts.OnClick(buttons[text]) end
SlashCmdList.GMTRANSPORT()
assert(sent[#sent]==".gmroute sync")
local count=#sent; click("Activer");assert(#sent==count,"Cannot activate unsaved draft")
click("Ajouter / Ctrl insérer");px=100;click("Ajouter / Ctrl insérer");py=100;click("Ajouter / Ctrl insérer")
assert(#GMTransportDraft.nodes==3)
map=0;click("Ajouter / Ctrl insérer");assert(#GMTransportDraft.nodes==4 and GMTransportDraft.nodes[4][5]==0,"Record cross-map arrival");click("Annuler modif.");assert(#GMTransportDraft.nodes==3,"Undo inserted point")
map=1
click("Sauvegarder")
local token=tonumber(sent[#sent]:match("^%.gmroute begin (%d+)"));assert(token)
count=#sent;click("Sauvegarder");click("Supprimer point");assert(#sent==count and #GMTransportDraft.nodes==3)
event("[TR:ACK:999:0]");assert(#sent==count)
event("[TR:ACK:"..token..":0]");assert(sent[#sent]:find(".gmroute node "..token.." 1 ",1,true)==1)
count=#sent;event("[TR:ACK:"..token..":0]");assert(#sent==count,"Duplicate ACK must not resend")
event("[TR:ACK:"..token..":1]");event("[TR:ACK:"..token..":2]");event("[TR:ACK:"..token..":3]")
assert(sent[#sent]==".gmroute commit "..token)
event("[TR:BEGIN:9000000:3015:1:10.000000:1.000000:3]")
event("[TR:NODE:9000000:1:0:0:20:0]");event("[TR:NODE:9000000:2:100:0:20:0]")
assert(not definitions[9000000],"Incomplete route must not publish")
event("[TR:NODE:9000000:3:100:100:20:0]");event("[TR:END:9000000]")
assert(definitions[9000000].nodes[2][1]==100)
event("[TR:SAVED:"..token..":9000000]");assert(GMTransportDraft.savedEntry==9000000)
click("Activer");assert(sent[#sent]==".gmroute activate 9000000")
click("Déplacer ici");assert(not GMTransportDraft.savedEntry,"Editing creates a new revision")
click("Sauvegarder");count=#sent;now=now+21;events.scripts.OnUpdate(events,0.1);assert(#sent==count,"Timeout must not retry")
GMTransportPanel:Hide();SlashCmdList.GMTRANSPORT();assert(#GMTransportDraft.nodes==3,"Reopen must preserve draft")
locale="enUS";GMTransportPanel:Hide();SlashCmdList.GMTRANSPORT();assert(buttons["Save"] and buttons["Add / Ctrl insert"])
testPoints={{0.5,0.5},{-1,0.5},{2,0.5}}
testCurve={{-1,0.5},{2,0.5}}
events.scripts.OnUpdate(events,0.1)
local visibleMarker=false;local fullWidthLine=false
for _,f in ipairs(frames) do
 if f.kind=="Button" and rawget(f,"index")==1 and f.shown then visibleMarker=true end
 if f.kind=="Texture" and f.shown and f.width==1000 then fullWidthLine=true end
 if f.kind=="Texture" and f.shown then assert(f.width<=1000 and f.height<=700) end
end
assert(visibleMarker,"Markers must render while animated preview is off")
assert(fullWidthLine,"Link crossing the viewport must remain visible with both nodes off-screen")
print("PASS: route draft, map guard, upload ACKs, duplicate protection, sync, activation, revision, timeout, localization")
-- Reopen saved custom route from the server, without losing the draft on partial replies.
locale="frFR";GMTransportPanel:Hide();SlashCmdList.GMTRANSPORT()
click("Catalogue des transports");assert(sent[#sent]==".gmroute list")
event("[TR:LISTBEGIN]");event("[TR:ITEM:9000000:0:2:GM Test]");event("[TR:ITEM:20808:1:0:Boat]");event("[TR:ITEM:123:2:0:Lift]");event("[TR:LISTEND]")
local function choose(entry)
 for _,f in ipairs(frames) do if rawget(f,"item") and f.item.entry==entry then f.scripts.OnClick(f);return end end
 error("Missing catalogue entry "..entry)
end
count=#sent;choose(123);assert(#sent==count,"Animated elevators must not be offered as waypoint routes")
choose(9000000);assert(sent[#sent]==".gmroute edit 9000000")
local old=GMTransportDraft
event("[TR:EDIT:9000000:20808:0:2:1:10:3015:1:3:1:0:0]")
event("[TR:ENODE:9000000:1:0:0:20:0:1:0:0:0]");assert(GMTransportDraft==old,"Partial response must not replace draft")
event("[TR:ENODE:9000000:2:100:0:20:0:1:0:0:0]");event("[TR:ENODE:9000000:3:100:100:20:0:1:0:0:0]");event("[TR:EEND:9000000]")
assert(GMTransportDraft.editEntry==9000000 and GMTransportDraft.revision==2 and GMTransportDraft.activeEntry==9000000)
px=5;click("Déplacer ici");click("Mettre à jour")
token=tonumber(sent[#sent]:match("^%.gmroute beginupdate (%d+) 9000000 2 "));assert(token,"Must update existing identity with loaded revision")
for i=0,3 do event("[TR:ACK:"..token..":"..i.."]") end
assert(sent[#sent]==".gmroute commit "..token)
event("[TR:UPDATED:"..token..":9000000:3:0]");assert(GMTransportDraft.revision==3 and not GMTransportDraft.dirty)
-- Native route spans two maps; retain transition/event data and edit existing stops only.
local nativeDraftCalls=0;local nativeDefines={}
function GMRouteNativeDraft(path,speed,accel,nodes,display,scale) nativeDraftCalls=nativeDraftCalls+1;assert(path==77 and accel==1);return true,100 end
function GMRouteNativeDefine(entry,path,speed,accel,period,nodes) nativeDefines[entry]=nodes;return true end
click("Catalogue des transports");choose(20808)
event("[TR:EDIT:20808:20808:1:0:1:20:3015:1:4:0:1:77]")
for i=1,4 do event("[TR:ENODE:20808:"..i..":"..(i*100)..":0:20:5:"..(i<3 and 1 or 0)..":2:99:100]") end
event("[TR:EEND:20808]");assert(GMTransportDraft.native and nativeDraftCalls==1)
local oldUID=GMTransportDraft.nodes[1][9]
function IsControlKeyDown() return true end
click("Ajouter / Ctrl insérer");assert(#GMTransportDraft.nodes==5 and GMTransportDraft.nodes[1][9]==oldUID and GMTransportDraft.nodes[2][9]>1000)
click("Supprimer point");assert(#GMTransportDraft.nodes==4,"Native inserted points can be deleted")
click("Annuler modif.");assert(#GMTransportDraft.nodes==5,"Deletion is undoable")
click("Supprimer point");click("< Point")
function IsControlKeyDown() return false end
local edits={};for _,f in ipairs(frames) do if f.kind=="EditBox" then edits[#edits+1]=f end end
edits[1]:SetText("150");edits[4]:SetText("10");click("Appliquer")
assert(GMTransportDraft.nodes[1][1]==150 and GMTransportDraft.nodes[1][4]==10 and GMTransportDraft.nodes[1][5]==1 and GMTransportDraft.nodes[1][7]==99,"Editing must retain map and event metadata")
click("Mettre à jour");token=tonumber(sent[#sent]:match("^%.gmroute beginupdate (%d+) 20808 0 "));assert(token)
for i=0,4 do event("[TR:ACK:"..token..":"..i.."]") end
event("[TR:UPDATED:"..token..":20808:1:1]");assert(GMTransportDraft.revision==1)
count=#sent;click("Activer");assert(#sent==count,"Native updates apply at server restart, never spawn a duplicate")
event("[TR:NATIVE:20808:77:20:1:100000:4]")
for i=1,4 do event("[TR:NNODE:20808:"..i..":"..(i*100)..":0:20:5:1:2:99:100]") end
event("[TR:NEND:20808]");assert(nativeDefines[20808] and #nativeDefines[20808]==4)
click("Nouveau");assert(not GMTransportDraft.editEntry and not GMTransportDraft.native and #GMTransportDraft.nodes==0)
print("PASS: catalogue, partial load, same-entry update, revisions, native maps/events/stops, restart-only activation, native sync, new draft isolation")
-- Teleport uses the selected route node, including map 0, without changing it.
count=#sent;click("Se téléporter au point");assert(#sent==count,"No teleport without a node")
GMTransportDraft={source=GMTransportModels[1][1],speed=10,map=1,nodes={{10,20,30,0},{40,50,60,0}}}
SlashCmdList.GMTRANSPORT();click("Point >");click("Se téléporter au point")
assert(sent[#sent]==".go xyz 40.000000 50.000000 60.000000 1")
assert(GMTransportDraft.nodes[2][1]==40 and not GMTransportDraft.dirty)
GMTransportDraft.native=true;GMTransportDraft.path=77;GMTransportDraft.accel=1
GMTransportDraft.nodes={{10,20,30,0,1,2,0,0},{-10,-20,300,0,0,2,0,0}}
SlashCmdList.GMTRANSPORT();click("Se téléporter au point")
assert(sent[#sent]==".go xyz -10.000000 -20.000000 300.000000 0","Use selected native node map, not player's map")
GMTransportDraft.nodes[2][1]=0/0;count=#sent;click("Se téléporter au point");assert(#sent==count,"Reject non-finite coordinates")
locale="enUS";SlashCmdList.GMTRANSPORT();assert(buttons["Teleport to node"])
print("PASS: selected-node teleport, custom/native map selection, map zero, no-node and invalid coordinate guards, bilingual label")
locale="frFR";map=1;px=0;py=0;pz=20
GMTransportDraft={source=GMTransportModels[1][1],speed=10,map=1,nodes={{0,0,20,0,1},{100,0,20,0,1},{100,100,20,0,1}}}
SlashCmdList.GMTRANSPORT()
edits[1]:SetText("42");edits[5]:SetText("12");click("Appliquer")
assert(GMTransportDraft.nodes[1][1]==42 and GMTransportDraft.speed==12)
click("Annuler modif.");assert(GMTransportDraft.nodes[1][1]==0 and GMTransportDraft.speed==10,"Undo coordinates and speed together")
local cursorX=10
function IsShiftKeyDown() return true end
function GMRouteCursor(z) assert(z==20);return cursorX,25 end
events.scripts.OnUpdate(events,0.1);cursorX=30;events.scripts.OnUpdate(events,0.1)
assert(GMTransportDraft.nodes[1][1]==30)
function IsShiftKeyDown() return false end
events.scripts.OnUpdate(events,0.1);click("Annuler modif.")
assert(GMTransportDraft.nodes[1][1]==0 and GMTransportDraft.nodes[1][2]==0,"One undo restores the whole drag")
map=0;px=-9000;click("Ajouter / Ctrl insérer");px=-8900;click("Ajouter / Ctrl insérer")
map=571;px=1000;click("Ajouter / Ctrl insérer");px=1100;click("Ajouter / Ctrl insérer")
assert(#GMTransportDraft.nodes==7 and GMTransportDraft.nodes[4][5]==0 and GMTransportDraft.nodes[6][5]==571)
events.scripts.OnEvent(events,"PLAYER_LEAVING_WORLD");events.scripts.OnEvent(events,"PLAYER_ENTERING_WORLD");SlashCmdList.GMTRANSPORT()
assert(#GMTransportDraft.nodes==7 and GMTransportDraft.nodes[4][5]==0,"Map transfers preserve draft")
click("Sauvegarder");token=tonumber(sent[#sent]:match("^%.gmroute begin (%d+)"));assert(token)
for i=0,3 do event("[TR:ACK:"..token..":"..i.."]") end
assert(sent[#sent]:match(" 0 0 0 0 %d+$"),"Upload contains destination map 0")
for i=4,5 do event("[TR:ACK:"..token..":"..i.."]") end
assert(sent[#sent]:match(" 571 0 0 0 %d+$"),"Upload contains third map")
for i=6,7 do event("[TR:ACK:"..token..":"..i.."]") end
event("[TR:SAVED:"..token..":9000001]");local savedNodes=GMTransportDraft.nodes;click("Annuler modif.");assert(GMTransportDraft.nodes==savedNodes,"Save clears undo history")
event("[TR:BEGIN:9000001:3015:1:10:1:3]")
event("[TR:NODE:9000001:1:0:0:20:0:1]");event("[TR:NODE:9000001:2:100:0:20:0:0]");event("[TR:NODE:9000001:3:100:100:20:0:571]");event("[TR:END:9000001]")
assert(definitions[9000001].nodes[2][5]==0 and definitions[9000001].nodes[3][5]==571,"Sync retains all maps")
print("PASS: undo coordinates/speed, one undo per drag, three-map draft, world transfers, node-map upload/sync, save history boundary")
