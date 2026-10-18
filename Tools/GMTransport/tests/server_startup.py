"""Temporary isolated route in the editor tables; start/stop server and remove it.
Run only while the user's worldserver is stopped. Never touches gameobject spawns.
"""
import os,re,subprocess,time,pathlib,shutil
import ctypes
import struct
from ctypes import wintypes

def console_command(pid,text):
 # TrinityCore uses ReadConsoleW on Windows, so a stdin pipe cannot deliver commands.
 kernel=ctypes.WinDLL('kernel32',use_last_error=True)
 kernel.FreeConsole()
 if not kernel.AttachConsole(pid):raise ctypes.WinError(ctypes.get_last_error())
 class Key(ctypes.Structure):
  _fields_=[('down',wintypes.BOOL),('repeat',wintypes.WORD),('vk',wintypes.WORD),('scan',wintypes.WORD),('char',wintypes.WCHAR),('state',wintypes.DWORD)]
 class Event(ctypes.Union):
  _fields_=[('key',Key),('padding',ctypes.c_byte*16)]
 class Input(ctypes.Structure):
  _fields_=[('kind',wintypes.WORD),('event',Event)]
 kernel.CreateFileW.restype=wintypes.HANDLE
 kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
 kernel.WriteConsoleInputW.argtypes=[wintypes.HANDLE,ctypes.POINTER(Input),wintypes.DWORD,ctypes.POINTER(wintypes.DWORD)]
 kernel.CloseHandle.argtypes=[wintypes.HANDLE]
 handle=kernel.CreateFileW('CONIN$',0xC0000000,3,None,3,0,None)
 try:
  records=[]
  for c in text+'\r':
   for down in (True,False):
    r=Input();r.kind=1;r.event.key=Key(down,1,ord(c.upper()),0,c,0);records.append(r)
  batch=(Input*len(records))(*records);written=wintypes.DWORD()
  if not kernel.WriteConsoleInputW(handle,batch,len(batch),ctypes.byref(written)):raise ctypes.WinError(ctypes.get_last_error())
 finally:
  kernel.CloseHandle(handle);kernel.FreeConsole()
