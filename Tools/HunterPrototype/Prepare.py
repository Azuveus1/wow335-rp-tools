"""Install the owned prototype template and its two configurable endpoints only."""
from pathlib import Path
import os,re,subprocess,datetime
config=Path(r'C:/TC/bin/worldserver.conf').read_text(encoding='utf-8-sig')
host,port,user,password,database=re.search(r'^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"',config,re.M).group(1).split(';')
env=os.environ.copy();env['MYSQL_PWD']=password
def query(sql):
 r=subprocess.run([r'C:/Program Files/MySQL/MySQL Server 8.0/bin/mysql.exe','--host='+host,'--port='+port,'--user='+user,'--database='+database,'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names'],input=sql.encode('utf-8'),env=env,capture_output=True)
 if r.returncode:raise RuntimeError('Prototype database operation failed (connection details omitted).')
 return r.stdout.decode('utf-8').strip()
if __name__=='__main__':
 entry=9100100
 existing=query(f'SELECT ScriptName FROM creature_template WHERE entry={entry}')
 if existing and existing!='npc_hunter_navigation_proto':raise RuntimeError('Prototype entry is already used by another NPC; refusing overwrite.')
 if not query('SELECT entry FROM creature_template WHERE entry=3296'):raise RuntimeError('Source NPC template missing.')
 columns=[line.split('\t')[0] for line in query('SHOW COLUMNS FROM creature_template').splitlines()]
 overrides={'entry':str(entry),'name':"'Mercenaire eclaireur'",'subname':"'Prototype de navigation'",'AIName':"''",'ScriptName':"'npc_hunter_navigation_proto'",'npcflag':'1','faction':'35','gossip_menu_id':'0','MovementType':'0','InhabitType':'1','minlevel':'60','maxlevel':'60','unit_flags':'0','unit_flags2':'0'}
 names=','.join('`'+c+'`' for c in columns);values=','.join(overrides.get(c,'`'+c+'`') for c in columns)
 # Only the new owned template is inserted. Existing prototype customizations are preserved.
 query(f'INSERT IGNORE INTO creature_template ({names}) SELECT {values} FROM creature_template WHERE entry=3296')
 query('''CREATE TABLE IF NOT EXISTS hunter_proto_point (
 name VARCHAR(16) NOT NULL PRIMARY KEY,map INT UNSIGNED NOT NULL,
 x DOUBLE NOT NULL,y DOUBLE NOT NULL,z DOUBLE NOT NULL,o DOUBLE NOT NULL,seat TINYINT UNSIGNED NOT NULL DEFAULT 1
 ) ENGINE=InnoDB;
 INSERT IGNORE INTO hunter_proto_point VALUES ('inn',1,1631.2,-4438.2,15.7436,2.75762,1),('flight',1,1677.8,-4316.0,61.9445,5.25344,0);''')
 print('Owned prototype template and endpoints ready; existing game spawns unchanged.')
# Contract actor has its own template: leave the original navigation test untouched.
if __name__=='__main__':
 contract_entry=9100101
 existing=query(f'SELECT ScriptName FROM creature_template WHERE entry={contract_entry}')
 if existing and existing!='npc_hunter_contract_proto':raise RuntimeError('Contract entry already belongs to another script.')
 contract_overrides=dict(overrides)
 contract_overrides.update(entry=str(contract_entry),name="'Mercenaire traqueur'",subname="'Contrat de test'",ScriptName="'npc_hunter_contract_proto'",npcflag='0',faction='14',rank='1',HealthModifier='2',DamageModifier='1',RegenHealth='0')
 values=','.join(contract_overrides.get(c,'`'+c+'`') for c in columns)
 query(f'INSERT IGNORE INTO creature_template ({names}) SELECT {values} FROM creature_template WHERE entry=3296')
 # Same credentials, but contract identity/state belongs to the character database.
 host,port,user,password,database=re.search(r'^\s*CharacterDatabaseInfo\s*=\s*"([^"]+)"',config,re.M).group(1).split(';')
 env['MYSQL_PWD']=password
 query('''CREATE TABLE IF NOT EXISTS hunter_proto_contract (
 target INT UNSIGNED NOT NULL PRIMARY KEY,buyer INT UNSIGNED NOT NULL,
 map INT UNSIGNED NOT NULL,x DOUBLE NOT NULL,y DOUBLE NOT NULL,z DOUBLE NOT NULL,o DOUBLE NOT NULL,
 level INT UNSIGNED NOT NULL,health DOUBLE NOT NULL DEFAULT 1,outcome INT UNSIGNED NOT NULL DEFAULT 0
 ) ENGINE=InnoDB''')
 print('Contract template and persistent character contract table ready.')

if __name__=='__main__':
 if not query("SHOW COLUMNS FROM hunter_proto_contract LIKE 'payment_id'"):
  query('ALTER TABLE hunter_proto_contract ADD COLUMN payment_id BIGINT UNSIGNED NOT NULL DEFAULT 0')
 query('''CREATE TABLE IF NOT EXISTS hunter_proto_payment (
 id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,target INT UNSIGNED NOT NULL,
 buyer INT UNSIGNED NOT NULL,amount INT UNSIGNED NOT NULL,state INT UNSIGNED NOT NULL DEFAULT 0,
 alert_mail INT UNSIGNED NOT NULL DEFAULT 0,created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 INDEX (target)
 ) ENGINE=InnoDB''')
 print('Escrow ledger ready; legacy contracts retain a zero bounty.')
if __name__=='__main__':
 query('''CREATE TABLE IF NOT EXISTS hunter_proto_pursuit (
 target INT UNSIGNED NOT NULL PRIMARY KEY,payment_id BIGINT UNSIGNED NOT NULL,
 clue_source INT UNSIGNED NOT NULL DEFAULT 0,destination INT UNSIGNED NOT NULL DEFAULT 0,
 phase INT UNSIGNED NOT NULL DEFAULT 0,source INT UNSIGNED NOT NULL DEFAULT 0,path INT UNSIGNED NOT NULL DEFAULT 0,
 next_node INT UNSIGNED NOT NULL DEFAULT 0,flight_to INT UNSIGNED NOT NULL DEFAULT 0
 ) ENGINE=InnoDB''')
 print('Taxi clue and travel checkpoint table ready.')
