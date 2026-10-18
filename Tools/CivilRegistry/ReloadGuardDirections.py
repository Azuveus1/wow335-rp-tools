"""Reload only the four direction tables in an already-running worldserver."""
from pathlib import Path
import sys,time
root=Path(__file__).resolve().parents[2]
helpers={};exec((root/'Tools/GMTransport/tests/server_startup.py').read_text(encoding='utf-8').split('root=pathlib.Path')[0],helpers)
log=Path('C:/TC/bin/Server.log');before=log.stat().st_size
for table in ['points_of_interest','points_of_interest_locale','gossip_menu_option','gossip_menu_option_locale']:
 helpers['console_command'](int(sys.argv[1]),'reload '+table);time.sleep(.3)
end=time.time()+10
while time.time()<end:
 with log.open('rb') as f:f.seek(before);text=f.read().decode('utf-8',errors='replace')
 if 'Loaded' in text and 'gossip_menu_option_locale' in text:break
 time.sleep(.2)
print('Direction reload requested on the running server. New server log lines:')
print('\n'.join(line for line in text.splitlines() if any(word in line.lower() for word in ('poi','interest','gossip','reload'))))