root=pathlib.Path(r'C:\wowdecomp')
testbin=root/'build/transport-server-test';testbin.mkdir(exist_ok=True)
shutil.copy2(r'C:\TC\build\bin\Release\worldserver.exe',testbin/'worldserver.exe')
for dll in pathlib.Path(r'C:\TC\bin').glob('*.dll'):shutil.copy2(dll,testbin/dll.name)
config=pathlib.Path(r'C:\TC\bin\worldserver.conf').read_text(encoding='utf-8-sig')
h,p,u,w,d=re.search(r'^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"',config,re.M).group(1).split(';')
env=os.environ.copy();env['MYSQL_PWD']=w
def query(sql):
 r=subprocess.run([r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe','--host='+h,'--port='+p,'--user='+u,'--database='+d,'--batch','--skip-column-names'],input=sql.encode(),env=env,capture_output=True)
 if r.returncode:raise RuntimeError('Test SQL failed (credentials omitted)')
 return r.stdout.decode().strip()
entry=9999999
assert query(f'SELECT COUNT(*) FROM gm_transport_route WHERE entry={entry}')=='0'
assert query(f'SELECT COUNT(*) FROM gameobject_template WHERE entry={entry}')=='0'
query(f'INSERT INTO gm_transport_route VALUES ({entry},20808,1,10,1)')
for i,(x,y) in enumerate([(-5700,-2900),(-5600,-2900),(-5600,-2800),(-5700,-2800)]):
 query(f'INSERT INTO gm_transport_route_node VALUES ({entry},{i},{x},{y},200,2)')
proc=None
native_entry=20808
native_inserted=False
multi_entry=9999998
multi_inserted=False
ann_inserted=False
try:
 assert query(f'SELECT COUNT(*) FROM gm_transport_native WHERE entry={native_entry}')=='0','Test native entry already edited; refusing to overwrite'
 path,speed=map(int,query(f'SELECT data0,data1 FROM gameobject_template WHERE entry={native_entry}').split())
 dbc=pathlib.Path(r'C:\TC\data\dbc\TaxiPathNode.dbc').read_bytes()
 magic,count,fields,size,strings=struct.unpack_from('<4s4I',dbc)
 nodes=[]
 for i in range(count):
  node=struct.unpack_from('<4I3f4I',dbc,20+i*size)
  if node[1]==path:nodes.append(node)
 nodes.sort(key=lambda n:n[2]);assert len(nodes)>=4
 query(f'INSERT INTO gm_transport_native VALUES ({native_entry},{speed})');native_inserted=True
 # Insert a plain spline point and persist all metadata, retaining original UIDs.
 edited=[]
 for n in nodes:
  edited.append((n,n[2]+1))
  if len(edited)==2:
   nextnode=nodes[2]
   added=list(n);added[4:7]=[(n[k]+nextnode[k])/2 for k in (4,5,6)];added[7:]=[0,0,0,0]
   edited.append((added,2000000000))
 for i,(n,uid) in enumerate(edited):
  query(f'INSERT INTO gm_transport_node_meta VALUES ({native_entry},{i},{uid},{n[3]},{n[7]},{n[9]},{n[10]})')
  query(f'INSERT INTO gm_transport_native_node VALUES ({native_entry},{i},{n[4]+0.25},{n[5]},{n[6]},{n[8]})')
 assert query(f'SELECT COUNT(*) FROM gm_transport_route WHERE entry={multi_entry}')=='0'
 assert query(f'SELECT COUNT(*) FROM gameobject_template WHERE entry={multi_entry}')=='0'
 query(f'INSERT INTO gm_transport_route VALUES ({multi_entry},20808,1,40,1)');multi_inserted=True
 points=[(1,-5700,-2900),(1,-5680,-2900),(0,-9000,-100),(0,-8980,-100),(571,5800,600),(571,5820,600)]
 for i,(node_map,x,y) in enumerate(points):
  query(f'INSERT INTO gm_transport_route_node VALUES ({multi_entry},{i},{x},{y},200,2)')
  query(f'INSERT INTO gm_transport_route_node_map VALUES ({multi_entry},{i},{node_map})')
 query("INSERT INTO gm_transport_announcement(entry,node_uid,phase,delay_ms,spawn_id,map,mode,text,legacy_script,legacy_type,legacy_id,enabled) VALUES (9999999,1,0,21000,3463,1,1,'Temporary disabled announcement test',0,0,0,0)")
 ann_inserted=True
 for name in ['Server.log' ,'GM.log','DBErrors.log']:
  path=pathlib.Path(r'C:\TC\bin')/name
  if path.exists():shutil.copy2(path,root/'build'/('before-transport-test-'+name))
 log=root/'build/transport-server-startup.log'
 serverlog=pathlib.Path(r'C:\TC\bin\Server.log')
 before=serverlog.read_text(errors='replace') if serverlog.exists() else ''
 with log.open('wb') as output:
  startup=subprocess.STARTUPINFO();startup.dwFlags=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
  proc=subprocess.Popen([str(testbin/'worldserver.exe'),'-c',r'C:\TC\bin\worldserver.conf'],cwd=r'C:\TC\bin',creationflags=subprocess.CREATE_NEW_CONSOLE,startupinfo=startup)
  deadline=time.time()+180;ready=False
  while time.time()<deadline and proc.poll() is None:
   time.sleep(1)
   text=log.read_text(errors='replace')
   if serverlog.exists():
    current=serverlog.read_text(errors='replace')
    text+='\n'+(current[len(before):] if before and current.startswith(before) else current)
   if f'GM route {entry} activated' in text and f'GM route {multi_entry} activated' in text:
    ready=True;break
  if ready:
   console_command(proc.pid,'gmroute ann audit 9999999')
   time.sleep(2)
   console_command(proc.pid,'gmroute ann audit 176495')
   time.sleep(32)
   current=serverlog.read_text(errors="replace")
   text=current[len(before):] if before and current.startswith(before) else current
   console_command(proc.pid,'server shutdown 0')
   proc.wait(timeout=45)
  elif proc.poll() is None:
   console_command(proc.pid,'server shutdown 0')
   proc.wait(timeout=30)
  assert ready,'Test route did not activate; inspect startup log'
  assert f'Loaded native editor override {native_entry}' in text,'Native override was not loaded'
  assert 'GM announcement audit 9999999: 1 entries' in text,'Temporary persisted announcement was not loaded correctly'
  assert 'GM announcement audit route 9999999 uid 1 NPC 3463 delay 21000 phase 0 original false' in text,'Temporary binding / delay not recognized'
  assert re.search(r'GM announcement audit 176495: [2-9][0-9]* entries',text),'Other Blizzard announcements not recognized'
  assert proc.returncode==0,'Server shutdown failed'
  for node_map in (0,571,1): assert f'GM route {multi_entry} transitioned to map {node_map}' in text,f'Missing transition to {node_map}'
 print('PASS: inserted native waypoint and metadata, persisted announcement override and Blizzard discovery; native override and legacy route loaded; new route crossed maps 1 -> 0 -> 571 -> 1 repeatedly; clean shutdown')
finally:
 if proc and proc.poll() is None:
  proc.terminate();proc.wait(timeout=20)
 if ann_inserted:query("DELETE FROM gm_transport_announcement WHERE entry=9999999 AND text='Temporary disabled announcement test'")
 if native_inserted:query(f'DELETE FROM gm_transport_node_meta WHERE entry={native_entry}')
 if multi_inserted:query(f'DELETE FROM gm_transport_route_node WHERE entry={multi_entry}; DELETE FROM gm_transport_route WHERE entry={multi_entry}')
 if native_inserted:query(f'DELETE FROM gm_transport_native_node WHERE entry={native_entry}; DELETE FROM gm_transport_native WHERE entry={native_entry}')
 query(f'DELETE FROM gm_transport_route_node WHERE entry={entry}; DELETE FROM gm_transport_route WHERE entry={entry}')
 print('Temporary editor route removed.')
