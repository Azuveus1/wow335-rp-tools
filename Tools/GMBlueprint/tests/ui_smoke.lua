-- Runs in the client's Lua 5.1, with UI/network stubs. It does not connect to a realm.
local frames, buttons, messages, sent = {}, {}, {}, {}
local now, shift = 100, false
local cursorX, cursorY, combat, ctrl, controlsEnabled = 100, 100, false, false, false
local bindings, adjustments = {}, {}
function GetCursorPosition() return cursorX, cursorY end
function InCombatLockdown() return combat end
function ClearOverrideBindings() bindings = {} end
function SetOverrideBindingClick(owner, priority, key, name) bindings[key] = name end
local nativeEntry, nativeValid, nativeLoading, nativeError = 0, false, false, ""
local beginCount, tickFollow = 0, false
local methods = {}
local function noop() end
setmetatable(methods, {__index=function() return noop end})
function methods:SetScript(name, fn) self.scripts[name] = fn end
function methods:SetText(text) self.text = text; if self.kind == "Button" then buttons[text] = self end end
function methods:GetText() return self.text or "" end
function methods:Show()
    local shown=self.shown; self.shown=true
    if not shown and self.scripts.OnShow then self.scripts.OnShow(self) end
end
function methods:Hide()
    local shown = self.shown; self.shown = false
    if shown and self.scripts.OnHide then self.scripts.OnHide(self) end
end
function methods:IsShown() return self.shown end
function methods:Enable()
    if not self.enabled then self.enabled = true; self.pressed = false end
end
function methods:Disable()
    if self.enabled then self.enabled = false; self.pressed = false end
