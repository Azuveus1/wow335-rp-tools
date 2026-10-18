from pathlib import Path
import datetime,shutil
root=Path(__file__).resolve().parents[2];server=Path('C:/TC/TrinityCore');task=Path(__file__).parent
backup=root/'build'/('birth-names-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
def edit(rel,old,new):
 p=server/rel;data=p.read_bytes();s=data.decode('utf-8',errors='surrogateescape').replace('\r\n','\n')
 if new in s:return
 if s.count(old)!=1:raise RuntimeError('Source anchor changed: '+old[:80])
 dest=backup/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data);p.write_text(s.replace(old,new,1),encoding='utf-8',errors='surrogateescape')
shutil.copy2(task/'BirthNames.h',server/'src/server/game/Globals/BirthNames.h')
file='src/server/game/Handlers/CharacterHandler.cpp'
edit(file,'#include "ObjectMgr.h"','#include "ObjectMgr.h"\n#include "BirthNames.h"')
for obj,send,owner in [('createInfo','SendCharCreate','0'),('renameInfo','SendCharRename','renameInfo->Guid.GetCounter()'),('customizeInfo','SendCharCustomize','customizeInfo->Guid.GetCounter()'),('factionChangeInfo','SendCharFactionChange','factionChangeInfo->Guid.GetCounter()')]:
 anchor=f'    ResponseCodes res = ObjectMgr::CheckPlayerName({obj}->Name, GetSessionDbcLocale(), true);'
 args='CHAR_CREATE_NAME_IN_USE' if obj=='createInfo' else 'CHAR_CREATE_NAME_IN_USE, '+obj+'.get()'
 edit(file,anchor,f'''    if (BirthNames::Taken({obj}->Name, {owner})) {{
        {send}({args});
        return;
    }}
'''+anchor)
# A deleted character's reserved birth identity must not acquire a new owner
# through reuse of its former GUID after a server restart.
edit('src/server/game/Globals/ObjectMgr.cpp','CharacterDatabase.Query("SELECT MAX(guid) FROM characters")','CharacterDatabase.Query("SELECT GREATEST(COALESCE((SELECT MAX(guid) FROM characters),0),COALESCE((SELECT MAX(player) FROM rp_registry_birth_name),0))")')
player='src/server/game/Entities/Player/Player.cpp'
edit(player,'#include "RegistryPolicy.h"','#include "RegistryPolicy.h"\n#include "BirthNames.h"')
edit(player,'void Player::SaveToDB(CharacterDatabaseTransaction trans, bool create /* = false */)\n{','void Player::SaveToDB(CharacterDatabaseTransaction trans, bool create /* = false */)\n{\n    if(create) BirthNames::AppendIdentity(trans,GetGUID().GetCounter(),GetName());')
# Legacy rename callback previously issued standalone queries despite creating
# a transaction. Put the name and its claim in the same checked transaction.
edit(file,'''    // Update name and at_login flag in the db
    CharacterDatabasePreparedStatement* stmt''','''    BirthNames::AppendRename(trans,renameInfo->Guid.GetCounter(),oldName,renameInfo->Name);
    // Update name and at_login flag in the db
    CharacterDatabasePreparedStatement* stmt''')
edit(file,'''    CharacterDatabase.Execute(stmt);

    // Removed declined name from db''','''    trans->Append(stmt);

    // Removed declined name from db''')
edit(file,'''    CharacterDatabase.Execute(stmt);

    TC_LOG_INFO("entities.player.character", "Account: {} (IP: {}) Character:[{}] ({}) Changed name to: {}"''','''    trans->Append(stmt);
    if(!CharacterDatabase.AsyncCommitTransaction(trans).m_future.get()) {
        SendCharRename(CHAR_CREATE_NAME_IN_USE,renameInfo.get());return;
    }

    TC_LOG_INFO("entities.player.character", "Account: {} (IP: {}) Character:[{}] ({}) Changed name to: {}"''')
edit(file,'    /// Customize\n    Player::Customize(customizeInfo.get(), trans);','    BirthNames::AppendRename(trans,customizeInfo->Guid.GetCounter(),oldName,customizeInfo->Name);\n    /// Customize\n    Player::Customize(customizeInfo.get(), trans);')
edit(file,'''    CharacterDatabase.CommitTransaction(trans);

    sCharacterCache->UpdateCharacterData(customizeInfo->Guid''','''    if(!CharacterDatabase.AsyncCommitTransaction(trans).m_future.get()) {
        SendCharCustomize(CHAR_CREATE_NAME_IN_USE,customizeInfo.get());return;
    }

    sCharacterCache->UpdateCharacterData(customizeInfo->Guid''')
edit(file,'''    // resurrect the character in case he's dead
    Player::OfflineResurrect''','''    BirthNames::AppendRename(trans,factionChangeInfo->Guid.GetCounter(),oldName,factionChangeInfo->Name);
    // resurrect the character in case he's dead
    Player::OfflineResurrect''')
edit(file,'    //std::string oldName = characterInfo->Name;','    std::string oldName = characterInfo->Name;')
cache='    sCharacterCache->UpdateCharacterData(factionChangeInfo->Guid, factionChangeInfo->Name, factionChangeInfo->Gender, factionChangeInfo->Race);'
edit(file,cache,'    // Family identity cache is published only after successful commit.')
edit(file,'''    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_DEBUG("entities.player", "{} (IP: {}) changed race''','''    if(!CharacterDatabase.AsyncCommitTransaction(trans).m_future.get()) {
        SendCharFactionChange(CHAR_CREATE_NAME_IN_USE,factionChangeInfo.get());return;
    }
'''+cache+'''

    TC_LOG_DEBUG("entities.player", "{} (IP: {}) changed race''')
print('Creation, rename/customization/faction change and GUID allocation protected. Backup:',backup)
