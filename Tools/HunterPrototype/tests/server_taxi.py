"""Exercise actual escrow/mail SQL on two disposable offline fixture characters.
No existing player is charged or sent a test letter. Server must be stopped.
"""
from pathlib import Path
import subprocess,time,shutil,importlib.util,re
root=Path(r'C:/wowdecomp')
if b'worldserver.exe' in subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout.lower():raise RuntimeError('Stop worldserver first.')
spec=importlib.util.spec_from_file_location('prepare',root/'Tools/HunterPrototype/Prepare.py');db=importlib.util.module_from_spec(spec);spec.loader.exec_module(db)
db.host,db.port,db.user,password,db.database=re.search(r'^\s*CharacterDatabaseInfo\s*=\s*"([^"]+)"',db.config,re.M).group(1).split(';');db.env['MYSQL_PWD']=password
for table,clause in [('characters',"guid IN (9000100,9000101) OR name IN ('Huntbuyer','Hunttarget')"),('hunter_proto_contract','target IN (9000100,9000101)'),('hunter_proto_payment','target IN (9000100,9000101) OR buyer IN (9000100,9000101)'),('mail','receiver IN (9000100,9000101)')]:
 assert db.query('SELECT COUNT(*) FROM '+table+' WHERE '+clause)=='0','Reserved fixture identity already used; refusing changes.'
source=db.query('SELECT guid FROM characters WHERE deleteDate IS NULL ORDER BY guid LIMIT 1');assert source.isdigit()
columns=[line.split('\t')[0] for line in db.query('SHOW COLUMNS FROM characters').splitlines()]
helpers={};exec((root/'Tools/GMTransport/tests/server_startup.py').read_text(encoding='utf-8').split('root=pathlib.Path')[0],helpers);send=helpers['console_command']
folder=root/'build/hunter-taxi-test';folder.mkdir(exist_ok=True)
shutil.copy2(r'C:/TC/build/bin/Release/worldserver.exe',folder/'worldserver.exe')
for dll in Path(r'C:/TC/bin').glob('*.dll'):shutil.copy2(dll,folder/dll.name)
conf=folder/'config';conf.mkdir(exist_ok=True);logs=folder/'logs';logs.mkdir(exist_ok=True)
(conf/'prototype.conf').write_text('[worldserver]\nLogsDir="'+logs.as_posix()+'"\nWorldServerPort=8086\nBindIP="127.0.0.1"\nLogger.scripts=3,Server\n',encoding='utf-8')
log=logs/'Server.log';proc=None;owned=False

def wait_for(needle,seconds=140):
 end=time.time()+seconds
 while time.time()<end and proc.poll() is None:
  text=log.read_text(errors='replace') if log.exists() else ''
  if needle in text:return text
  if 'Hunter travel blocked:' in text:raise AssertionError('Mercenary navigation/flight blocked; inspect isolated log.')
  if 'Hunter finance selftest' in text and 'FAIL' in text:raise AssertionError('Finance selftest failed; inspect isolated log.')
  time.sleep(.5)
 raise AssertionError('Missing server event: '+needle)
def start():
 global proc
 if log.exists():log.unlink()
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 proc=subprocess.Popen([str(folder/'worldserver.exe'),'-c',r'C:/TC/bin/worldserver.conf','--config-dir',str(conf)],cwd=r'C:/TC/bin',creationflags=subprocess.CREATE_NEW_CONSOLE,startupinfo=startup)
 wait_for('worldserver-daemon) ready')
def stop():
 send(proc.pid,'server shutdown 0');proc.wait(timeout=45);assert proc.returncode==0
