"""Read transport models; optional installation of two new editor tables only."""
from pathlib import Path
import os,re,subprocess,json,struct,sys
task=Path(__file__).parent
conf=Path(r'C:\TC\bin\worldserver.conf').read_text(encoding='utf-8-sig')
host,port,user,password,database=re.search(r'^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"',conf,re.M).group(1).split(';')
env=os.environ.copy(); env['MYSQL_PWD']=password
def query(sql):
    r=subprocess.run([r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe','--host='+host,'--port='+port,'--user='+user,'--database='+database,'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names'],input=sql.encode('utf-8'),env=env,capture_output=True)
    if r.returncode: raise SystemExit('Database operation failed (connection details omitted).')
    return r.stdout.decode('utf-8')
if '--schema' in sys.argv:
    query((task/'server/schema.sql').read_text())
    print('Editor tables ready. Existing game tables unchanged.')
data=Path(r'C:\TC\data\dbc\GameObjectDisplayInfo.dbc').read_bytes()
_,count,fields,size,strings=struct.unpack_from('<4s4I',data); base=20+count*size; wmos=set()
for i in range(count):
    ident,off=struct.unpack_from('<II',data,20+i*size)
    name=data[base+off:data.index(b'\0',base+off)].decode('utf-8',errors='replace')
    if name.lower().endswith('.wmo'): wmos.add(ident)
rows=query("SELECT JSON_ARRAY(g.entry,g.name,g.displayId,g.size,COALESCE(NULLIF(l.name,''),g.name)) FROM gameobject_template g LEFT JOIN gameobject_template_locale l ON l.entry=g.entry AND l.locale='frFR' WHERE g.type=15 ORDER BY g.entry;")
seen=set(); models=[]
for line in rows.splitlines():
    row=json.loads(line)
    if row[2] in wmos and row[2] not in seen: models.append(row); seen.add(row[2])
out='GMTransportModels = {\n'+''.join(' {'+','.join(json.dumps(x,ensure_ascii=False) for x in row)+'},\n' for row in models)+'}\n'
(task/'addon/GMTransport/TransportModels.lua').write_text(out,encoding='utf-8')
# A tiny transparent diagonal texture used for projected 3D path segments.
size=64
header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,size,size,32,0x28)
pixels=bytes(c for y in range(size) for x in range(size) for c in (255,255,255,255 if abs(x+y-(size-1))<=1 else 0))
(task/'addon/GMTransport/Line.tga').write_bytes(header+pixels)
print('Transport models:',len(models))
