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
function IsMouseButtonDown() return mouseDown end
function GMBlueprintGizmo(mode)
 if not controlsEnabled then return end
 local p={0.5,0.5,{0.6,0.5},{0.5,0.65},{0.46,0.62}}
 if mode==0 then
  p[6]={.55,.55,{.53,.53},{.57,.53},{.57,.57},{.53,.57}}
  p[7]={.55,.45,{.53,.43},{.57,.43},{.57,.47},{.53,.47}}
  p[8]={.45,.55,{.43,.53},{.47,.53},{.47,.57},{.43,.57}}
  p[9]={.5,.5}
 else
  p[10]={}
  for axis=1,3 do local ring={};for i=0,64 do local angle=i*math.pi/32;ring[i+1]={.5+math.cos(angle)*.12,.5+math.sin(angle)*.12} end;p[10][axis]=ring end
 end
 return p,nativeAxis
end
function GMBlueprintGizmoDrag(axis) nativeAxis=axis;return true end
function GMBlueprintState() return nativeEntry,nativeValid,nativeLoading,0,1.25,-2.5,3.75,0,"Test object",nativeError,false,0,0.382683432,0,0.923879533 end
local events;for _,f in ipairs(frames) do if f.scripts.OnEvent then events=f end end
local function update() events.scripts.OnUpdate(events,0.016) end
local function event(s) events.scripts.OnEvent(events,"CHAT_MSG_SYSTEM",s) end
local function click(label) assert(buttons[label],label);buttons[label].scripts.OnClick(buttons[label]) end
local function handles() local result={};for _,f in ipairs(frames) do if f.axis and f.shown then result[f.axis]=f end end;return result end
SlashCmdList.GMBLUEPRINT("149048");event("[WB:READY:149048]");nativeValid=true;update()
local h=handles();for mode=1,7 do assert(h[mode],"Missing movement handle "..mode);mouseDown=true;h[mode].scripts.OnMouseDown(h[mode],"LeftButton");assert(nativeAxis==mode);shift=true;update();assert(not tickFollow);mouseDown=false;update();assert(nativeAxis==0) end
click("Gizmo : deplacer");update();h=handles();for mode=8,10 do assert(h[mode],"Missing rotation ring "..mode);mouseDown=true;h[mode].scripts.OnMouseDown(h[mode],"LeftButton");assert(nativeAxis==mode);update();assert(not tickFollow);mouseDown=false;update();assert(nativeAxis==0) end
assert(not h[1] and not h[7],"Movement handles must hide in rotation mode")
click("Gizmo : rotation");update();h=handles();assert(h[1] and h[7] and not h[8])
click("Valider");local command=sent[#sent];local fields={};for word in command:gmatch("%S+") do fields[#fields+1]=word end
assert(#fields==14 and fields[1]==".gobject" and fields[3]=="place",command)
assert(fields[11]=="0.000000000" and fields[12]=="0.382683432" and fields[13]=="0.000000000" and fields[14]=="0.923879533",command)
assert(#command<=255);update();assert(not GMBlueprintGizmoFrame:IsShown())
event("[WB:OK:"..fields[10]..":123]");update();assert(not GMBlueprintGizmoFrame:IsShown())
print("PASS: all seven translation handles, three rotation rings, mode switching, Shift exclusion, release and full quaternion placement payload")
