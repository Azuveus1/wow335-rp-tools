-- Execute the real 3.3.5 addon with only the rendering/transport API mocked.
local frames,byname,hooks,sends={},{},{},{}
UIParent={} UISpecialFrames={} GameTooltip={}
function UnitName(u) return u=='player' and 'Martin Vendacier' or 'Elise Clairbois' end
function UnitIsPlayer() return true end
function SendAddonMessage(p,t,c,s) sends[#sends+1]={p,t,c,s} end
UIErrorsFrame={AddMessage=function() end}
local methods={}
function methods:SetScript(k,v) self.scripts[k]=v end
function methods:HookScript(k,v) self.scripts[k]=v end
function methods:SetText(v) self.text=v end
function methods:GetText() return self.text or '' end
function methods:Show() self.shown=true end
function methods:Hide() self.shown=false end
function methods:IsShown() return self.shown end
function methods:CreateTexture() return CreateFrame('Texture') end
function methods:CreateFontString() return CreateFrame('Font') end
function methods:FamilyDesign(...) self.design={...} return unpack(self.design) end
function methods:FamilyBorder() return 'border' end
function methods:GetUpperBackgroundFileName() return 'background' end
function methods:GetLowerBackgroundFileName() return 'background' end
function methods:GetUpperEmblemFileName() return 'emblem' end
function methods:GetLowerEmblemFileName() return 'emblem' end
function CreateFrame(kind,name,parent)
 local f=setmetatable({kind=kind,scripts={},parent=parent},{__index=function(_,k) return methods[k] or function() end end})
 frames[#frames+1]=f if name then byname[name]=f end return f
end
function hooksecurefunc(_,name,fn) hooks[name]=fn end
function GetContainerItemLink() return '|Hitem:9100304:0|h[Alliance]|h' end
function GetInventoryItemLink() return '|Hitem:9100304:0|h[Alliance]|h' end
GameTooltip=CreateFrame('Tooltip')
dofile('Tools/CivilRegistry/CivilRegistry.lua')
local window=byname.CivilRegistryWindow
local events for _,f in ipairs(frames) do if f.scripts.OnEvent then events=f end end
local function receive(t,s) events.scripts.OnEvent(events,'CHAT_MSG_ADDON','RPREG',t,'WHISPER',s or UnitName('player')) end
local function button(t) for _,f in ipairs(frames) do if f.kind=='Button' and f.text==t then return f end end error(t) end
local input for _,f in ipairs(frames) do if f.kind=='EditBox' then input=f end end
assert(not window.shown)
receive('REG|123|Martin|0','Un autre joueur') assert(not window.shown)
receive('REG|123|Martin|0') assert(window.shown and input.shown)
input:SetText('Vendacier') button('Valider').scripts.OnClick() assert(sends[#sends][2]=='RENAME|123|Vendacier' and not window.shown)
receive('RINGS|124') input:SetText('Elise Clairbois') button('Valider').scripts.OnClick() assert(sends[#sends][2]=='RINGS|124|Elise Clairbois')
receive('DOC|125|Martin Vendacier||') assert(button('Demander la signature').shown)
button('Demander la signature').scripts.OnClick() assert(sends[#sends][2]=='OFFER|125')
receive('SIGN|126|Martin Vendacier|Vendacier') button('Signer et accepter').scripts.OnClick() assert(sends[#sends][2]=='SIGN|126')
receive('DOC|127|Martin Vendacier|Elise Clairbois|Vendacier') assert(not button('Demander la signature').shown)
receive('EMBLEM|128|15033779') button('Enregistrer le blason').scripts.OnClick() assert(sends[#sends][2]=='EMBLEM|128|169,16,5,16,50')
hooks.SetBagItem(GameTooltip,0,1) assert(sends[#sends][2]=='TIP|255,23')
hooks.SetBagItem(GameTooltip,1,1) assert(sends[#sends][2]=='TIP|19,0')
hooks.SetInventoryItem(GameTooltip,'player',11) assert(sends[#sends][2]=='TIP|255,10')
print('PASS: real addon forms, sender gate, input submission, signature and palette messages, backpack/bag/equipped ring slot mapping.')
