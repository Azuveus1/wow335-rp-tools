local B = GMBlueprintUI
local T = B.T
local categories = {"all","buildings","trees","plants","chairs","tables","fires","fences","containers","rocks","lights","signs","other"}
local names = {
 all={"Tous les objets","All objects"}, buildings={"Maisons et bâtiments","Houses and buildings"},
 trees={"Arbres","Trees"}, plants={"Végétation","Plants"}, chairs={"Chaises et bancs","Chairs and benches"},
 tables={"Tables","Tables"}, fires={"Feux et torches","Fires and torches"}, fences={"Barrières et murs","Fences and walls"},
 containers={"Caisses et récipients","Containers"}, rocks={"Rochers et cristaux","Rocks and crystals"},
 lights={"Lampes et bougies","Lights and candles"}, signs={"Panneaux et bannières","Signs and banners"}, other={"Autres","Other"}
}
local function categoryName(key) return names[key][B.language() == "frFR" and 1 or 2] end
local function normalize(s)
    s = string.lower(s or "")
    for _, pair in ipairs({{"é","e"},{"è","e"},{"ê","e"},{"ë","e"},{"à","a"},{"â","a"},{"ä","a"},{"î","i"},{"ï","i"},{"ô","o"},{"ö","o"},{"ù","u"},{"û","u"},{"ü","u"},{"ç","c"},{"É","e"},{"È","e"},{"À","a"},{"œ","oe"}}) do s = s:gsub(pair[1],pair[2]) end
    return s
end
local index = {}
local byEntry = {}
for _, row in ipairs(GMBlueprintCatalog or {}) do
    byEntry[row[1]] = row
    index[#index+1] = {row=row, search=normalize(table.concat({row[1],row[2],row[3],row[5],names[row[4]][1],names[row[4]][2]}," "))}
end
B.objectName=function(entry)
 local row=byEntry[entry]
 return row and row[B.language()=="frFR" and 3 or 2]
end
local frame = CreateFrame("Frame", "GMBlueprintCatalogPanel", GMBlueprintPanel)
frame:SetSize(280,355)
frame:SetPoint("TOPRIGHT",GMBlueprintPanel,"TOPLEFT",0,0)
frame:SetBackdrop({bgFile="Interface\\DialogFrame\\UI-DialogBox-Background",edgeFile="Interface\\DialogFrame\\UI-DialogBox-Border",tile=true,tileSize=32,edgeSize=32,insets={left=8,right=8,top=8,bottom=8}})
frame:EnableMouse(true)
frame:Hide()
local function text(y)
 local f=frame:CreateFontString(nil,"OVERLAY","GameFontNormal")
 f:SetPoint("TOPLEFT",20,y); f:SetSize(240,24); f:SetJustifyH("LEFT"); return f
end
local title,caption,footer = text(-18),text(-40),text(-283)
local search=CreateFrame("EditBox","GMBlueprintCatalogSearch",frame,"InputBoxTemplate")
search:SetSize(230,24); search:SetPoint("TOPLEFT",25,-65); search:SetAutoFocus(false); search:SetMaxLetters(80)
search:SetScript("OnEscapePressed",function(self) self:ClearFocus() end)
search:SetScript("OnEnterPressed",function(self) self:ClearFocus() end)
local dropdown=CreateFrame("Frame","GMBlueprintCategoryDropdown",frame,"UIDropDownMenuTemplate")
dropdown:SetPoint("TOPLEFT",4,-92); UIDropDownMenu_SetWidth(dropdown,214)
local selected,page,filtered="all",1,{}
local rows={}
local pageSize=6
local function makeButton(name,label,x,y,width,callback)
 local b=CreateFrame("Button",name,frame,"UIPanelButtonTemplate")
 b:SetSize(width,24); b:SetPoint("TOPLEFT",x,y); b:SetText(label); b:SetScript("OnClick",callback); return b
end
local render
for i=1,pageSize do
 local b=makeButton("GMBlueprintCatalogRow"..i,"",20,-125-(i-1)*26,240,function(self)
     if self.entry and B.preview(self.entry) then search:ClearFocus() end
 end)
 b:SetNormalFontObject("GameFontNormalSmall")
 b:SetScript("OnEnter",function(self)
     local row=self.entry and byEntry[self.entry]
     if not row then return end
     GameTooltip:SetOwner(self,"ANCHOR_RIGHT")
     GameTooltip:SetText(B.objectName(self.entry))
     GameTooltip:AddLine(categoryName(row[4]),1,1,1)
     GameTooltip:AddLine("Entry: "..row[1].." / Display: "..row[6],1,1,1)
     GameTooltip:AddLine(row[5],0.7,0.7,0.7,true)
     GameTooltip:Show()
 end)
 b:SetScript("OnLeave",function() GameTooltip:Hide() end)
 rows[i]=b
end
local prev=makeButton(nil,"<",20,-318,40,function() page=page-1; render() end)
local nextButton=makeButton(nil,">",65,-318,40,function() page=page+1; render() end)
local close=makeButton(nil,"",140,-318,120,function() frame:Hide() end)
render=function()
 local pages=math.max(1,math.ceil(#filtered/pageSize)); page=math.max(1,math.min(page,pages))
 title:SetText(T("Catalogue")); caption:SetText(T("Recherche : nom ou identifiant"))
 close:SetText(T("Fermer")); UIDropDownMenu_SetText(dropdown,categoryName(selected))
 footer:SetText(string.format(T("%d résultats — page %d / %d"),#filtered,page,pages))
 if page>1 then prev:Enable() else prev:Disable() end
 if page<pages then nextButton:Enable() else nextButton:Disable() end
 for i,b in ipairs(rows) do
     local row=filtered[(page-1)*pageSize+i]
     if row then
         b.entry=row[1]; b:SetText(row[B.language()=="frFR" and 3 or 2].."  ["..row[1].."]"); b:Show()
     else b.entry=nil; b:Hide() end
 end
end
local function filter()
 filtered={}; page=1
 local words={}; for word in normalize(search:GetText()):gmatch("%S+") do words[#words+1]=word end
 for _,item in ipairs(index) do
     local matches=selected=="all" or selected==item.row[4]
     for _,word in ipairs(words) do if not item.search:find(word,1,true) then matches=false; break end end
     if matches then filtered[#filtered+1]=item.row end
 end
 table.sort(filtered,function(a,b)
     local col=B.language()=="frFR" and 3 or 2
     if a[col]==b[col] then return a[1]<b[1] end
     return a[col]<b[col]
 end)
 render()
end
local delay
search:SetScript("OnTextChanged",function() delay=0.2 end)
frame:SetScript("OnUpdate",function(self,elapsed) if delay then delay=delay-elapsed; if delay<=0 then delay=nil; filter() end end end)
UIDropDownMenu_Initialize(dropdown,function()
 for _,key in ipairs(categories) do
     local category=key
     local info=UIDropDownMenu_CreateInfo(); info.text=categoryName(key); info.checked=selected==key
     info.func=function() selected=category; filter() end
     UIDropDownMenu_AddButton(info)
 end
end)
B.catalogRefresh=filter
B.toggleCatalog=function() if frame:IsShown() then frame:Hide() else frame:Show(); filter() end end
