"""Create only the civil registry's owned tables, templates and spawns."""
from pathlib import Path
import sys,re,datetime,json,importlib.util
spec=importlib.util.spec_from_file_location('hunterdb',Path(__file__).parents[1]/'HunterPrototype/Prepare.py')
db=importlib.util.module_from_spec(spec);spec.loader.exec_module(db)
def switch(kind):
 db.host,db.port,db.user,password,db.database=re.search(r'^\s*'+kind+r'DatabaseInfo\s*=\s*"([^"]+)"',db.config,re.M).group(1).split(';');db.env['MYSQL_PWD']=password
if __name__=='__main__':
 switch('World')
 print(db.query("SELECT entry,name FROM item_template WHERE entry IN (5863,5976,7337,9100302,9100303,9100304)"))
 print(db.query("SELECT entry,name FROM creature_template WHERE name LIKE '%ring%' AND subname LIKE '%Merchant%' LIMIT 30"))
 if '--apply' not in sys.argv:sys.exit()
 backup=Path(__file__).resolve().parents[2]/'build'/('civil-registry-db-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
 backup.write_text(json.dumps({'creatures':db.query('SELECT * FROM creature_template WHERE entry BETWEEN 9100300 AND 9100301'),'items':db.query('SELECT * FROM item_template WHERE entry BETWEEN 9100302 AND 9100304'),'spawns':db.query('SELECT * FROM creature WHERE id BETWEEN 9100300 AND 9100301')},indent=2),encoding='utf-8')
 def clone(table,key,source,entry,overrides):
  old=db.query(f'SELECT ScriptName FROM {table} WHERE {key}={entry}')
  exists=db.query(f'SELECT {key} FROM {table} WHERE {key}={entry}')
  if exists and old!=overrides.get('ScriptName',"''").strip("'"):raise RuntimeError(f'Entry {entry} already belongs to another script')
  columns=[x.split('\t')[0] for x in db.query('SHOW COLUMNS FROM '+table).splitlines()]
  names=','.join('`'+x+'`' for x in columns);values=','.join(str(entry) if x==key else overrides.get(x,'`'+x+'`') for x in columns)
  db.query(f'INSERT IGNORE INTO {table} ({names}) SELECT {values} FROM {table} WHERE {key}={source}')
 for entry,source,name,title in [(9100300,4974,"Officier d etat civil","Registre des noms"),(9100301,5193,"Notaire matrimonial","Contrats et familles")]:
  clone('creature_template','entry',source,entry,{'name':"'"+name+"'",'subname':"'"+title+"'",'npcflag':'1','AIName':"''",'ScriptName':"'npc_civil_registry'",'faction':'35','gossip_menu_id':'0','MovementType':'0','unit_flags':'0'})
  if not db.query(f'SELECT guid FROM creature WHERE id={entry}'):
   x,y=(-8882.9,614.9) if entry==9100300 else (-8881.2,613.4)
   db.query(f'INSERT INTO creature(id,map,spawnMask,phaseMask,position_x,position_y,position_z,orientation,spawntimesecs,curhealth) VALUES ({entry},0,1,1,{x},{y},95.2576,3.5,300,1000)')
 clone('item_template','entry',5863,9100302,{'name':"'Contrat de mariage'",'description':"'A faire signer puis a rapporter au notaire matrimonial.'",'Flags':'0','FlagsExtra':'0','BuyPrice':'0','SellPrice':'0','maxcount':'1','stackable':'1','bonding':'1','spellid_1':'24740','spelltrigger_1':'0','ScriptName':"'item_civil_registry'"})
 clone('item_template','entry',5976,9100303,{'name':"'Tabard familial'",'description':"'Porte les armoiries de votre famille. Independant du tabard de guilde.'",'Flags':'0','FlagsExtra':'0','BuyPrice':'0','SellPrice':'0','maxcount':'1','bonding':'1','ScriptName':"'item_civil_registry_tabard'"})
 clone('item_template','entry',7337,9100304,{'name':"'Alliance gravee'",'description':"'Une alliance personnelle, gravee pour votre union.'",'Quality':'1','Flags':'0','FlagsExtra':'0','BuyPrice':'0','SellPrice':'0','maxcount':'0','stackable':'1','bonding':'0','ItemLevel':'1','RequiredLevel':'1','ScriptName':"'item_civil_registry_ring'"})
 existing=db.query('SELECT ScriptName FROM creature_template WHERE entry=2849')
 if existing and existing!='npc_civil_registry':raise RuntimeError('Qixdi already has a script; refusing to replace it')
 db.query("UPDATE creature_template SET npcflag=npcflag|1,ScriptName='npc_civil_registry' WHERE entry=2849")
 switch('Character')
 db.query('''CREATE TABLE IF NOT EXISTS rp_registry_name(player INT UNSIGNED PRIMARY KEY,last_changed BIGINT UNSIGNED NOT NULL) ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_registry_history(id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,player INT UNSIGNED NOT NULL,old_name VARCHAR(25) NOT NULL,new_name VARCHAR(25) NOT NULL,reason VARCHAR(16) NOT NULL,changed_at BIGINT UNSIGNED NOT NULL,INDEX(player)) ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_family(id INT UNSIGNED PRIMARY KEY,surname VARCHAR(12) NOT NULL,design INT UNSIGNED NOT NULL DEFAULT 0,created_at BIGINT UNSIGNED NOT NULL) ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_family_member(player INT UNSIGNED PRIMARY KEY,family_id INT UNSIGNED NOT NULL,INDEX(family_id)) ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_registry_contract(owner INT UNSIGNED PRIMARY KEY,partner INT UNSIGNED NOT NULL DEFAULT 0,owner_name VARCHAR(25) NOT NULL,partner_name VARCHAR(25) NOT NULL DEFAULT '',surname VARCHAR(12) NOT NULL DEFAULT '') ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_registry_participant(player INT UNSIGNED PRIMARY KEY,owner INT UNSIGNED NOT NULL,INDEX(owner)) ENGINE=InnoDB;
 CREATE TABLE IF NOT EXISTS rp_registry_ring(item_guid INT UNSIGNED PRIMARY KEY,wearer INT UNSIGNED NOT NULL,spouse INT UNSIGNED NOT NULL,engraving VARCHAR(25) NOT NULL,contract_owner INT UNSIGNED NOT NULL,INDEX(contract_owner)) ENGINE=InnoDB;''')
 db.query('''CREATE TABLE IF NOT EXISTS rp_registry_worn_ring(player INT UNSIGNED PRIMARY KEY,item_guid INT UNSIGNED NOT NULL UNIQUE) ENGINE=InnoDB''')
 print('Civil registry schema, owned NPCs and items prepared. Existing characters and guilds preserved. Backup:',backup)
