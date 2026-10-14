"""Safe stdlib controller_profile.ini editing for stable v1.2 runtime.

Do not invent a second profile format. Preserves the existing v2 entries,
including range:z:min:max trigger calibration from the visual tester.
"""
from __future__ import annotations
from pathlib import Path
import os
import re
import tempfile

CONTROLS = ('a','b','x','y','start','up','down','left','right','c','z',
            'joy_x','joy_y','joy2_x','joy2_y','ltrig','rtrig')
BACKENDS=('auto','xinput','ps4','winmm')
DEFAULTS={k:'logical:'+k for k in CONTROLS}
BINDING_PATTERN=re.compile(r'^(?:logical:[a-z0-9_]+|button:\d{1,2}|pov:(?:up|down|left|right)|trigger:(?:lt|rt)|axis:[a-zA-Z0-9_]+[+-]?(?::invert)?|range:[a-zA-Z0-9_]+:-?\d{1,6}:-?\d{1,6})$',re.IGNORECASE)

def read_profile(path: Path) -> dict:
    info={'backend':'auto','device':0,'deadzone':0.18,'bindings':dict(DEFAULTS)}
    if not path.is_file():return info
    for line in path.read_text(encoding='utf-8-sig',errors='replace').splitlines():
        if '=' not in line or line.lstrip().startswith(('#',';','[')):continue
        name,value=(s.strip() for s in line.split('=',1))
        name=name.lower()
        if name=='backend' and value in BACKENDS:info['backend']=value
        elif name=='device':
            try:info['device']=max(0,min(15,int(value)))
            except ValueError:pass
        elif name=='deadzone':
            try:info['deadzone']=max(0.,min(.5,float(value.replace(',','.'))))
            except ValueError:pass
        elif name in CONTROLS and BINDING_PATTERN.fullmatch(value):info['bindings'][name]=value
    return info

def write_profile(path: Path,info:dict):
    backend=info.get('backend','auto')
    if backend not in BACKENDS:raise ValueError('Backend de mando inválido')
    device=int(info.get('device',0))
    deadzone=float(info.get('deadzone',0.18))
    if not 0<=device<=15 or not 0<=deadzone<=.5:raise ValueError('Puerto o zona muerta inválidos')
    bindings=info.get('bindings',{})
    lines=['[DreamcastRecompController]','version=2',f'backend={backend}',
           f'device={device}',f'deadzone={deadzone:.2f}']
    for key in CONTROLS:
        v=bindings.get(key,DEFAULTS[key])
        if not BINDING_PATTERN.fullmatch(v):raise ValueError('Asignación inválida para '+key)
        lines.append(f'{key}={v}')
    path.parent.mkdir(parents=True,exist_ok=True)
    # Preserve calibrated trigger data and previous mappings in a recoverable
    # backup if the user manually saves a new configuration.
    if path.exists():
        import shutil
        shutil.copy2(path,path.with_suffix('.ini.bak'))
    fd,temp=tempfile.mkstemp(prefix='controller-',suffix='.tmp',dir=path.parent)
    try:
        with os.fdopen(fd,'w',encoding='ascii') as fp:
            fp.write('\n'.join(lines)+'\n')
        os.replace(temp,path)
    finally:
        if os.path.exists(temp):os.unlink(temp)
    return path
