local methods={}
setmetatable(methods,{__index=function()return function()end end})
function methods:GetParent()return self.parent end
function methods:SetScript(k,f)self.scripts[k]=f end
function methods:SetText(t)self.text=t end
function methods:GetText()return self.text end
function methods:Show()self.shown=true end
function methods:Hide()self.shown=false end
function methods:SetMaxLetters(n)self.limit=n end
function methods:SetFocus()self.focus=true end
function methods:ClearFocus()self.focus=false end
function methods:CreateFontString()return CreateFrame("FontString")end
function methods:GetObjectType()return self.kind end
function CreateFrame(kind,name,parent)
 local f=setmetatable({kind=kind,parent=parent,scripts={},text="",shown=true},{__index=methods})
 if name then _G[name]=f end;return f
end
NAME="Nom";function GetLocale()return "frFR"end
CharacterCreate=CreateFrame("Frame")
CharacterCreateNameEdit=CreateFrame("EditBox",nil,CharacterCreate)
local label=CreateFrame("FontString");label:SetText(NAME)
function CharacterCreateNameEdit:GetRegions()return label end
function CharacterCreate_OnShow()CharacterCreateNameEdit:SetText(PAID_CHARACTER_CUSTOMIZATION and "Martin Ventdacier" or "")end
local sent={};function CreateCharacter(name)sent[#sent+1]=name end
function CharacterCreate_Okay()CreateCharacter(CharacterCreateNameEdit:GetText())end
function CharacterCreate_Back()end
dofile("Tools/RoleplayNames/Creation.lua")
local surname=RoleplayFamilyNameEdit
CharacterCreate_OnShow();assert(surname.shown and label:GetText()=="Prénom")
CharacterCreateNameEdit:SetText("Martin");CharacterCreate_Okay();assert(#sent==0 and surname.focus)
surname:SetText("Ventdacier");CharacterCreate_Okay();assert(sent[1]=="Martin Ventdacier")
surname:SetText("Clairbois");surname.scripts.OnEnterPressed();assert(sent[2]=="Martin Clairbois")
CharacterCreateNameEdit.scripts.OnTabPressed();assert(surname.focus)
surname.scripts.OnTabPressed();assert(CharacterCreateNameEdit.focus)
CharacterCreate_OnShow();assert(surname:GetText()=="")
PAID_CHARACTER_CUSTOMIZATION=1;CharacterCreate_OnShow();CharacterCreate_Okay()
assert(not surname.shown and CharacterCreateNameEdit.limit==47 and sent[3]=="Martin Ventdacier")
PAID_CHARACTER_CUSTOMIZATION=nil;CharacterCreate_OnShow();assert(surname.shown and CharacterCreateNameEdit.limit==12)
print("PASS: two fields, required family name, exact full name submission, keyboard, reset and existing customization")
