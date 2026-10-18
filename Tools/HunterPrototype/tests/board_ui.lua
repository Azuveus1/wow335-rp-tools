local frames,buttons={},{}
local methods={}
local function noop() end
setmetatable(methods,{__index=function()return noop end})
function methods:SetScript(k,v)self.scripts[k]=v end
function methods:GetScript(k)return self.scripts[k] end
function methods:SetText(t)self.text=t;if self.kind=="Button" then buttons[t]=self end end
function methods:GetText()return self.text or "" end
function methods:Show()self.shown=true end
function methods:Hide()local old=self.shown;self.shown=false;if old and self.scripts.OnHide then self.scripts.OnHide(self)end end
function methods:IsShown()return self.shown end
function methods:Enable()self.enabled=true end
function methods:Disable()self.enabled=false end
function methods:CreateFontString()return CreateFrame("FontString")end
function CreateFrame(kind,name,parent)
 local f=setmetatable({kind=kind,scripts={},shown=true,enabled=true,text=""},{__index=methods});frames[#frames+1]=f
 if name then _G[name]=f end;return f
end
UIParent=CreateFrame("Frame");UISpecialFrames={};tinsert=table.insert
local now,money,closed=0,5000000,0
local choices={}
local sent={}
function GetTime()return now end
function GetMoney()return money end
function GetGossipOptions()return unpack(choices) end
function CloseGossip()closed=closed+1 end
function HideUIPanel(f)f:Hide()end
function SelectGossipOption(index,code,confirmed)sent[#sent+1]={index,code,confirmed};choices={}end
GossipFrame=CreateFrame("Frame")
GossipFrame:SetScript("OnHide",CloseGossip)
dofile("Tools/HunterPrototype/addon/HunterContracts/HunterContracts.lua")
local p=HunterContractBoardFrame
local name,amount=HunterContractTargetInput,HunterContractGoldInput
local function show(message)
 choices={"Deposer un contrat de mercenaire","gossip",message or "Choisissez une cible et une prime.","gossip"}
 GossipFrame:Show();p.scripts.OnEvent(p,"GOSSIP_SHOW");p.scripts.OnUpdate(p,0)
end
show()
assert(p:IsShown() and amount:GetText()=="250" and closed==0,"Opening must preserve gossip and initialize amount")
name:SetText("Testeur");amount:SetText("250.5");buttons.Valider.scripts.OnClick();assert(#sent==0)
amount:SetText("1000");buttons.Valider.scripts.OnClick();assert(#sent==0,"Insufficient funds")
amount:SetText("250");buttons.Valider.scripts.OnClick();assert(#sent==1 and sent[1][2]=="Testeur;250" and sent[1][3]==1 and not buttons.Valider.enabled)
buttons.Valider.scripts.OnClick();assert(#sent==1,"Double click must be suppressed")
show("Desole, impossible : un contrat est deja en cours sur cette cible.")
assert(name:GetText()=="Testeur" and buttons.Valider.enabled and closed==0,"Rejection preserves entered fields")
show("Contrat cree. Prime reservee : 250 pieces d'or.")
assert(name:GetText()=="" and buttons.Valider.enabled)
choices={"Other NPC","gossip"};p.scripts.OnEvent(p,"GOSSIP_SHOW")
assert(not p:IsShown() and closed==0,"Another NPC must retain its own gossip interaction")
show();name:SetText("Testeur");amount:SetText("1");buttons.Valider.scripts.OnClick();assert(#sent==2)
now=11;p.scripts.OnUpdate(p,0);buttons.Valider.scripts.OnClick();assert(#sent==2 and closed==1,"Timeout cannot resubmit a stale menu")
p.scripts.OnEvent(p,"GOSSIP_CLOSED");assert(not p:IsShown())
show();buttons.Fermer.scripts.OnClick();assert(not p:IsShown() and closed==2)
print("PASS: board UI default values, validation, insufficient funds, one request, error retry, success, unrelated gossip, timeout and close.")
