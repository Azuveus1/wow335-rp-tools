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
dofile("Tools/GMBlueprint/addon/GMBlueprint/Catalog.lua")

function methods:CreateTexture() return CreateFrame("Texture") end
function methods:GetWidth() return 1024 end
function methods:GetHeight() return 768 end
local mouseDown,nativeAxis=false,0
local dragCalls={}
function IsMouseButtonDown(button) assert(button=="LeftButton");return mouseDown end
function GMBlueprintGizmo()
 if not controlsEnabled then return end
 return {0.5,0.5,{0.6,0.5},{0.5,0.65},{0.46,0.62}},nativeAxis
end
function GMBlueprintGizmoDrag(axis) nativeAxis=axis;dragCalls[#dragCalls+1]=axis;return true end
local events;for _,f in ipairs(frames) do if f.scripts.OnEvent then events=f end end
local function update() events.scripts.OnUpdate(events,0.016) end
local function event(s) events.scripts.OnEvent(events,"CHAT_MSG_SYSTEM",s) end
local function click(label) buttons[label].scripts.OnClick(buttons[label]) end
SlashCmdList.GMBLUEPRINT("149048");assert(not GMBlueprintGizmoFrame:IsShown())
event("[WB:READY:149048]");nativeValid=true;update();assert(GMBlueprintGizmoFrame:IsShown())
local handles={};for _,f in ipairs(frames) do if f.axis then handles[f.axis]=f end end
assert(handles[1] and handles[2] and handles[3],"Three projected arrow axes must be interactive")
for axis=1,3 do
 mouseDown=true;handles[axis].scripts.OnMouseDown(handles[axis],"LeftButton");assert(nativeAxis==axis)
 shift=true;update();assert(not tickFollow,"Shift ground placement must not override axis drag")
 mouseDown=false;update();assert(nativeAxis==0,"Release outside the handle ends the drag")
end
mouseDown=true;handles[1].scripts.OnMouseDown(handles[1],"LeftButton");combat=true;update();assert(not GMBlueprintGizmoFrame:IsShown() and nativeAxis==0)
combat=false;mouseDown=false;update();assert(GMBlueprintGizmoFrame:IsShown())
mouseDown=true;handles[2].scripts.OnMouseDown(handles[2],"LeftButton");GMBlueprintPanel:Hide();assert(nativeAxis==0 and not GMBlueprintGizmoFrame:IsShown())
SlashCmdList.GMBLUEPRINT("149048");event("[WB:READY:149048]");nativeValid=true;update();click("Valider");update();assert(not GMBlueprintGizmoFrame:IsShown(),"Hide and lock gizmo during placement submission")
event("[WB:OK:"..sent[#sent]:match(" (%d+)$")..":123]");update();assert(not GMBlueprintGizmoFrame:IsShown())
print("PASS: gizmo visibility, X/Y/Z handles, mouse release, Shift exclusion, combat lock, close cleanup, placement pending/receipt")
