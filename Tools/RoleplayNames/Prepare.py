from pathlib import Path
import importlib.util,re,sys,datetime,json,subprocess
spec=importlib.util.spec_from_file_location('hunterdb',Path(__file__).parents[1]/'HunterPrototype/Prepare.py')
db=importlib.util.module_from_spec(spec);spec.loader.exec_module(db)
db.host,db.port,db.user,password,db.database=re.search(r'^\s*CharacterDatabaseInfo\s*=\s*"([^"]+)"',db.config,re.M).group(1).split(';');db.env['MYSQL_PWD']=password
if __name__=='__main__':
 print(db.query("SHOW COLUMNS FROM characters WHERE Field IN ('name','deleteInfos_Name')"))
 print(db.query("SHOW INDEX FROM characters WHERE Column_name='name'"))
 if '--apply' in sys.argv:
  if b'worldserver.exe' in subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout.lower():raise RuntimeError('Stop worldserver before changing the name column.')
  if db.query("SELECT COUNT(*) FROM (SELECT name FROM characters GROUP BY name HAVING COUNT(*)>1) duplicates")!='0':raise RuntimeError('Duplicate names already exist; no automatic deletion or rename.')
  before=db.query('SELECT guid,name FROM characters ORDER BY guid')
  backup=Path(__file__).resolve().parents[2]/'build'/('rp-names-database-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
  backup.write_text(json.dumps({'schema':db.query('SHOW CREATE TABLE characters'),'names':before},ensure_ascii=False,indent=2),encoding='utf-8')
  db.query('ALTER TABLE characters MODIFY name VARCHAR(25) DEFAULT NULL, MODIFY deleteInfos_Name VARCHAR(25) NULL')
  indexes=db.query("SHOW INDEX FROM characters WHERE Column_name='name'")
  if not any(len(line.split('\t'))>1 and line.split('\t')[1]=='0' for line in indexes.splitlines()):db.query('CREATE UNIQUE INDEX rp_fullname_unique ON characters(name)')
  assert before==db.query('SELECT guid,name FROM characters ORDER BY guid'),'Existing names changed unexpectedly.'
  print('Database structure/name backup:',backup)
  print('Full character names enabled; existing characters retained.')
