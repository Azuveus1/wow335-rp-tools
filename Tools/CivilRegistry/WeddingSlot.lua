-- A server-backed cosmetic equipment slot, separate from both combat fingers.
local prefix='RPREG'
local own={item=0,engraving='',nonce=nil}
local inspected={id=nil,item=0,engraving=''}
local ownButton,inspectButton
local function send(text) SendAddonMessage(prefix,text,'WHISPER',UnitName('player')) end
local function id(unit)
 local guid=UnitGUID(unit) return guid and tonumber(guid:sub(-8),16) or nil
end
local function icon(button,item)
 if button then button.icon:SetTexture(item~=0 and (GetItemIcon(9100304) or 'Interface\\Icons\\INV_Jewelry_Ring_03') or 'Interface\\PaperDoll\\UI-PaperDoll-Slot-Finger') end
end
local function tooltip(button,state,personal)
 GameTooltip:SetOwner(button,'ANCHOR_RIGHT')
 GameTooltip:SetText(state.item~=0 and 'Alliance de mariage' or 'Emplacement d alliance',1,0.82,0)
 if state.item~=0 then GameTooltip:AddLine('Gravure : '..state.engraving,1,1,1,true) end
 GameTooltip:AddLine('Emplacement cosmetique, sans bonus de combat.',0.6,0.8,1,true)
 if personal then GameTooltip:AddLine(state.item~=0 and 'Clic droit : remettre l alliance dans les sacs.' or 'Clic : porter votre alliance gravee depuis les sacs ou un doigt.',0.8,0.8,0.8,true) end
 GameTooltip:Show()
end
local function create(parent,anchor,name,state,personal)
 local b=CreateFrame('Button',name,parent,'ItemButtonTemplate') b:SetSize(34,34)
 -- Dedicated space below the model rotation arrows, left of the character.
 b:SetPoint('TOPLEFT',anchor,'BOTTOMLEFT',12,-12)
 b:SetFrameLevel(parent:GetFrameLevel()+8)
 b.icon=b:CreateTexture(nil,'ARTWORK') b.icon:SetAllPoints(b)
 local label=b:CreateFontString(nil,'OVERLAY','GameFontNormalSmall') label:SetPoint('TOP',b,'BOTTOM',0,-2) label:SetText('Alliance')
 b:SetScript('OnEnter',function(self) tooltip(self,state,personal) end)
 b:SetScript('OnLeave',function() GameTooltip:Hide() end)
 if personal then
  b:RegisterForClicks('LeftButtonUp','RightButtonUp')
  local function equip()
   if not own.nonce then send('RINGVIEW|0') return end
   send('RINGEQUIP|'..own.nonce) own.nonce=nil
  end
  b:SetScript('OnClick',function(_,button)
   if own.item~=0 then
    if IsShiftKeyDown() then ChatEdit_InsertLink('|Hitem:9100304:0:0:0:0:0:0:0|h[Alliance gravee]|h')
    elseif button=='RightButton' and own.nonce then send('RINGREMOVE|'..own.nonce) own.nonce=nil end
   else equip() end
  end)
  b:SetScript('OnReceiveDrag',function()
   local kind,entry=GetCursorInfo() if kind=='item' and entry==9100304 and own.item==0 then ClearCursor() equip() end
  end)
 end
 icon(b,state.item) return b
end
local function inspect()
 if not InspectFrame or not InspectFrame:IsShown() then return end
 local unit=InspectFrame.unit or 'target'
 local target=id(unit) inspected.id=target inspected.item=0 inspected.engraving='' icon(inspectButton,0)
 if target then send('RINGVIEW|'..target) end
end
local function install()
 if not ownButton and PaperDollFrame and CharacterModelFrameRotateLeftButton then
  ownButton=create(PaperDollFrame,CharacterModelFrameRotateLeftButton,'CivilRegistryWeddingSlot',own,true)
  PaperDollFrame:HookScript('OnShow',function() own.nonce=nil send('RINGVIEW|0') end)
  if PaperDollFrame:IsShown() then send('RINGVIEW|0') end
 end
 if not inspectButton and InspectPaperDollFrame and InspectModelRotateLeftButton then
  inspectButton=create(InspectPaperDollFrame,InspectModelRotateLeftButton,'CivilRegistryInspectWeddingSlot',inspected,false)
  InspectFrame:HookScript('OnShow',inspect)
  -- Wrath uses INSPECT_TALENT_READY, not the newer INSPECT_READY event.
 end
end
local events=CreateFrame('Frame')
for _,event in ipairs({'PLAYER_LOGIN','ADDON_LOADED','INSPECT_TALENT_READY','PLAYER_TARGET_CHANGED','PARTY_MEMBERS_CHANGED','CHAT_MSG_ADDON'}) do events:RegisterEvent(event) end
events:SetScript('OnEvent',function(_,event,p,text,channel,sender)
 if event=='CHAT_MSG_ADDON' then
  if p~=prefix or channel~='WHISPER' or sender~=UnitName('player') then return end
  local kind,key,item,engraving=text:match('^(RING%u+)|(%d+)|(%d+)|(.*)$')
  if kind=='RINGSELF' then own.nonce=key own.item=tonumber(item) own.engraving=engraving icon(ownButton,own.item)
  elseif kind=='RINGOTHER' and tonumber(key)==inspected.id then inspected.item=tonumber(item) inspected.engraving=engraving icon(inspectButton,inspected.item) end
 elseif event=='INSPECT_TALENT_READY' or event=='PLAYER_TARGET_CHANGED' or event=='PARTY_MEMBERS_CHANGED' then install() inspect()
 else install() end
end)
install()
