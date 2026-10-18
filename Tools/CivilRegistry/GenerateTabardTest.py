from pathlib import Path
import sys
text=(Path(__file__).resolve().parents[2]/'WoW/Source/Object/ObjectClient/Player_C.cpp').read_text(encoding='utf-8')
start=text.index('void CGPlayer_C::UpdateInventoryComponent(');end=text.index('    CGTradeSkillInfo::RefreshList(0);',start)
Path(sys.argv[1]).write_text(text[start:end]+'}\n',encoding='utf-8')
