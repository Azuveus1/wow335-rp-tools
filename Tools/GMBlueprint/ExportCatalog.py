"""Read-only world catalog export. Credentials stay in memory, never in output files."""
import argparse, json, os, pathlib, re, struct, subprocess, unicodedata

p = argparse.ArgumentParser()
p.add_argument('--config', default=r'C:\TC\bin\worldserver.conf')
p.add_argument('--dbc', default=r'C:\TC\data\dbc\GameObjectDisplayInfo.dbc')
p.add_argument('--mysql', default=r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe')
p.add_argument('--output', default=str(pathlib.Path(__file__).parent / 'addon/GMBlueprint/CatalogData.lua'))
a = p.parse_args()
config = pathlib.Path(a.config).read_text(encoding='utf-8-sig')
match = re.search(r'^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"', config, re.M)
if not match: raise SystemExit('WorldDatabaseInfo not found')
host, port, user, password, database = match.group(1).split(';')
env = os.environ.copy()
env['MYSQL_PWD'] = password
sql = "SELECT JSON_ARRAY(g.entry,g.type,g.displayId,g.name,COALESCE(l.name,'')) FROM gameobject_template g LEFT JOIN gameobject_template_locale l ON l.entry=g.entry AND l.locale='frFR' WHERE g.displayId<>0 AND g.type NOT IN (11,15) ORDER BY g.entry"
result = subprocess.run([a.mysql,'--host='+host,'--port='+port,'--user='+user,'--database='+database,'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--execute='+sql],env=env,capture_output=True)
if result.returncode: raise SystemExit('Catalog query failed; check local database access and schema (credentials omitted).')
data = pathlib.Path(a.dbc).read_bytes()
magic,count,fields,size,strings = struct.unpack_from('<4s4I',data)
assert magic == b'WDBC' and fields >= 3 and size >= 12
base = 20 + count * size
models = {}
for i in range(count):
    ident, offset = struct.unpack_from('<II',data,20+i*size)
    end = data.index(b'\0',base+offset)
    models[ident] = data[base+offset:end].decode('utf-8',errors='replace')
rules = [
 ('tables',r'table|desk'), ('chairs',r'chair|bench|throne|stool|seat'),
 ('fires',r'campfire|bonfire|brazier|firepit|fireplace|torch'),
 ('trees',r'tree|sapling|palm'), ('plants',r'bush|flower|plant|shrub|grass|fern|mushroom|vine'),
 ('buildings',r'house|building|hut|tower|barn|inn\b|tent|castle|barracks'),
 ('fences',r'fence|barrier|barricade|gate|wall'),
 ('containers',r'crate|barrel|chest|box|basket|pot\b|urn'),
 ('rocks',r'rock|boulder|stone|crystal'), ('lights',r'lamp|lantern|candle'),
 ('signs',r'sign|banner|flag'),
]
rows=[]
for line in result.stdout.decode('utf-8').splitlines():
    entry,kind,display,en,fr=json.loads(line)
    path=models.get(display,'')
    if not path: continue
    text=(en+' '+path).lower()
    category=next((key for key,pattern in rules if re.search(pattern,text)), 'other')
    rows.append([entry,en,fr or en,category,path,display,kind])
out = ['-- Generated from local world templates and GameObjectDisplayInfo.dbc. No credentials.', 'GMBlueprintCatalog = {']
for row in rows: out.append(' {'+','.join(json.dumps(v,ensure_ascii=False) for v in row)+'},')
out.append('}')
pathlib.Path(a.output).write_text('\n'.join(out)+'\n',encoding='utf-8')
print('Exported',len(rows),'placeable-template candidates; categories are heuristic.')
