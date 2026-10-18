from pathlib import Path
import datetime
root=Path(__file__).resolve().parents[2];path=root/'WoW/Source/Ui/GameUI.cpp'
old=path.read_bytes();s=old.decode('utf-8').replace('\r\n','\n')
anchor='''    CGAutoCompleteName::RefreshName(guid);
}'''
new='''    CGAutoCompleteName::RefreshName(guid);

    // Invalidation clears the name cache, but existing world text and unit
    // frames keep their rendered name. Refresh only loaded players and wait
    // for the new server response before regenerating those displays.
    if (ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__)) {
        auto name = g_nameDBCache.GetRecord(guid.m_guid, guid, NameQueryCallback, nullptr, true);
        if (name) NameQueryCallback(0, guid, nullptr, true);
    }
}'''
if new not in s:
 if s.count(anchor)!=1:raise RuntimeError('Player invalidation anchor changed')
 backup=root/'build'/('rp-live-name-source-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.cpp')
 backup.write_bytes(old);path.write_text(s.replace(anchor,new,1),encoding='utf-8');print('Live-name refresh patched. Backup:',backup)
else:print('Live-name refresh already installed in sources.')
