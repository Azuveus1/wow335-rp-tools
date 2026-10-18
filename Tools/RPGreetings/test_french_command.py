"""Follow the real French slash command through Glue/FrameXML and the DBC ID."""
from pathlib import Path
import re,struct,subprocess
root=Path(__file__).resolve().parents[2]
extractor=root/'build/rp-names-tests/Release/rp_extract_ui.exe'
archive=Path('C:/Jeux/WoW335a/Data/frFR/locale-frFR.MPQ')
def read(name):return subprocess.run([str(extractor),str(archive),'Interface\\FrameXML\\'+name],capture_output=True,check=True).stdout.decode('utf-8')
strings=read('GlobalStrings.lua');chat=read('ChatFrame.lua')
match=re.search(r'EMOTE(\d+)_CMD\d*\s*=\s*"/saluer"',strings);assert match,'French /saluer binding missing.'
token=re.search(r'EMOTE'+match[1]+r'_TOKEN\s*=\s*"([^"]+)"',chat)[1]
b=Path('C:/TC/data/DBC/EmotesText.dbc').read_bytes();n,f,size,_=struct.unpack_from('<4I',b,4);pool=b[20+n*size:]
rows=[]
for i in range(n):
 row=struct.unpack_from('<'+'I'*f,b,20+i*size)
 if pool[row[1]:].split(b'\0',1)[0].decode('utf-8')==token:rows.append(row)
assert len(rows)==1
emote=rows[0][0];assert token=='GREET' and emote==48
subprocess.run([str(root/'build/rp-greetings-tests/Release/rp_greeting_test.exe'),str(emote)],check=True)
print(f'PASS: actual French /saluer -> EMOTE{match[1]} -> {token} -> CMSG_TEXT_EMOTE {emote} is accepted by the greeting policy.')
