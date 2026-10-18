-- NPC announcements are attached to stable node IDs and exact creature spawn GUIDs.
GMTransportAnnouncements={busy=false}
local API=GMTransportAnnouncements
local function T(fr,en) return GMBlueprintUI.language()=="frFR" and fr or en end
local frame=CreateFrame("Frame","GMTransportAnnouncementsPanel",UIParent)
frame:SetSize(370,510);frame:SetPoint("RIGHT",GMTransportPanel,"LEFT",-6,0);frame:SetFrameStrata("DIALOG");frame:SetClampedToScreen(true);frame:EnableMouse(true)
frame:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border",tile=true,tileSize=32,edgeSize=32,insets={left=8,right=8,top=8,bottom=8}});frame:Hide()
tinsert(UISpecialFrames,"GMTransportAnnouncementsPanel")
local rows,current,entry={},nil,nil
local upload,index,token= nil,1,0
local widgets={}
local function label(y,height)
 local f=frame:CreateFontString(nil,"OVERLAY","GameFontNormal");f:SetPoint("TOPLEFT",20,y);f:SetSize(330,height or 22);f:SetJustifyH("LEFT");return f
end
local function button(fr,en,x,y,w,fn)
 local b=CreateFrame("Button",nil,frame,"UIPanelButtonTemplate");b:SetPoint("TOPLEFT",x,y);b:SetSize(w,24);b:SetText(T(fr,en));b:SetScript("OnClick",fn);widgets[#widgets+1]={b,fr,en};return b
end
local title=label(-18);local info=label(-46,18);local npc=label(-98,24);local node=label(-153,30)
local phase,mode,enabled
local delayLabel=label(-215)
local delay=CreateFrame("EditBox","GMTransportAnnouncementDelay",frame,"InputBoxTemplate");delay:SetPoint("TOPLEFT",260,-215);delay:SetSize(80,24);delay:SetAutoFocus(false);delay:SetMaxLetters(6)
local text=CreateFrame("EditBox","GMTransportAnnouncementText",frame);text:SetPoint("TOPLEFT",22,-250);text:SetSize(326,98);text:SetMultiLine(true);text:SetAutoFocus(false);text:SetMaxLetters(1000);text:SetFontObject(GameFontHighlightSmall)
text:SetBackdrop({bgFile="Interface\\Tooltips\\UI-Tooltip-Background"});text:SetBackdropColor(0,0,0,0.8)
text:SetScript("OnEscapePressed",function(self) self:ClearFocus() end)
delay:SetScript("OnEscapePressed",function(self) self:ClearFocus() end)
local status=label(-443,52)
local function send(s) SendChatMessage(".gmroute ann "..s,"SAY") end
local function context() return GMTransportUI.context() end
local function decode(s) return (s:gsub("%x%x",function(x)return string.char(tonumber(x,16)) end)) end
local function encode(s) return (s:gsub(".",function(x)return string.format("%02x",string.byte(x)) end)) end
local function refresh()
 title:SetText(T("Annonces des PNJ","NPC announcements"))
 for _,w in ipairs(widgets) do w[1]:SetText(T(w[2],w[3])) end
 delayLabel:SetText(T("Délai après événement (s)","Delay after event (s)"))
 if not current then return end
 info:SetText((current.original and T("Annonce Blizzard","Blizzard announcement") or current.id>0 and T("Annonce enregistrée","Saved announcement") or T("Nouvelle annonce","New announcement")).." — "..index.." / "..#rows)
 npc:SetText(current.selected and T("PNJ ciblé : sera lié à l'enregistrement","Targeted NPC: will be bound on save") or ((current.name or "PNJ / NPC").." — GUID "..tostring(current.spawn or 0)))
 local d=context();local number="?";for i,n in ipairs(d.nodes) do if (n[9] or i)==current.uid then number=tostring(i);break end end
 node:SetText(T("Point lié : ","Linked node: ")..number.." — "..T("carte ","map ")..tostring(current.map or d.map or 0))
 phase:SetText(current.phase==0 and T("À l'arrivée","On arrival") or T("Au départ","On departure"))
 mode:SetText(current.mode==1 and T("Crier","Yell") or T("Dire","Say"))
 enabled:SetText(current.enabled and T("Annonce active","Announcement enabled") or T("Annonce désactivée","Announcement disabled"))
end
local function showCurrent()
 current=rows[index];if current then text:SetText(current.text or "");delay:SetText(tostring((current.delay or 0)/1000)) end;refresh()
end
local function capture() if current then current.text=text:GetText();current.delay=(tonumber(delay:GetText()) or -1)*1000 end end
button("< Annonce","< Announcement",20,-65,155,function() if API.busy or #rows==0 then return end;capture();index=(index-2)%#rows+1;showCurrent() end)
button("Annonce >","Announcement >",195,-65,155,function() if API.busy or #rows==0 then return end;capture();index=index%#rows+1;showCurrent() end)
button("Lier le PNJ ciblé","Bind targeted NPC",20,-124,330,function() if API.busy or not current then return end;current.selected=true;current.name=UnitName("target");refresh() end)
button("Utiliser le point sélectionné","Use selected route node",20,-183,330,function()
 if API.busy or not current then return end;local d,selected=context();local n=d.nodes[selected or 0];if n then current.uid=n[9] or selected;current.map=n[5] or d.map;refresh() end
end)
phase=button("À l'arrivée","On arrival",20,-355,155,function() if not API.busy and current then current.phase=1-current.phase;refresh() end end)
mode=button("Crier","Yell",195,-355,155,function() if not API.busy and current then current.mode=1-current.mode;refresh() end end)
enabled=button("Annonce active","Announcement enabled",20,-384,330,function() if not API.busy and current then current.enabled=not current.enabled;refresh() end end)
button("Nouvelle","New",20,-411,90,function()
 if API.busy then return end;capture();local d,selected=context();local n=d.nodes[selected or 0];if not n then return end
 rows[#rows+1]={id=0,revision=0,uid=n[9] or selected,map=n[5] or d.map,phase=0,delay=0,mode=1,enabled=true,script=0,kind=0,action=0,text="",selected=true};index=#rows;showCurrent()
end)
button("Enregistrer","Save",115,-411,130,function()
 if API.busy or not current then return end
 local d=context();capture()
 if d.dirty or not d.editEntry or d.editEntry~=entry then status:SetText(T("Enregistre le trajet, puis rouvre les annonces.","Save the route, then reopen announcements."));return end
 local a=current
 if a.delay<0 or a.delay>600000 or a.delay~=math.floor(a.delay) or #a.text==0 or #a.text>1000 or a.text:find("[%c|]") then status:SetText(T("Délai : 0–600 s. Texte : 1–1000 octets, une ligne, sans |.","Delay: 0–600 s. Text: 1–1000 bytes, one line, no |."));return end
 token=math.max(token+1,math.floor(GetTime()*1000)%2000000000);upload={token=token,text=a.text,sent=0,started=GetTime()};API.busy=true
 send(string.format("begin %d %d %d %d %d %d %d %d %d %d %d %d %d %d",token,entry,d.revision or 0,a.id,a.revision,a.uid,a.phase,a.delay,a.mode,a.script,a.kind,a.action,a.enabled and 1 or 0,a.selected and 1 or 0))
 status:SetText(T("Enregistrement…","Saving…"));text:ClearFocus();delay:ClearFocus()
end)
button("Fermer","Close",250,-411,100,function() frame:Hide() end)
function API.close() frame:Hide() end
function API.open()
 if API.busy then frame:Show();return end
 local d=context();if not d.editEntry or d.dirty then DEFAULT_CHAT_FRAME:AddMessage(T("Enregistre d'abord le trajet et ses modifications.","Save the route and its changes first."));return end
 entry=d.editEntry;rows={};index=1;current=nil;frame:Show();refresh();send("list "..entry)
 status:SetText(T("Chargement… Les changements s'appliquent au redémarrage serveur.","Loading… Changes apply after a server restart."))
end
local events=CreateFrame("Frame");events:RegisterEvent("CHAT_MSG_SYSTEM")
events:SetScript("OnEvent",function(self,event,message)
 if type(message)~="string" then return end
 local data=message:match("^%[TR:(A.+)%]$");if not data then return end
 local f={};for w in data:gmatch("[^:]+") do f[#f+1]=w end;local id=tonumber(f[2]);local key=tonumber(f[3])
 if f[1]=="ALIST" and id==entry then rows={};current=nil
 elseif f[1]=="ANN" and id==entry then rows[key]={id=tonumber(f[4]),revision=tonumber(f[5]),uid=tonumber(f[6]),phase=tonumber(f[7]),delay=tonumber(f[8]),mode=tonumber(f[9]),spawn=f[10],map=tonumber(f[11]),script=tonumber(f[12]),kind=tonumber(f[13]),action=tonumber(f[14]),enabled=f[15]=="1",original=f[16]=="1",text=""}
 elseif f[1]=="ATEXT" and id==entry and rows[key] then rows[key].text=rows[key].text..decode(f[4] or "")
 elseif f[1]=="ANAME" and id==entry and rows[key] then rows[key].name=decode(f[4] or "")
 elseif f[1]=="ALISTEND" and id==entry then index=math.min(index,math.max(1,#rows));showCurrent();if #rows==0 then info:SetText(T("Aucune annonce détectée. Clique sur Nouvelle.","No announcement detected. Click New."));npc:SetText("");node:SetText("");text:SetText("") end
 elseif f[1]=="AACK" and upload and id==upload.token and key==upload.sent then
  upload.started=GetTime()
  if key==#upload.text then send("commit "..id)
  else local part=upload.text:sub(key+1,key+80);upload.sent=key+#part;send("text "..id.." "..key.." "..encode(part)) end
 elseif f[1]=="ASAVED" then API.busy=false;upload=nil;status:SetText(T("Enregistré. Redémarre le serveur pour appliquer les annonces.","Saved. Restart the server to apply announcements."))
 elseif f[1]=="AERROR" then API.busy=false;upload=nil;status:SetText(f[2] or "Error") end
end)
events:SetScript("OnUpdate",function() if upload and GetTime()-upload.started>20 then upload=nil;API.busy=false;status:SetText(T("Délai dépassé. Rouvre les annonces pour vérifier.","Timed out. Reopen announcements to verify.")) end end)
