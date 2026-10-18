"""Compile the actual client quest parser functions with small world/cache stubs."""
from pathlib import Path
import sys
source=Path(__file__).resolve().parents[2]/'WoW/Source/Ui/QuestTextParser.cpp'
text=source.read_text(encoding='utf-8')
def function(signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  if text[end]=='{':depth+=1
  elif text[end]=='}':depth-=1
  end+=1
 return text[start:end]
Path(sys.argv[1]).write_text(function('int32_t QuestParserParseText(')+'\n'+function('bool QuestParserParseToken(')+'\n',encoding='utf-8')
