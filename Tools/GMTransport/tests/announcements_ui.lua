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

function UnitName() return "Test NPC" end
dofile("Tools/GMTransport/addon/GMTransport/Announcements.lua")
local function event(message)
 for _,f in ipairs(frames) do if f.events.CHAT_MSG_SYSTEM and f.scripts.OnEvent then f.scripts.OnEvent(f,"CHAT_MSG_SYSTEM",message) end end
end
local function click(label) assert(buttons[label],label);buttons[label].scripts.OnClick(buttons[label]) end
GMTransportDraft={source=20808,speed=20,map=1,editEntry=175080,revision=3,nodes={{0,0,20,0,1,0,0,0,11},{100,0,20,0,1,0,0,0,22},{100,100,20,0,1,0,0,0,33}}}
SlashCmdList.GMTRANSPORT();click("Annonces PNJ");assert(sent[#sent]==".gmroute ann list 175080")
event("[TR:ALIST:175080]");event("[TR:ANN:175080:1:0:0:22:0:21000:1:3463:1:17508000:9:0:1:1]")
event("[TR:ATEXT:175080:1:416c6c2061626f61726421]");event("[TR:ANAME:175080:1:536e75726b]");event("[TR:ALISTEND:175080]")
assert(GMTransportAnnouncementText:GetText()=="All aboard!")
assert(GMTransportAnnouncementDelay:GetText()=="21")
click("À l'arrivée");click("Utiliser le point sélectionné");click("Lier le PNJ ciblé")
local message="Annonce : départ du bateau. "..string.rep("é",90)
GMTransportAnnouncementText:SetText(message);GMTransportAnnouncementDelay:SetText("2.5");click("Enregistrer")
local command=sent[#sent]
local token=tonumber(command:match("^%.gmroute ann begin (%d+) 175080 3 0 0 11 1 2500 1 17508000 9 0 1 1$"));assert(token,command)
assert(GMTransportAnnouncements.busy)
local count=#sent;click("Ajouter / Ctrl insérer");assert(#GMTransportDraft.nodes==3 and #sent==count,"Route mutations are locked during announcement upload")
event("[TR:AACK:"..token..":0]")
local reconstructed="";local offset=0
while offset<#message do
 local c=sent[#sent];local start,hex=c:match("^%.gmroute ann text "..token.." (%d+) (%x+)$");assert(tonumber(start)==offset,c)
 assert(#c<=255,"Chat command must fit protocol limit")
 reconstructed=reconstructed..hex:gsub("%x%x",function(x)return string.char(tonumber(x,16)) end)
 offset=math.min(offset+80,#message);count=#sent;event("[TR:AACK:"..token..":"..offset.."]")
 if offset<#message then local after=#sent;event("[TR:AACK:"..token..":"..(offset-80).."]");assert(#sent==after,"Duplicate ACK must not resend") end
end
assert(reconstructed==message and sent[#sent]==".gmroute ann commit "..token)
event("[TR:ASAVED:175080]");assert(not GMTransportAnnouncements.busy)
event("[TR:AERROR:Test refused]");assert(not GMTransportAnnouncements.busy)
click("Nouvelle");GMTransportAnnouncementText:SetText("bad|text");count=#sent;click("Enregistrer");assert(#sent==count,"Reject WoW control codes")
GMTransportAnnouncementText:SetText("Hello");click("Enregistrer");assert(GMTransportAnnouncements.busy)
now=now+30;for _,f in ipairs(frames) do if f.scripts.OnUpdate then f.scripts.OnUpdate(f,0.1) end end
assert(not GMTransportAnnouncements.busy,"Timeout releases upload lock")
locale="enUS";GMTransportAnnouncements.open();event("[TR:ALIST:175080]");event("[TR:ALISTEND:175080]");click("New");assert(buttons["Use selected route node"] and buttons["Bind targeted NPC"])
print("PASS: announcement listing, native action identity, exact NPC binding request, selected stable node, phases, fractional delay, UTF-8 chunks, ACK checks, route lock, invalid text, timeout, French/English")
