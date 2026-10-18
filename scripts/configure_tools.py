"""Adapt historical Windows development scripts copied into the client checkout.

Only the copied Tools directory is edited; the publication stays portable.
No credentials are accepted or stored here. See local.example.json.
"""
from pathlib import Path
import argparse,json

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('config',type=Path);a=ap.parse_args()
    c=json.loads(a.config.read_text(encoding='utf-8-sig'))
    pairs=[('C:/TC/TrinityCore',c['server']),('C:/TC',c['server_home']),('C:/Jeux/WoW335a',c['game']),('C:/wowdecomp',c['client']),('C:/Program Files/MySQL/MySQL Server 8.0/bin/mysql.exe',c['mysql'])]
    root=Path(c['client']).resolve()/'Tools'
    if not (root/'CivilRegistry/BirthNames.h').is_file():raise RuntimeError('Apply the source bundle first.')
    count=0
    for p in root.rglob('*'):
        if not p.is_file() or p.suffix not in {'.py','.ps1','.txt','.md'}:continue
        text=p.read_text(encoding='utf-8-sig');before=text
        for old,new in pairs:
            new=Path(new).as_posix()
            if "'" in new or '"' in new or '\n' in new:raise RuntimeError('Unsupported quote in path.')
            for variant in (old,old.replace('/','\\\\'),old.replace('/','\\')):text=text.replace(variant,new)
        if text!=before:p.write_text(text,encoding='utf-8');count+=1
    print('Configured',count,'development files; no database operation performed.')
if __name__=='__main__':main()
