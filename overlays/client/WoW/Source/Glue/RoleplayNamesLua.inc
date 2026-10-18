R"RPNAMES(-- Runs in GlueXML, before login; ordinary in-game addons cannot change this screen.
if CharacterCreate and CharacterCreateNameEdit and not RoleplayFamilyNameEdit then
 local first=CharacterCreateNameEdit
 local last=CreateFrame("EditBox","RoleplayFamilyNameEdit",first:GetParent())
 last:SetWidth(170);last:SetHeight(40)
 last:SetPoint("LEFT",first,"RIGHT",10,0)
 last:SetAutoFocus(false);last:SetMaxLetters(12)
 last:SetFontObject(first:GetFontObject())
 last:SetTextInsets(10,10,0,0)
 last:SetBackdrop(first:GetBackdrop())
 last:SetBackdropColor(0,0,0,.8);last:SetBackdropBorderColor(.6,.6,.6,1)
 local fr=GetLocale()=="frFR"
 local firstLabel
 for _,region in ipairs({first:GetRegions()}) do
  if region:GetObjectType()=="FontString" and region:GetText()==NAME then firstLabel=region;break end
 end
 local label=last:CreateFontString(nil,"BACKGROUND","GlueFontNormalLarge")
 label:SetWidth(170);label:SetHeight(64)
 label:SetPoint("BOTTOM",last,"TOP",0,-23)
 label:SetText(fr and "Nom de famille" or "Family name")
 local function layout()
  first:ClearAllPoints()
  if PAID_CHARACTER_CUSTOMIZATION then
   first:SetWidth(156);first:SetMaxLetters(47)
   first:SetPoint("BOTTOM",first:GetParent(),"BOTTOM",8,50)
   if firstLabel then firstLabel:SetText(NAME);firstLabel:SetWidth(256) end
   last:Hide()
  else
   first:SetWidth(170);first:SetMaxLetters(12)
   first:SetPoint("BOTTOM",first:GetParent(),"BOTTOM",-90,50)
   if firstLabel then firstLabel:SetText(fr and "Prénom" or "First name");firstLabel:SetWidth(170) end
   last:SetText("");last:Show()
  end
 end
 -- Set the capacity before the stock show handler loads an existing full name.
 local onShow=CharacterCreate_OnShow
 function CharacterCreate_OnShow(...)
  layout()
  return onShow(...)
 end
 first:SetScript("OnTabPressed",function()first:ClearFocus();last:SetFocus()end)
 last:SetScript("OnTabPressed",function()last:ClearFocus();first:SetFocus()end)
 last:SetScript("OnEscapePressed",function()CharacterCreate_Back()end)
 first:SetScript("OnEnterPressed",function()last:SetFocus();first:ClearFocus()end)
 last:SetScript("OnEnterPressed",function()CharacterCreate_Okay()end)
 local create=CreateCharacter
 function CreateCharacter(name)
  if PAID_CHARACTER_CUSTOMIZATION then return create(name) end
  local surname=last:GetText() or ""
  if not name or name=="" then first:SetFocus();return end
  if surname=="" then last:SetFocus();return end
  -- Native validation and the server independently validate both components.
  return create(name.." "..surname)
 end
 layout()
end
)RPNAMES"