"""Disposable account-0 identities; never exercise actions on real characters."""
from pathlib import Path
import subprocess,shutil,time
from Prepare import db,switch
root=Path(__file__).resolve().parents[2];switch('Character')
if b'worldserver.exe' in subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout.lower():raise RuntimeError('Stop worldserver before this isolated test.')
assert db.query('SELECT COUNT(*) FROM characters WHERE guid BETWEEN 9000300 AND 9000302')=='0','Fixture GUID in use'
assert db.query('SELECT COUNT(*) FROM rp_family WHERE id=9000300')=='0','Fixture family ID in use'
assert db.query('SELECT COUNT(*) FROM rp_registry_birth_name WHERE player BETWEEN 9000300 AND 9000302')=='0','Fixture birth identity in use'
assert db.query("SELECT COUNT(*) FROM characters WHERE name IN ('Rpfirst Rpstone','Rpother Rpriver','Rpfirst Rpcliff','Rpother Rpcliff')")=='0','Fixture name in use'
before=db.query('SELECT guid,name,money FROM characters ORDER BY guid')
guilds=db.query('SELECT * FROM guild_member ORDER BY guid')
source=db.query('SELECT guid FROM characters WHERE deleteDate IS NULL ORDER BY guid LIMIT 1');assert source.isdigit()
columns=[x.split('\t')[0] for x in db.query('SHOW COLUMNS FROM characters').splitlines()]
def insert(guid,name):
 overrides={'guid':str(guid),'account':'0','name':"'"+name+"'",'online':'0'}
 return 'INSERT INTO characters ('+','.join('`'+c+'`' for c in columns)+') SELECT '+','.join(overrides.get(c,'`'+c+'`') for c in columns)+' FROM characters WHERE guid='+source+';'
helpers={};exec((root/'Tools/GMTransport/tests/server_startup.py').read_text(encoding='utf-8').split('root=pathlib.Path')[0],helpers);send=helpers['console_command']
folder=root/'build/civil-registry-server-test';folder.mkdir(exist_ok=True)
shutil.copy2('C:/TC/build/bin/Release/worldserver.exe',folder/'worldserver.exe')
for dll in Path('C:/TC/bin').glob('*.dll'):shutil.copy2(dll,folder/dll.name)
config=folder/'config';config.mkdir(exist_ok=True);logs=folder/'logs';logs.mkdir(exist_ok=True)
(config/'registry.conf').write_text('[worldserver]\nLogsDir="'+logs.as_posix()+'"\nWorldServerPort=8086\nBindIP="127.0.0.1"\nLogger.scripts=3,Server\n',encoding='utf-8')
log=logs/'Server.log';proc=None;owned=False
def wait(needle):
 end=time.time()+150
 while time.time()<end and proc.poll() is None:
  text=log.read_text(errors='replace') if log.exists() else ''
  if 'Civil registry selftest FAIL' in text:raise AssertionError('Civil registry selftest failed; see '+str(log))
  if needle in text:return
  time.sleep(.5)
 raise RuntimeError('Missing server event: '+needle)
try:
 owned=True;db.query(insert(9000300,'Rpfirst Rpstone')+insert(9000301,'Rpother Rpriver'))
 if log.exists():log.unlink()
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 proc=subprocess.Popen([str(folder/'worldserver.exe'),'-c','C:/TC/bin/worldserver.conf','--config-dir',str(config)],cwd='C:/TC/bin',creationflags=subprocess.CREATE_NEW_CONSOLE,startupinfo=startup)
 wait('worldserver-daemon) ready');send(proc.pid,'civiltest');wait('Civil registry selftest PASS')
 send(proc.pid,'server shutdown 0');proc.wait(timeout=45);assert proc.returncode==0
 assert db.query('SELECT COUNT(*) FROM rp_registry_history WHERE player BETWEEN 9000300 AND 9000301')=='4'
 assert 'Birth identity protection PASS' in log.read_text(errors='replace')
 assert 'Wedding slot persistence PASS' in log.read_text(errors='replace')
 assert db.query('SELECT design FROM rp_family WHERE id=9000300')=='15033779'
 assert 'Civil registry loaded:' in log.read_text(errors='replace')
 assert 'Civil registry inventory objects PASS' in log.read_text(errors='replace')
 assert 'Civil registry rollback test: expected duplicate fixture membership follows' in log.read_text(errors='replace')
 print('PASS: real server registration, two spawned human NPCs and floor checks, persistent shared family name/history/cooldown/style, full-name cache refresh, clean shutdown.')
finally:
 if proc and proc.poll() is None:
  try:send(proc.pid,'server shutdown 0');proc.wait(timeout=35)
  except Exception:proc.terminate();proc.wait(timeout=20)
 if owned:
  db.query('DELETE FROM rp_registry_name_claim WHERE player BETWEEN 9000300 AND 9000302;DELETE FROM rp_registry_birth_name WHERE player BETWEEN 9000300 AND 9000302;DELETE FROM item_instance WHERE owner_guid BETWEEN 9000300 AND 9000302;DELETE FROM character_inventory WHERE guid BETWEEN 9000300 AND 9000302;DELETE FROM rp_registry_worn_ring WHERE player BETWEEN 9000300 AND 9000302;DELETE FROM rp_registry_ring WHERE wearer BETWEEN 9000300 AND 9000302;DELETE FROM rp_registry_history WHERE player BETWEEN 9000300 AND 9000302;DELETE FROM rp_registry_name WHERE player BETWEEN 9000300 AND 9000302;DELETE FROM rp_family_member WHERE family_id=9000300;DELETE FROM rp_family WHERE id=9000300;DELETE FROM characters WHERE guid BETWEEN 9000300 AND 9000302 AND account=0;')
  assert before==db.query('SELECT guid,name,money FROM characters ORDER BY guid'),'Real character data changed'
  assert guilds==db.query('SELECT * FROM guild_member ORDER BY guid'),'Guild membership changed'
