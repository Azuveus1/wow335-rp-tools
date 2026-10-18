from pathlib import Path
import shutil,datetime
root=Path(__file__).resolve().parents[2];task=Path(__file__).parent;server=Path('C:/TC/TrinityCore')
backup=root/'build'/('civil-registry-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
def edit(base,relative,fn):
 p=base/relative;old=p.read_bytes();s=old.decode('utf-8',errors='surrogateescape').replace('\r\n','\n');new=fn(s)
 if s!=new:
  dest=backup/('client' if base==root else 'server')/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(old);p.write_bytes(new.encode('utf-8',errors='surrogateescape'))
def replace(s,old,new):
 if new in s:return s
 if s.count(old)!=1:raise RuntimeError('Anchor not unique: '+old[:100])
 return s.replace(old,new,1)
def include(s,path):
 line='#include "'+path+'"\n';return s if line in s else line+s
custom=server/'src/server/scripts/Custom'
for name in ['civil_registry.cpp','RegistryPolicy.h','WeddingSlot.inc']:shutil.copy2(task/name,custom/name)
shutil.copy2(task/'RegistryPolicy.h',server/'src/server/game/Entities/Player/RegistryPolicy.h')
edit(server,'src/server/scripts/Custom/custom_script_loader.cpp',lambda s:replace(s,'void AddCustomScripts()\n{','void AddCustomScripts()\n{\n    AddSC_civil_registry();') if 'void AddSC_civil_registry();' in s else 'void AddSC_civil_registry();\n'+replace(s,'void AddCustomScripts()\n{','void AddCustomScripts()\n{\n    AddSC_civil_registry();'))
def player_server(s):
 s=include(s,'RegistryPolicy.h')
 s=replace(s,'void Player::ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition)\n{','void Player::ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition)\n{\n    // Family tabards carry a cosmetic design, never a gameplay enchantment.\n    if (item && item->GetEntry()==CivilRegistry::Tabard) return;')
 s=replace(s,'        *data << uint32(enchant ? enchant->ItemVisual : 0);','''        uint32 visual = enchant ? enchant->ItemVisual : 0;
        CivilRegistry::Design familyDesign{};
        if (*itemId==CivilRegistry::Tabard && CivilRegistry::Decode(uint16(*enchants),uint16(*enchants>>16),familyDesign))
            visual = 0x80000000u | CivilRegistry::Pack(familyDesign);
        *data << visual;''')
 s=replace(s,'    corpse->SetUInt32Value(CORPSE_FIELD_GUILD, GetGuildId());','''    corpse->SetUInt32Value(CORPSE_FIELD_GUILD, GetGuildId());
    if (auto tabard=GetItemByPos(INVENTORY_SLOT_BAG_0,EQUIPMENT_SLOT_TABARD); tabard && tabard->GetEntry()==CivilRegistry::Tabard) {
        CivilRegistry::Design d{};
        if(CivilRegistry::Decode(uint16(tabard->GetEnchantmentId(PERM_ENCHANTMENT_SLOT)),uint16(tabard->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT)),d))
            corpse->SetUInt32Value(CORPSE_FIELD_GUILD,0x80000000u|CivilRegistry::Pack(d));
    }''')
 return s
edit(server,'src/server/game/Entities/Player/Player.cpp',player_server)
edit(server,'src/server/game/Globals/ObjectMgr.cpp',lambda s:replace(include(s,'RegistryPolicy.h'),'        ItemEntry const* dbcitem = sItemStore.LookupEntry(entry);','''        // Civil-registry items reuse the exact artwork/type of these stock items.
        uint32 dbcEntry = entry==CivilRegistry::Paper ? 5863 : entry==CivilRegistry::Tabard ? 5976 : entry==CivilRegistry::Ring ? 7337 : entry;
        ItemEntry const* dbcitem = sItemStore.LookupEntry(dbcEntry);'''))
edit(server,'src/server/game/Guilds/Guild.h',lambda s:replace(s,'        void UpdateMemberData(Player* player, uint8 dataid, uint32 value);','        void UpdateMemberData(Player* player, uint8 dataid, uint32 value);\n        void UpdateMemberName(ObjectGuid guid, std::string_view name);'))
edit(server,'src/server/game/Guilds/Guild.cpp',lambda s:replace(s,'void Guild::UpdateMemberData(Player* player, uint8 dataid, uint32 value)','''void Guild::UpdateMemberName(ObjectGuid guid, std::string_view name)
{
    if (auto m=GetMember(guid))
        m->SetStats(name,m->GetLevel(),m->GetClass(),m->GetGender(),m->GetZoneId(),m->GetAccountId());
}

void Guild::UpdateMemberData(Player* player, uint8 dataid, uint32 value)'''))
edit(server,'src/server/game/Groups/Group.h',lambda s:replace(s,'        const char * GetLeaderName() const;','        const char * GetLeaderName() const;\n        void UpdateMemberName(ObjectGuid guid, std::string const& name);'))
edit(server,'src/server/game/Groups/Group.cpp',lambda s:replace(s,'char const* Group::GetLeaderName() const','''void Group::UpdateMemberName(ObjectGuid guid, std::string const& name)
{
    for(auto& member:m_memberSlots)if(member.guid==guid)member.name=name;
    if(m_leaderGuid==guid)m_leaderName=name;
    SendUpdate();
}

char const* Group::GetLeaderName() const'''))
def player_client(s):
 s=include(s,'Tools/CivilRegistry/RegistryPolicy.h')
 anchor='    this->m_characterComponent->AddItemBySlot(INVSLOT_TABARD, displayId, 0);'
 s=replace(s,anchor,anchor+'''
    auto const& familyItem=this->m_player->m_playerVisibleItems[INVSLOT_TABARD];
    CivilRegistry::Design familyDesign{};
    if (familyItem.m_itemId==CivilRegistry::Tabard && CivilRegistry::Decode(familyItem.m_enchantment[0],familyItem.m_enchantment[1],familyDesign)) {
        this->m_characterComponent->ApplyGuildColor(familyDesign[0],familyDesign[1],familyDesign[2],familyDesign[3],familyDesign[4]);
        return true;
    }
''')
 old='    if (GuildGetGuildTabard(guid, guildID, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background) && player->m_characterComponent) {'
 s=replace(s,old,'''    if (player->m_player->m_playerVisibleItems[INVSLOT_TABARD].m_itemId==CivilRegistry::Tabard) {
        player->LoadGuildTextures(player->m_player->m_playerVisibleItems[INVSLOT_TABARD].GetDisplayID());
        return;
    }
'''+old)
 return s
edit(root,'WoW/Source/Object/ObjectClient/Player_C.cpp',player_client)
edit(root,'WoW/Source/Object/ObjectClient/Corpse_C.cpp',lambda s:replace(include(s,'Tools/CivilRegistry/RegistryPolicy.h'),'    if (GuildGetGuildTabard(this->m_corpse->m_corpseFieldOwner, this->m_corpse->m_corpseFieldGuild, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {','''    CivilRegistry::Design familyDesign{};
    if ((this->m_corpse->m_corpseFieldGuild&0x80000000u) && CivilRegistry::Unpack(this->m_corpse->m_corpseFieldGuild&0xffffffu,familyDesign)) {
        this->m_component->ApplyGuildColor(familyDesign[0],familyDesign[1],familyDesign[2],familyDesign[3],familyDesign[4]);
        return;
    }
    if (GuildGetGuildTabard(this->m_corpse->m_corpseFieldOwner, this->m_corpse->m_corpseFieldGuild, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {'''))
def glue(s,creation=False):
 s=include(s,'Tools/CivilRegistry/RegistryPolicy.h')
 condition='if (GuildGetGuildTabard(display->m_characterInfo.m_guid' if creation else 'if (GuildGetGuildTabard(tabardInfo.m_guid'
 start=s.index(condition);line=s[start:s.index('\n',start)]
 component='CCharacterCreation::m_character' if creation else 'CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component'
 new='''CivilRegistry::Design familyDesign{};
                if ((uint32_t(itemVisual)&0x80000000u) && CivilRegistry::Unpack(uint32_t(itemVisual)&0xffffffu,familyDesign)) {
                    COMPONENT->ApplyGuildColor(familyDesign[0],familyDesign[1],familyDesign[2],familyDesign[3],familyDesign[4]);
                } else '''.replace('COMPONENT',component)+line
 if 'CivilRegistry::Design familyDesign{};' not in s:s=replace(s,line,new)
 # The packet extension's high bit is a design marker, not an enchant visual.
 anchor='int32_t visual = found && g_itemVisualsDB.GetRecord(itemRec.m_itemVisual) ? 0 : itemVisual;'
 s=replace(s,anchor,'int32_t visual = (uint32_t(itemVisual)&0x80000000u) ? 0 : (found && g_itemVisualsDB.GetRecord(itemRec.m_itemVisual) ? 0 : itemVisual);')
 if not creation:
  old='        if (GuildGetGuildTabard(WOWGUID(), guildID, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {'
  s=replace(s,old,'        if (!(uint32_t(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_inventoryItemVisualID[INVSLOT_TABARD])&0x80000000u) && GuildGetGuildTabard(WOWGUID(), guildID, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {')
 return s
edit(root,'WoW/Source/Glue/CharacterSelection.cpp',lambda s:glue(s))
edit(root,'WoW/Source/Glue/CharacterCreation.cpp',lambda s:glue(s,True))
def editor(s):
 s=include(s,'Tools/CivilRegistry/RegistryPolicy.h')
 code='''// Family editor stores its own palette; it never calls the guild purchase API.
static int32_t Script_FamilyDesign(lua_State* L) {
    auto frame=static_cast<CGTabardModelFrame*>(FrameScript_GetObjectThis(L,CGTabardModelFrame::GetObjectType()));
    CivilRegistry::Design d{};
    if (lua_gettop(L)>1) {
        for(int i=0;i<5;++i)d[i]=int(luaL_checkinteger(L,i+2));
        if(!CivilRegistry::Valid(d))return luaL_error(L,"Invalid family design");
        for(int i=0;i<5;++i)frame->m_variations[i]=d[i];
    }
    for(int i=0;i<5;++i)lua_pushinteger(L,frame->m_variations[i]);
    return 5;
}
static int32_t Script_FamilyBorder(lua_State* L) {
    auto frame=static_cast<CGTabardModelFrame*>(FrameScript_GetObjectThis(L,CGTabardModelFrame::GetObjectType()));
    char file[260];
    CCharacterComponent::GetTabardBorderFileName(lua_toboolean(L,2)?SECTION_TORSO_UPPER:SECTION_TORSO_LOWER,frame->m_variations[2],frame->m_variations[3],file,sizeof(file));
    lua_pushstring(L,file);return 1;
}

'''
 if 'static int32_t Script_FamilyDesign' not in s:s=s.replace('static FrameScript_Method s_ScriptMethods[] = {',code+'static FrameScript_Method s_ScriptMethods[] = {\n    { "FamilyDesign", &Script_FamilyDesign },\n    { "FamilyBorder", &Script_FamilyBorder },',1)
 return s
edit(root,'WoW/Source/Ui/TabardModelFrame_Scripts.cpp',editor)
# Register the three custom entries for inventory/equipment/artwork queries.
# No Item.dbc archive replacement or large sparse ID allocation is needed.
for p in (root/'WoW/Source').rglob('*.cpp'):
 if 'g_itemDB.GetRecord(' in p.read_text(encoding='utf-8',errors='surrogateescape'):
  edit(root,str(p.relative_to(root)),lambda s:include(s.replace('g_itemDB.GetRecord(','RegistryItemRecord('),'Tools/CivilRegistry/ClientItems.h'))
print('Patched client and server. Source backup:',backup)


