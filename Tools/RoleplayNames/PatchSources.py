from pathlib import Path
import shutil,datetime
client=Path(__file__).resolve().parents[2];server=Path('C:/TC/TrinityCore');task=Path(__file__).parent
backup=client/'build'/('rp-names-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
def edit(root,relative,transform):
 p=root/relative;old=p.read_bytes();text=old.decode('utf-8',errors='surrogateescape');new=transform(text)
 if new!=text:
  save=backup/('client' if root==client else 'server')/relative;save.parent.mkdir(parents=True,exist_ok=True);save.write_bytes(old)
  p.write_bytes(new.encode('utf-8',errors='surrogateescape'))
def replace_once(s,a,b):
 if b in s:return s
 if s.count(a)!=1:raise RuntimeError('Source anchor changed: '+a[:75])
 return s.replace(a,b,1)
header='#include "Tools/RoleplayNames/NameParts.h"\n'
def validator(s):
 if header not in s:s=header+s
 old='    return ValidateCharacterNameConst(s_locale, name, true, true, true, true, 0) + 87;'
 new='''    if (!name) return CHAR_NAME_NO_NAME;
    std::string_view first,last,full(name);
    if (full.find(' ') == std::string_view::npos)
        return ValidateCharacterNameConst(s_locale, name, true, true, true, true, 0) + 87;
    if (!RoleplayNames::Split(full,first,last)) return CHAR_NAME_INVALID_SPACE;
    std::string a(first),b(last);
    int result=ValidateCharacterNameConst(s_locale,a.c_str(),true,true,true,true,0)+87;
    return result==CHAR_NAME_SUCCESS ? ValidateCharacterNameConst(s_locale,b.c_str(),true,true,true,true,0)+87 : result;'''
 if '#include <string>\n' not in s:s='#include <string>\n'+s
 return replace_once(s,old,new)
edit(client,'WoW/Source/WowSvcs/WowSvcsClient/ClientServices.cpp',validator)
def creation(s):
 if header not in s:s=header+s
 anchor='    CHARACTER_CREATE_INFO info;'
 new='''    if (CCharacterCreation::m_existingCharacterIndex < 0) {
        std::string_view first,last;
        if (!name || !RoleplayNames::Split(name,first,last)) {
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", FrameScript_GetText("CHAR_NAME_INVALID_SPACE", -1, GENDER_NOT_APPLICABLE));
            return;
        }
    }

    CHARACTER_CREATE_INFO info;'''
 return replace_once(s,anchor,new)
edit(client,'WoW/Source/Glue/CharacterCreation.cpp',creation)
# Embed the reviewed/tested Lua; no MPQ/GlueXML archive replacement is needed.
(client/'WoW/Source/Glue/RoleplayNamesLua.inc').write_text('R"RPNAMES('+ (task/'Creation.lua').read_text(encoding='utf-8') +')RPNAMES"',encoding='utf-8')
edit(client,'WoW/Source/Glue/CGlueMgr.cpp',lambda s:replace_once(s,'            FrameScript_SignalEvent(GLUE_EVENT_FRAMES_LOADED, nullptr);','''            FrameScript_Execute(
#include "RoleplayNamesLua.inc"
                , "RoleplayNamesCreation", nullptr);
            FrameScript_SignalEvent(GLUE_EVENT_FRAMES_LOADED, nullptr);'''))
shutil.copy2(task/'NameParts.h',server/'src/server/game/Globals/RoleplayNameParts.h')
def server_names(s):
 if '#include "RoleplayNameParts.h"' not in s:s='#include "RoleplayNameParts.h"\n'+s
 s=replace_once(s,'''    wstrToLower(tmp);
    if (!tmp.empty())
        tmp[0] = wcharToUpper(tmp[0]);''','''    if (name.find(' ')!=std::string::npos) {
        std::string_view first,last;
        if (!RoleplayNames::Split(name,first,last)) return false;
    }
    wstrToLower(tmp);
    for (size_t i=0;i<tmp.size();++i)
        if (!i || tmp[i-1]==L' ') tmp[i]=wcharToUpper(tmp[i]);''')
 start=s.index('ResponseCodes ObjectMgr::CheckPlayerName(');end=s.index('bool ObjectMgr::IsValidCharterName',start)
 old=s[start:end]
 if 'RoleplayNames::Split(name' not in old:
  body=old[old.index('{')+1:old.rindex('}')].replace('wname.size() > MAX_PLAYER_NAME','wname.size() > 12')
  new='''ResponseCodes ObjectMgr::CheckPlayerName(std::string_view name, LocaleConstant locale, bool create /*= false*/)
{
    auto checkPart=[&](std::string_view name)->ResponseCodes {
'''+body+'''
    };
    std::string_view first,last;
    if (RoleplayNames::Split(name,first,last)) {
        auto result=checkPart(first);
        return result==CHAR_NAME_SUCCESS ? checkPart(last) : result;
    }
    // Legacy single names remain usable for existing characters at login.
    if (!create && name.find(' ')==std::string_view::npos) return checkPart(name);
    return CHAR_NAME_INVALID_SPACE;
}

'''
  s=s[:start]+new+s[end:]
 s=replace_once(s,'''bool ObjectMgr::IsReservedName(std::string_view name) const
{
    std::wstring wstr;''','''bool ObjectMgr::IsReservedName(std::string_view name) const
{
    std::string_view first,last;
    if (RoleplayNames::Split(name,first,last)) return IsReservedName(first) || IsReservedName(last);
    std::wstring wstr;''')
 return s
edit(server,'src/server/game/Globals/ObjectMgr.cpp',server_names)
edit(server,'src/server/game/Globals/ObjectMgr.h',lambda s:replace_once(replace_once(s,'#define MAX_PLAYER_NAME          12','#define MAX_PLAYER_NAME          25'),'#define MAX_INTERNAL_PLAYER_NAME 15','#define MAX_INTERNAL_PLAYER_NAME 28'))
shutil.copy2(task/'server_tests.cpp',server/'src/server/scripts/Custom/roleplay_name_tests.cpp')
def loader(s):
 if 'void AddSC_roleplay_names_tests();' not in s:s='void AddSC_roleplay_names_tests();\n'+s
 if '    AddSC_roleplay_names_tests();' in s:return s
 return replace_once(s,'void AddCustomScripts()\n{','void AddCustomScripts()\n{\n    AddSC_roleplay_names_tests();')
edit(server,'src/server/scripts/Custom/custom_script_loader.cpp',loader)
print('Source backups:',backup)
