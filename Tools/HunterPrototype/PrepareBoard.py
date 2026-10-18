"""Install one owned, permanent bounty board near Orgrimmar's auction entrance."""
from pathlib import Path
import importlib.util,math
spec=importlib.util.spec_from_file_location('hunter_prepare',Path(__file__).with_name('Prepare.py'));db=importlib.util.module_from_spec(spec);spec.loader.exec_module(db)
query=db.query
entry=9100102
existing=query(f'SELECT ScriptName FROM gameobject_template WHERE entry={entry}')
if existing and existing!='go_hunter_contract_board':raise RuntimeError('Reserved board entry already belongs to another object.')
columns=[r.split('\t')[0] for r in query('SHOW COLUMNS FROM gameobject_template').splitlines() if r]
overrides={'entry':str(entry),'type':'2','displayId':'2491','name':"'Panneau des contrats'",'IconName':"'Interact'",'size':'1','AIName':"''",'ScriptName':"'go_hunter_contract_board'"}
for c in columns:
 if c.startswith('Data') or c.startswith('data'):overrides[c]='0'
names=','.join('`'+c+'`' for c in columns);values=','.join(overrides.get(c,'`'+c+'`') for c in columns)
query(f'INSERT IGNORE INTO gameobject_template ({names}) SELECT {values} FROM gameobject_template WHERE entry=179827')
assert query(f'SELECT entry FROM gameobject_template WHERE entry={entry}')==str(entry)
old=query(f'SELECT text0_0 FROM npc_text WHERE ID={entry}')
intro='Tableau des contrats de mercenaires. Choisissez une cible et une prime.'
if old and old!=intro:raise RuntimeError('Reserved board text already belongs to another feature.')
cols=[r.split('\t')[0] for r in query('SHOW COLUMNS FROM npc_text').splitlines() if r]
values=[]
for c in cols:
 if c=='ID':v=str(entry)
 elif c in ('text0_0','text0_1'):v="'"+intro+"'"
 elif c.startswith('BroadcastTextID'):v='0'
 elif c.startswith('prob'):v='1' if c=='prob0' else '0'
 else:v='`'+c+'`'
 values.append(v)
query('INSERT IGNORE INTO npc_text ('+','.join('`'+c+'`' for c in cols)+') SELECT '+','.join(values)+' FROM npc_text WHERE ID=1')
spawns=query(f'SELECT guid FROM gameobject WHERE id={entry}').splitlines()
if len(spawns)>1:raise RuntimeError('Multiple board spawns found; refusing to add another.')
if not spawns:
 angle=math.pi
 query(f'''INSERT INTO gameobject(id,map,spawnMask,phaseMask,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,state)
 VALUES ({entry},1,1,1,1650.0,-4438.0,18.43485,{angle},0,0,{math.sin(angle/2)},{math.cos(angle/2)},300,255,1)''')
# Calibrate only the initial staging position, preserving any later GM move.
query(f'UPDATE gameobject SET position_x=1650,position_y=-4438,position_z=18.43485 WHERE id={entry} AND ((ABS(position_x-1655)<0.01 AND ABS(position_y+4446)<0.01 AND ABS(position_z-17)<0.01) OR (ABS(position_x-1650)<0.01 AND ABS(position_y+4438)<0.01 AND ABS(position_z-18.7178)<0.01))')
print('Permanent board:',query(f'SELECT guid,id,map,position_x,position_y,position_z FROM gameobject WHERE id={entry}'))
