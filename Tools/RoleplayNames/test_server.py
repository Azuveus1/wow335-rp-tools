from pathlib import Path
import importlib.util,subprocess,time,shutil
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('rpdb',root/'Tools/RoleplayNames/Prepare.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);db=module.db
if b'worldserver.exe' in subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout.lower():raise RuntimeError('Stop worldserver before the isolated test.')
clause="guid BETWEEN 9000200 AND 9000203 OR name IN ('Rpfirst Rpstone','Rpfirst Rpriver','Rpother Rpstone')"
assert db.query('SELECT COUNT(*) FROM characters WHERE '+clause)=='0','Reserved fixture identities already used; refusing changes.'
source=db.query('SELECT guid FROM characters WHERE deleteDate IS NULL ORDER BY guid LIMIT 1');assert source.isdigit()
columns=[line.split('\t')[0] for line in db.query('SHOW COLUMNS FROM characters').splitlines() if line]
names=','.join('`'+c+'`' for c in columns)
def insert(guid,name,ignore=False):
 overrides={'guid':str(guid),'account':'0','name':"'"+name+"'",'online':'0'}
 values=','.join(overrides.get(c,'`'+c+'`') for c in columns)
 return f"INSERT {'IGNORE ' if ignore else ''}INTO characters ({names}) SELECT {values} FROM characters WHERE guid={source};"
helpers={};exec((root/'Tools/GMTransport/tests/server_startup.py').read_text(encoding='utf-8').split('root=pathlib.Path')[0],helpers);send=helpers['console_command']
folder=root/'build/rp-names-server-test';folder.mkdir(exist_ok=True)
shutil.copy2('C:/TC/build/bin/Release/worldserver.exe',folder/'worldserver.exe')
for dll in Path('C:/TC/bin').glob('*.dll'):shutil.copy2(dll,folder/dll.name)
config=folder/'config';config.mkdir(exist_ok=True);logs=folder/'logs';logs.mkdir(exist_ok=True)
(config/'roleplay.conf').write_text('[worldserver]\nLogsDir="'+logs.as_posix()+'"\nWorldServerPort=8086\nBindIP="127.0.0.1"\nLogger.scripts=3,Server\n',encoding='utf-8')
log=logs/'Server.log';proc=None;owned=False
def wait(needle):
 end=time.time()+150
 while time.time()<end and proc.poll() is None:
  text=log.read_text(errors='replace') if log.exists() else ''
  if 'Roleplay names selftest FAIL' in text:raise AssertionError('Core name validation/cache test failed.')
  if needle in text:return
  time.sleep(.5)
 raise RuntimeError('Server event missing: '+needle)
try:
 owned=True
 sql=insert(9000200,'Rpfirst Rpstone')+insert(9000201,'Rpfirst Rpriver')+insert(9000202,'Rpother Rpstone')
 db.query(sql)
 assert db.query('SELECT COUNT(*) FROM characters WHERE guid BETWEEN 9000200 AND 9000202')=='3'
 assert db.query(insert(9000203,'Rpfirst Rpstone',True)+'SELECT ROW_COUNT()')=='0','Exact duplicate full name was accepted.'
 assert db.query(insert(9000203,'RPFIRST RPSTONE',True)+'SELECT ROW_COUNT()')=='0','Case-variant duplicate full name was accepted.'
 db.query(insert(9000203,'Rpfirst'))
 if log.exists():log.unlink()
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 proc=subprocess.Popen([str(folder/'worldserver.exe'),'-c','C:/TC/bin/worldserver.conf','--config-dir',str(config)],cwd='C:/TC/bin',creationflags=subprocess.CREATE_NEW_CONSOLE,startupinfo=startup)
 wait('worldserver-daemon) ready');send(proc.pid,'rpnametest');wait('Roleplay names selftest PASS')
 send(proc.pid,'server shutdown 0');proc.wait(timeout=45);assert proc.returncode==0
 print('PASS: actual core name validation and capitalization; database retains distinct full names sharing first/family names; exact and case-variant duplicates rejected; full-name cache resolves distinct GUIDs; clean server shutdown.')
finally:
 if proc and proc.poll() is None:
  try:send(proc.pid,'server shutdown 0');proc.wait(timeout=35)
  except Exception:proc.terminate();proc.wait(timeout=20)
 if owned:db.query('DELETE FROM characters WHERE guid BETWEEN 9000200 AND 9000203 AND account=0')
