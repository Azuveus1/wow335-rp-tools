from pathlib import Path
import sys
root=Path(__file__).resolve().parents[2]
def extract(relative,signature):
 text=(root/relative).read_text(encoding='utf-8');start=text.index(signature);at=text.index('{',start);depth=1;end=at+1
 while depth:
  if text[end]=='{':depth+=1
  elif text[end]=='}':depth-=1
  end+=1
 return text[start:end]
pieces=[('WoW/Source/Ui/GameUI.cpp','void CGGameUI::InvalidatePlayer('),('WoW/Source/Ui/GameUI.cpp','void CGGameUI::UnitNameUpdate('),('WoW/Source/Object/ObjectClient/Unit_C.cpp','void CGUnit_C::UpdateUnitNameText('),('WoW/Source/Object/ObjectClient/Unit_C.cpp','void NameQueryCallback('),('WoW/Source/DB/DBClient/DBCacheInstances.cpp','static int32_t ReceiveInvalidatePlayer(')]
Path(sys.argv[1]).write_text('\n'.join(extract(*p) for p in pieces),encoding='utf-8')
