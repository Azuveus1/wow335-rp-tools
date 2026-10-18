"""Check that the new global PlayerScript loads and the server stops cleanly.
Does not create characters, NPCs, contracts, or change database definitions.
"""
from pathlib import Path
import subprocess,shutil,time
root=Path(__file__).resolve().parents[2]
if b'worldserver.exe' in subprocess.run(['tasklist','/FI','IMAGENAME eq worldserver.exe','/FO','CSV','/NH'],capture_output=True).stdout.lower():raise RuntimeError('Stop worldserver before the isolated startup test.')
helpers={};exec((root/'Tools/GMTransport/tests/server_startup.py').read_text(encoding='utf-8').split('root=pathlib.Path')[0],helpers);send=helpers['console_command']
folder=root/'build/rp-greetings-server-test';folder.mkdir(exist_ok=True)
shutil.copy2('C:/TC/build/bin/Release/worldserver.exe',folder/'worldserver.exe')
for dll in Path('C:/TC/bin').glob('*.dll'):shutil.copy2(dll,folder/dll.name)
config=folder/'config';config.mkdir(exist_ok=True);logs=folder/'logs';logs.mkdir(exist_ok=True)
(config/'greetings.conf').write_text('[worldserver]\nLogsDir="'+logs.as_posix()+'"\nWorldServerPort=8086\nBindIP="127.0.0.1"\nLogger.scripts=3,Server\n',encoding='utf-8')
log=logs/'Server.log';proc=None
try:
 if log.exists():log.unlink()
 startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
 proc=subprocess.Popen([str(folder/'worldserver.exe'),'-c','C:/TC/bin/worldserver.conf','--config-dir',str(config)],cwd='C:/TC/bin',creationflags=subprocess.CREATE_NEW_CONSOLE,startupinfo=startup)
 ready=False;end=time.time()+150
 while time.time()<end and proc.poll() is None:
  text=log.read_text(errors='replace') if log.exists() else ''
  if 'worldserver-daemon) ready' in text:
   assert 'RP greetings ready: selected friendly NPC, range=10m, cooldown=600s per player/NPC' in text,'Greeting PlayerScript did not register.'
   ready=True;break
  time.sleep(.5)
 assert ready,'Worldserver did not start; inspect isolated log.'
 send(proc.pid,'server shutdown 0');proc.wait(timeout=45);assert proc.returncode==0
 print('PASS: global targeted-greeting PlayerScript registered; worldserver initialized and shut down cleanly. No test NPCs, characters or database schema changes.')
finally:
 if proc and proc.poll() is None:
  try:send(proc.pid,'server shutdown 0');proc.wait(timeout=35)
  except Exception:proc.terminate();proc.wait(timeout=20)
