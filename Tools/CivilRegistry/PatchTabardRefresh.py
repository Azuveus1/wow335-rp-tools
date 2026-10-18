from pathlib import Path
import datetime
root=Path(__file__).resolve().parents[2];p=root/'WoW/Source/Object/ObjectClient/Player_C.cpp';old=p.read_bytes();s=old.decode('utf-8').replace('\r\n','\n')
a='''    if (abs(old->m_itemId) == abs(item->m_itemId) && (old->m_itemId > 0) == (item->m_itemId > 0)) {
        this->UpdateObjComponentVisualsSlot(item, slot);'''
b='''    if (abs(old->m_itemId) == abs(item->m_itemId) && (old->m_itemId > 0) == (item->m_itemId > 0)) {
        // A family design changes the visible enchantment words, not item ID.
        // UpdateObjComponentVisualsSlot only handles the two weapon slots.
        if (slot==INVSLOT_TABARD && item->m_itemId==CivilRegistry::Tabard) {
            this->LoadGuildTextures(item->GetDisplayID());
            return;
        }
        this->UpdateObjComponentVisualsSlot(item, slot);'''
# VisibleItem_C's existing accessor is non-const; item itself is mutable here.
if b not in s:
 if s.count(a)!=1:raise RuntimeError('Inventory update anchor changed')
 backup=root/'build'/('family-tabard-refresh-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.cpp');backup.write_bytes(old);p.write_text(s.replace(a,b,1),encoding='utf-8');print('Family tabard refresh patched:',backup)
