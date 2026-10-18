from pathlib import Path
import datetime,shutil
root=Path(__file__).resolve().parents[2];server=Path('C:/TC/TrinityCore');task=Path(__file__).parent
backup=root/'build'/('rp-command-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
def edit(rel,old,new):
 p=server/rel;data=p.read_bytes();s=data.decode('utf-8',errors='surrogateescape').replace('\r\n','\n')
 if new in s:return
 if s.count(old)!=1:raise RuntimeError('Source anchor changed: '+old[:80])
 target=backup/rel;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data);p.write_text(s.replace(old,new,1),encoding='utf-8',errors='surrogateescape')
shutil.copy2(task/'CommandPlayerNames.h',server/'src/server/game/Chat/CommandPlayerNames.h')
edit('src/server/game/Chat/Chat.cpp','#include "Chat.h"','#include "Chat.h"\n#include "CommandPlayerNames.h"')
edit('src/server/game/Chat/Chat.cpp','    char* name_str = extractKeyFromLink(text, "Hplayer");','''    char* name_str = nullptr;
    auto prefix = RoleplayNames::CommandNamePrefix(text ? text : "", [](std::string name) {
        return normalizePlayerName(name) && !sCharacterCache->GetCharacterGuidByName(name).IsEmpty();
    });
    if (prefix) {
        // strtok owns the legacy command cursor. Use an internal delimiter at
        // the name boundary so subsequent strtok(nullptr,...) reads arguments.
        // This buffer is mutable already, as required by extractKeyFromLink.
        if (text[prefix.end]) text[prefix.end]='\\a';
        name_str = strtok(text+prefix.begin, "\\a");
    } else {
        name_str = extractKeyFromLink(text, "Hplayer");
    }''')
# Summon accepts exactly a name. Parse that entire argument explicitly, so an
# unknown surname cannot accidentally summon a legacy character of that first name.
edit('src/server/scripts/Commands/cs_misc.cpp','''    static bool HandleSummonCommand(ChatHandler* handler, char const* args)
    {
        Player* target;''','''    static bool HandleSummonCommand(ChatHandler* handler, char const* args)
    {
        std::string complete = args ? args : "";
        auto first=complete.find_first_not_of(" \\t");
        if(first!=std::string::npos)complete=complete.substr(first,complete.find_last_not_of(" \\t")-first+1);
        if(!complete.empty() && complete.front()!='|' && complete.front()!='"') {
            if(!normalizePlayerName(complete) || ObjectMgr::CheckPlayerName(complete,handler->GetSession()->GetSessionDbcLocale(),false)!=CHAR_NAME_SUCCESS) {
                handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);handler->SetSentErrorMessage(true);return false;
            }
            complete="|cffffffff|Hplayer:"+complete+"|h["+complete+"]|h|r";
            args=complete.c_str();
        }
        Player* target;''')
print('Command parser patched; source backup:',backup)
