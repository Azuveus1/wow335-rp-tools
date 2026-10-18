local prefix='RPREG'
local function send(text) SendAddonMessage(prefix,text,'WHISPER',UnitName('player')) end
local function split(text)
 local out={} for bit in (text..'|'):gmatch('(.-)|') do out[#out+1]=bit end return out
end
local function label(parent,text,x,y,width)
 local f=parent:CreateFontString(nil,'OVERLAY','GameFontNormal')
 f:SetPoint('TOPLEFT',x,y) f:SetWidth(width or 332) f:SetJustifyH('LEFT') f:SetText(text) return f
end
local function button(parent,text,x,y,width,fn)
 local f=CreateFrame('Button',nil,parent,'UIPanelButtonTemplate') f:SetSize(width or 155,24)
 f:SetPoint('TOPLEFT',x,y) f:SetText(text) f:SetScript('OnClick',fn) return f
end
local window=CreateFrame('Frame','CivilRegistryWindow',UIParent)
window:SetSize(390,400) window:SetPoint('CENTER') window:SetFrameStrata('DIALOG') window:EnableMouse(true)
window:SetBackdrop({bgFile='Interface\\DialogFrame\\UI-DialogBox-Background',edgeFile='Interface\\DialogFrame\\UI-DialogBox-Border',tile=true,tileSize=32,edgeSize=32,insets={left=11,right=12,top=12,bottom=11}})
window:SetMovable(true) window:RegisterForDrag('LeftButton') window:SetScript('OnDragStart',window.StartMoving) window:SetScript('OnDragStop',window.StopMovingOrSizing)
table.insert(UISpecialFrames,'CivilRegistryWindow')
local close=CreateFrame('Button',nil,window,'UIPanelCloseButton') close:SetPoint('TOPRIGHT',-6,-6)
local title=label(window,'Registre civil',22,-22)
local body=label(window,'',22,-58,346) body:SetFontObject('GameFontHighlight')
local input=CreateFrame('EditBox',nil,window,'InputBoxTemplate') input:SetSize(332,26) input:SetPoint('TOPLEFT',30,-160) input:SetAutoFocus(false) input:SetMaxLetters(25)
local token,action,mode
local function submit()
 local value=input:GetText():gsub('^%s+',''):gsub('%s+$','')
 if value=='' or value:find('[|\t\r\n]') then UIErrorsFrame:AddMessage('Saisissez un nom valide.',1,0.2,0.2) return end
 send(action..'|'..token..'|'..value) window:Hide()
end
local accept=button(window,'Valider',22,-208,168,submit)
local offer=button(window,'Demander la signature',22,-208,220,function() send('OFFER|'..token) window:Hide() end)
local decline=button(window,'Refuser',204,-208,160,function() window:Hide() end)
local sign=button(window,'Signer et accepter',22,-208,176,function() send('SIGN|'..token) window:Hide() end)
input:SetScript('OnEnterPressed',submit) input:SetScript('OnEscapePressed',function() window:Hide() end)
-- A tabard palette using the same native five variations and textures as the
-- guild editor. It never opens or saves the guild's design.
local palette=CreateFrame('Frame',nil,window) palette:SetAllPoints(window)
local model=CreateFrame('TabardModel',nil,palette) model:SetSize(1,1) model:SetPoint('BOTTOMLEFT') model:Hide()
local design,counts={0,0,0,0,0},{170,17,6,17,51}
local names={'Embleme','Couleur de l embleme','Bordure','Couleur de la bordure','Fond'}
local values={}
local upper,lower={},{}
for layer=1,3 do
 upper[layer]=palette:CreateTexture(nil,layer==1 and 'BACKGROUND' or layer==2 and 'ARTWORK' or 'OVERLAY')
 lower[layer]=palette:CreateTexture(nil,layer==1 and 'BACKGROUND' or layer==2 and 'ARTWORK' or 'OVERLAY')
 upper[layer]:SetSize(128,128) upper[layer]:SetPoint('TOPLEFT',25,-92)
 lower[layer]:SetSize(128,64) lower[layer]:SetPoint('TOPLEFT',25,-220)
end
local function refresh()
 model:FamilyDesign(unpack(design))
 upper[1]:SetTexture(model:GetUpperBackgroundFileName()) lower[1]:SetTexture(model:GetLowerBackgroundFileName())
 upper[2]:SetTexture(model:GetUpperEmblemFileName()) lower[2]:SetTexture(model:GetLowerEmblemFileName())
 upper[3]:SetTexture(model:FamilyBorder(true)) lower[3]:SetTexture(model:FamilyBorder(false))
 for i=1,5 do values[i]:SetText(names[i]..' : '..(design[i]+1)..' / '..counts[i]) end
end
for i=1,5 do
 local index=i local y=-95-(i-1)*43
 values[i]=label(palette,'',164,y,205) values[i]:SetFontObject('GameFontHighlightSmall')
 button(palette,'<',166,y-16,70,function() design[index]=(design[index]-1)%counts[index] refresh() end)
 button(palette,'>',251,y-16,70,function() design[index]=(design[index]+1)%counts[index] refresh() end)
end
button(palette,'Enregistrer le blason',22,-340,220,function() send('EMBLEM|'..token..'|'..table.concat(design,',')) window:Hide() end)
local function reset()
 input:Hide() accept:Hide() offer:Hide() decline:Hide() sign:Hide() palette:Hide()
 window:SetHeight(290) window:Show()
end
local function receive(text)
 local f=split(text) local kind=f[1]
 if kind=='TIP' then
  if GameTooltip:IsShown() and GameTooltip.registryKey==f[2] and not GameTooltip.registryGraved then GameTooltip:AddLine('Gravure : '..(f[3] or ''),1,0.82,0,true) GameTooltip.registryGraved=true GameTooltip:Show() end return
 end
 if not ({REG=true,DOC=true,SIGN=true,EMBLEM=true,RINGS=true})[kind] then return end
 reset() token=f[2] mode=kind
 if kind=='REG' then
  title:SetText('Registre des noms')
  body:SetText('Nom actuel : '..f[3]..'\n\nNouveau nom de famille : '..(f[4]=='0' and 'premiere inscription gratuite.' or '10 po, une fois tous les 30 jours.\nPour un couple marie, les deux noms changent.'))
  action='RENAME' input:SetMaxLetters(12) input:SetText('') input:Show() accept:Show()
 elseif kind=='RINGS' then
  title:SetText('Deux alliances gravees — 10 po')
  body:SetText('Saisissez le nom complet de votre partenaire.\n\nVous recevrez deux alliances : une a votre nom pour votre partenaire, et une a son nom pour vous. Elles sont echangeables.')
  action='RINGS' input:SetMaxLetters(25) input:SetText(UnitIsPlayer('target') and UnitName('target') or '') input:Show() accept:Show()
 elseif kind=='DOC' then
  title:SetText('Contrat de mariage')
  body:SetText('Titulaire : '..f[3]..'\n\n'..(f[4]~='' and ('Signature : '..f[4]..'\nNom commun : '..f[5]..'\n\nRapportez le contrat signe au notaire pour enregistrer le mariage.') or 'Signature : en attente.\n\nPortez chacun l alliance gravee au nom de l autre. Selectionnez votre partenaire proche, puis demandez sa signature.'))
  if f[4]=='' then offer:Show() end
 elseif kind=='SIGN' then
  title:SetText('Demande de mariage')
  body:SetText(f[3]..' vous propose de signer son contrat.\n\nNom de famille commun : '..f[4]..'\n\nEn signant, vous acceptez cette union et ce nom lors du depot en mairie. Les deux alliances doivent rester equipees.')
  sign:Show() decline:Show()
 elseif kind=='EMBLEM' then
  title:SetText('Armoiries de la famille') body:SetText('Le blason familial est independant de votre guilde.') window:SetHeight(390)
  local n=tonumber(f[3]) or 0 for i=1,5 do design[i]=n%counts[i] n=math.floor(n/counts[i]) end
  palette:Show() refresh()
 end
end
local events=CreateFrame('Frame') events:RegisterEvent('CHAT_MSG_ADDON')
events:SetScript('OnEvent',function(_,_,p,text,channel,sender)
 if p==prefix and channel=='WHISPER' and sender==UnitName('player') then receive(text) end
end)
hooksecurefunc(GameTooltip,'SetBagItem',function(_,bag,slot)
 local serverBag=bag==0 and 255 or bag+18
 local serverSlot=bag==0 and slot+22 or slot-1
 GameTooltip.registryKey=serverBag..','..serverSlot GameTooltip.registryGraved=nil
 local link=GetContainerItemLink(bag,slot)
 if link and tonumber(link:match('item:(%d+)'))==9100304 then send('TIP|'..GameTooltip.registryKey) end
end)
-- Inventory slots are one-based in Lua and zero-based in the server protocol.
hooksecurefunc(GameTooltip,'SetInventoryItem',function(_,unit,slot) if unit=='player' then
 GameTooltip.registryKey='255,'..(slot-1) GameTooltip.registryGraved=nil
 local link=GetInventoryItemLink(unit,slot) if link and tonumber(link:match('item:(%d+)'))==9100304 then send('TIP|255,'..(slot-1)) end
end end)
GameTooltip:HookScript('OnHide',function() GameTooltip.registryKey=nil GameTooltip.registryGraved=nil end)
window:Hide()
