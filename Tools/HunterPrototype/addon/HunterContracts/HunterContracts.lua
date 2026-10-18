local OPTION = "Deposer un contrat de mercenaire"
local panel = CreateFrame("Frame", "HunterContractBoardFrame", UIParent)
panel:SetSize(400, 300)
panel:SetPoint("CENTER")
panel:SetFrameStrata("DIALOG")
panel:SetMovable(true)
panel:EnableMouse(true)
panel:RegisterForDrag("LeftButton")
panel:SetScript("OnDragStart", panel.StartMoving)
panel:SetScript("OnDragStop", panel.StopMovingOrSizing)
panel:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background", edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border", tile=true, tileSize=32, edgeSize=32, insets={left=11,right=12,top=12,bottom=11}})
panel:Hide()
tinsert(UISpecialFrames, "HunterContractBoardFrame")
local function label(text, x, y, width)
    local f=panel:CreateFontString(nil,"OVERLAY","GameFontNormal")
    f:SetPoint("TOPLEFT",x,y);f:SetWidth(width or 350);f:SetJustifyH("LEFT");f:SetText(text)
    return f
end
label("Tableau des contrats",24,-23)
label("Nom de la cible :",25,-65)
label("Prime en pièces d'or :",25,-126)
local function edit(name,y)
    local box=CreateFrame("EditBox",name,panel,"InputBoxTemplate")
    box:SetSize(333,25);box:SetPoint("TOPLEFT",33,y);box:SetAutoFocus(false)
    box:SetMaxLetters(48)
    box:SetScript("OnEscapePressed",function() panel:Hide() end)
    return box
end
local target=edit("HunterContractTargetInput",-86)
local gold=edit("HunterContractGoldInput",-148);gold:SetMaxLetters(6)
local money=label("",25,-185)
local status=label("",25,-207);status:SetHeight(36)
local submit=CreateFrame("Button",nil,panel,"UIPanelButtonTemplate")
submit:SetSize(160,25);submit:SetPoint("BOTTOMLEFT",25,18);submit:SetText("Valider")
local cancel=CreateFrame("Button",nil,panel,"UIPanelButtonTemplate")
cancel:SetSize(160,25);cancel:SetPoint("BOTTOMRIGHT",-25,18);cancel:SetText("Fermer")
local pending, sentAt, suppressClose, expired, scheduled, freshOpen = false, 0, false, false, false, false
local function showStatus(text,good)
    status:SetText(text)
    status:SetTextColor(good and .35 or 1,good and 1 or .8,good and .35 or .2)
end
local function closeWithoutSending()
    suppressClose=true;panel:Hide();suppressClose=false
end
panel:SetScript("OnHide",function()
    target:ClearFocus();gold:ClearFocus();pending=false;scheduled=false
    if not suppressClose then CloseGossip() end
end)
cancel:SetScript("OnClick",function() panel:Hide() end)
target:SetScript("OnTabPressed",function() gold:SetFocus() end)
gold:SetScript("OnTabPressed",function() target:SetFocus() end)
target:SetScript("OnEnterPressed",function() gold:SetFocus() end)
local function trim(s) return (s or ""):match("^%s*(.-)%s*$") end
local function send()
    if pending or expired then return end
    local name,amount=trim(target:GetText()),trim(gold:GetText())
    if name=="" or #name>48 or name:find("[;|%c]") then showStatus("Saisissez le nom de la cible.");return end
    if not amount:match("^%d+$") or tonumber(amount)<1 or tonumber(amount)>100000 then
        showStatus("La prime doit être un entier de 1 à 100000 pièces d'or.");return
    end
    if tonumber(amount)*10000>GetMoney() then showStatus("Vous n'avez pas assez d'or pour cette prime.");return end
    if GetGossipOptions()~=OPTION then showStatus("Rouvrez le panneau pour déposer ce contrat.");return end
    pending=true;sentAt=GetTime();submit:Disable();target:ClearFocus();gold:ClearFocus()
    showStatus("Vérification et réservation de la prime…")
    SelectGossipOption(1,name..";"..amount,1)
end
submit:SetScript("OnClick",send)
gold:SetScript("OnEnterPressed",send)
local function updateMoney() money:SetText("Or disponible : "..math.floor(GetMoney()/10000)) end
local function showBoard()
    local choices={GetGossipOptions()}
    if choices[1]~=OPTION then closeWithoutSending();return end
    local fresh=freshOpen;freshOpen=false
    -- Hiding the stock gossip must not close its network interaction.
    if GossipFrame then
        local onHide=GossipFrame:GetScript("OnHide")
        GossipFrame:SetScript("OnHide",nil);HideUIPanel(GossipFrame);GossipFrame:SetScript("OnHide",onHide)
    end
    pending=false;expired=false;submit:Enable()
    if fresh then target:SetText("");gold:SetText("250") end
    local message=choices[3] or "Choisissez une cible et une prime."
    local success=message:find("^Contrat cree%.")~=nil
    if success then target:SetText("") end
    showStatus(message,success);updateMoney();panel:Show()
    if fresh or success then target:SetFocus() end
end
panel:RegisterEvent("GOSSIP_SHOW")
panel:RegisterEvent("GOSSIP_CLOSED")
panel:RegisterEvent("PLAYER_LEAVING_WORLD")
panel:RegisterEvent("PLAYER_REGEN_DISABLED")
panel:RegisterEvent("PLAYER_MONEY")
panel:SetScript("OnEvent",function(_,event)
    if event=="GOSSIP_SHOW" then
        if GetGossipOptions()==OPTION then freshOpen=not panel:IsShown();scheduled=true;panel:Show()
        elseif panel:IsShown() then closeWithoutSending() end
    elseif event=="PLAYER_MONEY" then updateMoney()
    else closeWithoutSending() end
end)
panel:SetScript("OnUpdate",function()
    if scheduled then scheduled=false;showBoard() end
    if pending and GetTime()-sentAt>10 then
        pending=false;expired=true;CloseGossip();showStatus("Réponse absente. Fermez et rouvrez le panneau avant de réessayer.")
    end
end)
