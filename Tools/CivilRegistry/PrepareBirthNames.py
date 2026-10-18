"""Reserve immutable birth identities and active aliases with transactional uniqueness.

No current character name is changed. Existing birth names are recovered from
the earliest complete name in the registry history, or the current full name.
"""
from pathlib import Path
import sys,re,json,datetime,subprocess
from Prepare import db,switch
switch('Character')
def rows(sql):return [line.split('\t') for line in db.query(sql).splitlines() if line]
def lit(value):return "'"+value.replace('\\','\\\\').replace("'","''")+"'"
def full(name):return name not in ('','NULL') and ' ' in name
if __name__=='__main__':
 existing=bool(db.query("SHOW TABLES LIKE 'rp_registry_birth_name'"))
 births={int(i):n for i,n in rows('SELECT player,name FROM rp_registry_birth_name')} if existing else {}
 history=rows('SELECT player,old_name,new_name FROM rp_registry_history ORDER BY id')
 earliest={}
 for player,old,new in history:
  for name in (old,new):
   if full(name):earliest.setdefault(int(player),name)
 characters=rows('SELECT guid,name,deleteInfos_Name FROM characters ORDER BY guid')
 claims={}
 def claim(name,owner):
  # SQL's own unique collation below is the final arbiter for accented variants.
  key=name.casefold()
  if key in claims and claims[key][0]!=owner:raise RuntimeError('Conflicting identity; no automatic rename: '+name)
  claims[key]=(owner,name)
 for ident,current,deleted in characters:
  owner=int(ident);birth=earliest.get(owner,current if full(current) else deleted if full(deleted) else '')
  if owner not in births and birth:births[owner]=birth
  if current not in ('','NULL'):claim(current,owner)
 for owner,name in births.items():claim(name,owner)
 print('Birth identities:',json.dumps(births,ensure_ascii=False,sort_keys=True))
 if '--apply' not in sys.argv:sys.exit()
 if b'worldserver.exe' in subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout.lower():raise RuntimeError('Stop worldserver before installing birth reservations.')
 collation=rows("SHOW FULL COLUMNS FROM characters WHERE Field='name'")[0][2]
 assert re.fullmatch('[a-z0-9_]+',collation)
 charset=collation.split('_')[0]
 folder=Path(__file__).resolve().parents[2]/'build';backup=folder/('birth-names-db-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
 snapshot={'characters':characters,'history':history,'births':rows('SELECT * FROM rp_registry_birth_name') if existing else [],'claims':rows('SELECT * FROM rp_registry_name_claim') if existing else [],'triggers':db.query('SHOW TRIGGERS')}
 backup.write_text(json.dumps(snapshot,ensure_ascii=False,indent=2),encoding='utf-8')
 db.query(f'''CREATE TABLE IF NOT EXISTS rp_registry_birth_name(player INT UNSIGNED PRIMARY KEY,name VARCHAR(25) CHARACTER SET {charset} COLLATE {collation} NOT NULL UNIQUE,created_at BIGINT UNSIGNED NOT NULL) ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_registry_name_claim(name VARCHAR(25) CHARACTER SET {charset} COLLATE {collation} PRIMARY KEY,player INT UNSIGNED NOT NULL,INDEX(player)) ENGINE=InnoDB;''')
 # INSERT, never REPLACE: an already registered birth identity is immutable.
 commands=[]
 for owner,name in births.items():commands.append(f'INSERT IGNORE INTO rp_registry_birth_name(player,name,created_at) VALUES ({owner},{lit(name)},UNIX_TIMESTAMP())')
 for owner,name in claims.values():commands.append(f'INSERT INTO rp_registry_name_claim(name,player) VALUES ({lit(name)},{owner}) ON DUPLICATE KEY UPDATE player=player')
 db.query('START TRANSACTION;'+';'.join(commands)+';COMMIT;')
 assert characters==rows('SELECT guid,name,deleteInfos_Name FROM characters ORDER BY guid'),'Current names changed unexpectedly'
 assert db.query('SELECT COUNT(*) FROM rp_registry_birth_name')==str(len(births))
 print('Birth names and transactional reservations installed; current identities unchanged. Backup:',backup)
