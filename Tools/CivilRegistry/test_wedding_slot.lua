local frames,named,sent={},{},{}
local methods={}
function methods:SetScript(k,f) self.scripts[k]=f end
function methods:HookScript(k,f) self.scripts[k]=f end
function methods:CreateTexture() return CreateFrame('Texture') end
function methods:CreateFontString() return CreateFrame('Font') end
function methods:SetTexture(t) self.texture=t end
function methods:SetPoint(...) self.anchor={...} end
function methods:GetFrameLevel() return 1 end
function methods:Show() self.shown=true end
function methods:Hide() self.shown=false end
function methods:IsShown() return self.shown end
function methods:RegisterEvent(e) assert(e~='INSPECT_READY','Unsupported event on Wrath client') end
function CreateFrame(kind,name,parent)
 local f=setmetatable({scripts={},shown=true,parent=parent},{__index=function(_,k)return methods[k] or function()end end})
 frames[#frames+1]=f if name then named[name]=f end return f
end
PaperDollFrame=CreateFrame('Frame');CharacterModelFrameRotateLeftButton={};InspectPaperDollFrame=CreateFrame('Frame');InspectModelRotateLeftButton={}
InspectFrame=CreateFrame('Frame');InspectFrame.unit='target';GameTooltip=CreateFrame('Tooltip')
function UnitName() return 'Alice Clairbois' end
local target='00000006'
function UnitGUID(unit) return '0x00000000'..(unit=='player' and '00000005' or target) end
function GetItemIcon() return 'ring' end
function IsShiftKeyDown() return false end
function GetCursorInfo() return 'item',9100304 end
function ClearCursor() end
function SendAddonMessage(p,text,c,n) assert(p=='RPREG' and c=='WHISPER' and n==UnitName()) sent[#sent+1]=text end
dofile('Tools/CivilRegistry/WeddingSlot.lua')
local event=frames[#frames] -- install() creates buttons after event, so find its handler.
for _,f in ipairs(frames) do if f.scripts.OnEvent then event=f end end
local function recv(text,sender) event.scripts.OnEvent(event,'CHAT_MSG_ADDON','RPREG',text,'WHISPER',sender or UnitName()) end
local slot=named.CivilRegistryWeddingSlot;assert(slot and named.CivilRegistryInspectWeddingSlot)
assert(slot.anchor[1]=='TOPLEFT' and slot.anchor[2]==CharacterModelFrameRotateLeftButton and slot.anchor[3]=='BOTTOMLEFT' and named.CivilRegistryInspectWeddingSlot.anchor[2]==InspectModelRotateLeftButton,'Alliance must sit below the rotation arrows, left of the character')
assert(sent[1]=='RINGVIEW|0')
recv('RINGSELF|123|0|') slot.scripts.OnClick(slot,'LeftButton');assert(sent[#sent]=='RINGEQUIP|123')
slot.scripts.OnClick(slot,'LeftButton');assert(sent[#sent]=='RINGVIEW|0','Consumed nonce must not be reused')
recv('RINGSELF|124|77|Bertrand Clairbois');assert(slot.icon.texture=='ring')
slot.scripts.OnClick(slot,'RightButton');assert(sent[#sent]=='RINGREMOVE|124')
recv('RINGSELF|125|0|')
recv('RINGSELF|999|99|Spoof','Other player');slot.scripts.OnReceiveDrag();assert(sent[#sent]=='RINGEQUIP|125')
event.scripts.OnEvent(event,'INSPECT_TALENT_READY');assert(sent[#sent]=='RINGVIEW|6')
recv('RINGOTHER|6|78|Alice Belleplume');assert(named.CivilRegistryInspectWeddingSlot.icon.texture=='ring')
target='00000007';event.scripts.OnEvent(event,'PLAYER_TARGET_CHANGED');assert(sent[#sent]=='RINGVIEW|7')
recv('RINGOTHER|6|78|Old target');assert(named.CivilRegistryInspectWeddingSlot.icon.texture~='ring','Stale inspect response must be ignored')
print('PASS: actual wedding slot UI, dedicated equip/remove requests, nonce consumption, item drag, sender gate, Wrath inspect event and stale-target protection.')