try:
 owned=True
 for guid,name,money in [(9000100,'Huntbuyer',5000000),(9000101,'Hunttarget',0)]:
  overrides={'guid':str(guid),'account':'0','name':"'"+name+"'",'money':str(money),'online':'0','level':'30'}
  names=','.join('`'+c+'`' for c in columns);values=','.join(overrides.get(c,'`'+c+'`') for c in columns)
  db.query(f'INSERT INTO characters ({names}) SELECT {values} FROM characters WHERE guid={source}')
 start();send(proc.pid,'huntercontract boardcheck');board_log=wait_for('Hunter board check PASS',20)
 pose=re.search(r'Hunter board check PASS: spawn=\d+ x=([\d.-]+) y=([\d.-]+) z=([\d.-]+) floor=([\d.-]+)',board_log)
 assert pose and abs(float(pose[3])-float(pose[4]))<.5,'Board is too far from the measured ground (preserve its GM-adjusted placement).'
 send(proc.pid,'huntercontract selftest');wait_for('Hunter combat selftest PASS',20)
 send(proc.pid,'huntercontract financialselftest 1');wait_for('Hunter finance selftest stage=1 PASS',30)
 assert db.query('SELECT money FROM characters WHERE guid=9000100')=='2500000'
 assert db.query('SELECT money FROM characters WHERE guid=9000101')=='0'
 assert db.query('SELECT amount,state FROM hunter_proto_payment WHERE target=9000101')=='2500000\t1'
 assert db.query("SELECT COUNT(*) FROM mail WHERE receiver=9000101 AND subject='Un contrat a ete mis sur votre tete'")=='1'
 assert db.query('SELECT SUM(money) FROM mail WHERE receiver=9000100')=='0'
 send(proc.pid,'huntercontract taxitest');flight_log=wait_for('Hunter taxi selftest PASS',360)
 assert 'source=25 path=44' in flight_log,'Expected a real connecting flight at Crossroads.'
 stop();shutil.copy2(log,logs/'before-restart.log')
 start();wait_for('Hunter contract restored target=9000101 outcome=2',20)
 send(proc.pid,'huntercontract boardcheck');wait_for('Hunter board check PASS',20)
 send(proc.pid,'huntercontract financialselftest 2');wait_for('Hunter finance selftest stage=2 PASS',30)
 assert db.query('SELECT money FROM characters WHERE guid=9000101')=='2500000'
 assert db.query('SELECT money FROM characters WHERE guid=9000100')=='0'
 assert db.query('SELECT amount,state FROM hunter_proto_payment WHERE target=9000101 ORDER BY id').splitlines()==['2500000\t2','2500000\t4']
 assert db.query('SELECT SUM(money) FROM mail WHERE receiver=9000100')=='2500000'
 assert db.query("SELECT COUNT(*) FROM mail WHERE receiver=9000101 AND subject='Hunter test rollback'")=='0'
 assert db.query("SELECT COUNT(*) FROM mail WHERE receiver=9000101 AND subject='Un contrat a ete mis sur votre tete'")=='2'
 send(proc.pid,'huntercontract cluetest 1');wait_for('Hunter taxi clue accepted: target=9000101 playerDeparture=22 playerDestination=40',20)
 assert db.query('SELECT destination FROM hunter_proto_pursuit WHERE target=9000101')=='40'
 stop();shutil.copy2(log,logs/'before-clue-restart.log')
 start();send(proc.pid,'huntercontract cluetest 2');wait_for('Hunter clue persistence PASS',20);stop()
 print('PASS: real inn-to-flightmaster walk, real Horde DBC flight and in-flight resume; accepted destination clue survives restart; real private corpse with 250g; escrow debited once; alert mail saved; duplicate refused; pending reward survives restart; credit exactly once; cancellation refund sent exactly once. Only disposable fixtures used.')
finally:
 if proc and proc.poll() is None:
  try:stop()
  except Exception:proc.terminate();proc.wait(timeout=20)
 if owned:
  db.query('DELETE FROM hunter_proto_pursuit WHERE target IN (9000100,9000101); DELETE FROM mail_items WHERE receiver IN (9000100,9000101); DELETE FROM mail WHERE receiver IN (9000100,9000101); DELETE FROM hunter_proto_contract WHERE target IN (9000100,9000101); DELETE FROM hunter_proto_payment WHERE target IN (9000100,9000101); DELETE FROM characters WHERE guid IN (9000100,9000101) AND account=0')
