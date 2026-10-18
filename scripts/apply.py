"""Apply the published integration to clean checkouts at the pinned revisions.

Checks are read-only unless --apply is supplied. No configuration, DB or executable
is copied. Review the two patch files before applying to your own installation.
"""
from pathlib import Path
import argparse, json, shutil, subprocess

PACKAGE=Path(__file__).resolve().parents[1]
def git(root,*args):
    p=subprocess.run(['git','-C',str(root),*args],capture_output=True,text=True)
    if p.returncode:raise RuntimeError(p.stderr.strip())
    return p.stdout.strip()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--apply',action='store_true')
    a=p.parse_args(); roots={'client':a.client.resolve(),'server':a.server.resolve()}
    if roots['client']==roots['server']:raise RuntimeError('Client and server must be separate checkouts.')
    upstream=json.loads((PACKAGE/'upstream.json').read_text())
    operations=[]
    for kind,root in roots.items():
        if git(root,'rev-parse','HEAD')!=upstream[kind]['commit']:raise RuntimeError(kind+': unexpected base revision.')
        if git(root,'status','--porcelain','--untracked-files=all'):raise RuntimeError(kind+': checkout must be clean, including untracked files.')
        patch=PACKAGE/'patches'/(kind+'.patch')
        git(root,'apply','--check',str(patch))
        for source in (PACKAGE/'overlays'/kind).rglob('*'):
            if source.is_file():operations.append((source,root/source.relative_to(PACKAGE/'overlays'/kind)))
    for source in (PACKAGE/'Tools').rglob('*'):
        if source.is_file():operations.append((source,roots['client']/source.relative_to(PACKAGE)))
    for source,target in operations:
        if target.exists():raise RuntimeError('Refusing overwrite: '+str(target))
    print('Both revisions, patches and',len(operations),'new files checked.')
    if not a.apply:
        print('Read-only check complete. Add --apply to write source changes.');return
    for kind,root in roots.items():git(root,'apply',str(PACKAGE/'patches'/(kind+'.patch')))
    for source,target in operations:
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
    for root in roots.values():(root/'build').mkdir(exist_ok=True)
    print('Sources applied. Continue with docs/INSTALLATION.md; databases and installed game are untouched.')
if __name__=='__main__':main()
