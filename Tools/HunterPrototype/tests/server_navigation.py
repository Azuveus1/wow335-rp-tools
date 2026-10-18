"""Run the actual mercenary AI in an isolated server while the normal server is stopped."""
from pathlib import Path
import subprocess,time,shutil
root=Path(r'C:/wowdecomp')
listing=subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout
if b'worldserver.exe' in listing.lower():raise RuntimeError('Stop the normal worldserver before the prototype test.')
helpers={};exec((root/'Tools/GMTransport/tests/server_startup.py').read_text(encoding='utf-8').split('root=pathlib.Path')[0],helpers)
send=helpers['console_command']
folder=root/'build/hunter-prototype-test';folder.mkdir(exist_ok=True)
shutil.copy2(r'C:/TC/build/bin/Release/worldserver.exe',folder/'worldserver.exe')
for dll in Path(r'C:/TC/bin').glob('*.dll'):shutil.copy2(dll,folder/dll.name)
overrides=folder/'config';overrides.mkdir(exist_ok=True)
logs=folder/'logs';logs.mkdir(exist_ok=True)
(overrides/'prototype.conf').write_text('[worldserver]\nLogsDir="'+logs.as_posix()+'"\nWorldServerPort=8086\nBindIP="127.0.0.1"\nLogger.scripts=3,Server\n',encoding='utf-8')
log=logs/'Server.log'
proc=None
try:
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 proc=subprocess.Popen([str(folder/'worldserver.exe'),'-c',r'C:/TC/bin/worldserver.conf','--config-dir',str(overrides)],cwd=r'C:/TC/bin',creationflags=subprocess.CREATE_NEW_CONSOLE,startupinfo=startup)
 started=False;deadline=time.time()+150
 while time.time()<deadline and proc.poll() is None:
  text=log.read_text(errors='replace') if log.exists() else ''
  if 'worldserver-daemon) ready' in text:started=True;break
  time.sleep(1)
 assert started,'Prototype server did not initialize; inspect isolated test log.'
 send(proc.pid,'hunterproto selftest')
 passed=False;deadline=time.time()+160
 while time.time()<deadline and proc.poll() is None:
  text=log.read_text(errors='replace')
  if 'Hunter prototype blocked:' in text:raise AssertionError('Actual NPC path blocked; inspect isolated test log.')
  if 'Hunter prototype selftest_pass:' in text:passed=True;break
  time.sleep(1)
 assert passed,'Mercenary did not complete the round trip within the test timeout.'
 assert 'state=waiting_at_flightmaster' in text and 'state=resting' in text
 send(proc.pid,'hunterproto remove');time.sleep(1)
 send(proc.pid,'server shutdown 0');proc.wait(timeout=45)
 assert proc.returncode==0,'Prototype test server did not shut down cleanly.'
 print('PASS: actual NPC left inn, traversed Orgrimmar navmesh to Doras, waited, walked back and sat down; clean shutdown. No player gold or contracts involved.')
finally:
 if proc and proc.poll() is None:
  try:send(proc.pid,'hunterproto remove');send(proc.pid,'server shutdown 0');proc.wait(timeout=30)
  except Exception:proc.terminate();proc.wait(timeout=20)