end
function methods:IsEnabled() return self.enabled end
function methods:CreateFontString() return CreateFrame("FontString") end
function CreateFrame(kind, name)
    local frame = setmetatable({kind=kind, scripts={}, shown=true, enabled=true, text=""}, {__index=methods})
    frames[#frames+1] = frame
    if name then _G[name] = frame end
    return frame
end
UIParent = CreateFrame("Frame")
WorldFrame = CreateFrame("Frame")
UISpecialFrames, SlashCmdList = {}, {}
tinsert = table.insert
DEFAULT_CHAT_FRAME = {AddMessage=function(self, msg) messages[#messages+1] = msg end}
function GetTime() return now end
function IsShiftKeyDown() return shift end
function IsControlKeyDown() return ctrl end
function GetMouseFocus() return WorldFrame end
function SendChatMessage(msg, kind) assert(kind == "SAY"); sent[#sent+1] = msg end
function GMBlueprintBegin(entry) nativeEntry=entry; beginCount=beginCount+1; return true end
function GMBlueprintCancel() nativeEntry=0; nativeValid=false end
function GMBlueprintControls(enabled) controlsEnabled = enabled end
function GMBlueprintAdjust(angle, height) adjustments[#adjustments+1] = {angle, height} end
function GMBlueprintTick(follow) tickFollow=follow end
function GMBlueprintState()
    return nativeEntry, nativeValid, nativeLoading, 0, 1.25, -2.5, 3.75, 1.5, "Test object", nativeError, false
end
function GetLocale() return "frFR" end
function UIDropDownMenu_SetWidth() end
function UIDropDownMenu_SetText(frame,text) frame.text=text end
function UIDropDownMenu_Initialize(frame,fn) frame.initialize=fn end
function UIDropDownMenu_CreateInfo() return {} end
local menuItems={}
function UIDropDownMenu_AddButton(info) menuItems[#menuItems+1]=info end
dofile("Tools/GMBlueprint/addon/GMBlueprint/Locales.lua")
dofile("Tools/GMBlueprint/addon/GMBlueprint/GMBlueprint.lua")
dofile("Tools/GMBlueprint/addon/GMBlueprint/CatalogData.lua")
-- Small authored fixture; no generated world/DBC catalogue is needed for UI tests.
GMBlueprintCatalog = {
 {149048,"Test bench","Banc de test","chairs","TestBench.mdx",1,5},
 {900001,"Test table","Table de test","tables","TestTable.mdx",2,5},
 {900002,"Test plant","Plante de test","plants","TestPlant.mdx",3,5},
}
dofile("Tools/GMBlueprint/addon/GMBlueprint/Catalog.lua")
local events
for _, frame in ipairs(frames) do if frame.scripts.OnEvent then events=frame end end
assert(events)
local function event(message) events.scripts.OnEvent(events, "CHAT_MSG_SYSTEM", message) end
local function update() events.scripts.OnUpdate(events, 0.016) end
local function click(name) buttons[name].scripts.OnClick(buttons[name]) end

SlashCmdList.GMBLUEPRINT("123")
assert(sent[#sent] == ".gobject blueprint info 123")
assert(beginCount == 0, "Preview must wait for server permission")
event("[WB:READY:456]"); assert(beginCount == 0)
event("[WB:READY:123]"); assert(beginCount == 1 and nativeEntry == 123)
update(); assert(not buttons.Valider.enabled)
nativeValid=true; shift=true; update(); assert(tickFollow and buttons.Valider.enabled)
assert(controlsEnabled, "Native controls enabled only after the preview starts")
click("Rotation +1 deg"); assert(math.abs(adjustments[#adjustments][1]-math.pi/180) < 0.00001)
click("Rotation -1 deg"); assert(math.abs(adjustments[#adjustments][1]+math.pi/180) < 0.00001)
click("Hauteur +0.1"); assert(adjustments[#adjustments][2] == 0.1)
ctrl=true
click("Rotation +1 deg"); assert(math.abs(adjustments[#adjustments][1]-math.pi/1800) < 0.00001)
click("Hauteur -0.1"); assert(adjustments[#adjustments][2] == -0.01)
ctrl=false
combat=true; update(); assert(not controlsEnabled)
combat=false; update(); assert(controlsEnabled)
shift=false; update(); assert(not tickFollow)
-- A real click spans several OnUpdate frames between mouse down and mouse up.
-- Disabling and re-enabling CSimpleButton resets BUTTONSTATE_PUSHED, so toggling every frame
-- silently loses the mouse-up click even when the button looks enabled.
assert(buttons.Valider.enabled)
buttons.Valider.pressed = true
update(); update()
assert(buttons.Valider.pressed, "OnUpdate must preserve a held Validate click")
click("Valider")
assert(not controlsEnabled, "Pending placement must release native controls")
local command = sent[#sent]
assert(command:match("^%.gobject blueprint place 123 0 1%.250000 %-2%.500000 3%.750000 1%.500000 %d+$"))
local token = command:match("(%d+)$")
local count = #sent
click("Valider"); assert(#sent == count, "Double click must not create twice")
update(); assert(not buttons.Valider.enabled)
event("[WB:OK:9:80]"); assert(nativeEntry == 123, "Unrelated receipt must be ignored")
GMBlueprintPanel:Hide(); assert(nativeEntry == 0)
assert(not controlsEnabled, "Closing must release native controls")
event("[WB:OK:" .. token .. ":81]")
assert(messages[#messages]:find("GUID : 81", 1, true), "Receipt while hidden must still be handled")

SlashCmdList.GMBLUEPRINT("234")
event("[WB:ERROR] Identifiant inconnu.")
assert(beginCount == 1, "Rejected request must not create a preview")
SlashCmdList.GMBLUEPRINT("234")
now=now+16; update()
assert(beginCount == 1 and #sent == count+2, "Timeout must not retry")
event("[WB:READY:234]"); assert(beginCount == 1, "Late info reply after timeout must be ignored")

SlashCmdList.GMBLUEPRINT("345"); event("[WB:READY:345]")
nativeValid=true; click("Valider"); count=#sent
now=now+16; update()
assert(#sent == count and nativeEntry == 0, "Placement timeout must cancel locally, never resend")
assert(messages[#messages]:find("verifie les objets proches", 1, true))
SlashCmdList.GMBLUEPRINT("456"); event("[WB:READY:456]")
nativeLoading=true; now=now+31; update()
assert(nativeEntry == 0, "Missing model must time out")
nativeLoading=false
SlashCmdList.GMBLUEPRINT("567"); event("[WB:READY:567]")
events.scripts.OnEvent(events, "PLAYER_LEAVING_WORLD")
assert(nativeEntry == 0 and not GMBlueprintPanel:IsShown())
print("PASS: Lua 5.1 panel, authorization handshake, positioning, duplicate click, receipts, rejection, timeouts, map exit")

-- Catalog is local-only until a row requests the existing permission handshake.
SlashCmdList.GMBLUEPRINT("")
local before=#sent
click("Catalogue")
assert(GMBlueprintCatalogPanel:IsShown() and #sent==before)
local function searchCatalog(query)
    GMBlueprintCatalogSearch:SetText(query)
    GMBlueprintCatalogSearch.scripts.OnTextChanged()
    GMBlueprintCatalogPanel.scripts.OnUpdate(GMBlueprintCatalogPanel,0.25)
end
searchCatalog("149048")
assert(GMBlueprintCatalogRow1.entry==149048)
assert(not GMBlueprintCatalogRow2:IsShown())
GMBlueprintCatalogRow1.scripts.OnClick(GMBlueprintCatalogRow1)
assert(sent[#sent]==".gobject blueprint info 149048")
local sentPending=#sent
GMBlueprintCatalogRow1.scripts.OnClick(GMBlueprintCatalogRow1)
assert(#sent==sentPending, "Catalog cannot replace a pending request")
event("[WB:READY:149048]")
searchCatalog("table")
assert(GMBlueprintCatalogRow1:IsShown())
local firstTable=GMBlueprintCatalogRow1.entry
GMBlueprintCatalogRow1.scripts.OnClick(GMBlueprintCatalogRow1)
assert(sent[#sent]==".gobject blueprint info "..firstTable)
event("[WB:READY:"..firstTable.."]")
searchCatalog("no-such-model-xyz")
assert(not GMBlueprintCatalogRow1:IsShown(), "Empty search must hide stale results")
searchCatalog("végétation")
assert(GMBlueprintCatalogRow1:IsShown(), "French category search must normalize accents")
menuItems={}; GMBlueprintCategoryDropdown.initialize()
menuItems[6].func() -- tables
searchCatalog("")
local indexed={}; for _,row in ipairs(GMBlueprintCatalog) do indexed[row[1]]=row end
assert(indexed[GMBlueprintCatalogRow1.entry][4]=="tables")
menuItems={}; GMBlueprintLanguageDropdown.initialize(); menuItems[2].func()
assert(GMBlueprintSettings.language=="enUS")
assert(buttons["Place object"] and buttons["Catalog"])
assert(GMBlueprintUI.objectName(149048)==indexed[149048][2])
assert(GMBlueprintUI.T("Chargement du modele...")=="Loading model...")
assert(GMBlueprintUI.error("Sauvegarde effectuee (GUID 123), chargement impossible. Verifie cet objet avant de recommencer."):find("Saved (GUID 123)",1,true))
GMBlueprintPanel:Hide(); SlashCmdList.GMBLUEPRINT("")
assert(GMBlueprintLanguageDropdown.text=="English", "Language choice must survive panel reopen")
menuItems={}; GMBlueprintLanguageDropdown.initialize(); menuItems[1].func()
assert(GMBlueprintSettings.language=="frFR")
assert(GMBlueprintUI.objectName(149048)==indexed[149048][3])
print("PASS: real catalog, search, accents, categories, preview switching, pending guard, English/French, saved language")
