from pathlib import Path
import shutil,datetime
root=Path('C:/TC/TrinityCore/src/server/scripts/Custom');task=Path(__file__).parent
backup=task.resolve().parents[1]/'build'/('rp-greetings-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True)
for source,target in [(task/'rp_greetings.cpp',root/'rp_greetings.cpp'),(task/'GreetingPolicy.h',root/'RPGreetingPolicy.h')]:
 if target.exists():shutil.copy2(target,backup/target.name)
 shutil.copy2(source,target)
loader=root/'custom_script_loader.cpp';raw=loader.read_bytes();s=raw.decode('utf-8',errors='surrogateescape')
if 'void AddSC_rp_greetings();' not in s:
 (backup/loader.name).write_bytes(raw)
 anchor='void AddCustomScripts()\n{'
 if s.count(anchor)!=1:raise RuntimeError('Custom script loader anchor changed.')
 s='void AddSC_rp_greetings();\n'+s.replace(anchor,anchor+'\n    AddSC_rp_greetings();',1)
 loader.write_bytes(s.encode('utf-8',errors='surrogateescape'))
print('Source backup:',backup)
