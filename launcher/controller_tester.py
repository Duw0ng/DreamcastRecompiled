"""Pure controller-tester mapping model for the Dreamcast Recompiled launcher.

Does not inject inputs, modify Maple, or require a graphical desktop.
The profile and raw bindings follow the bundled controller_profile.ini / visual tester.
Cartesian Y is positive *up* for the two XY test gauges.
"""
from __future__ import annotations

from dataclasses import dataclass


XINPUT_MASKS=(0x1000,0x2000,0x4000,0x8000,0x0100,0x0200,0x0020,
              0x0010,0x0040,0x0080,0x0001,0x0002,0x0004,0x0008)
XINPUT_LOGICAL={
    'a':0,'b':1,'x':2,'y':3,'c':4,'z':5,'start':7,
    'up':10,'down':11,'left':12,'right':13,
}
WINMM_LOGICAL={'x':0,'a':1,'b':2,'y':3,'c':4,'z':5,'start':9}
AXIS_ACTIONS=('joy_x','joy_y','joy2_x','joy2_y')
BUTTON_ACTIONS=('a','b','x','y','start','up','down','left','right','c','z')
TRIGGER_ACTIONS=('ltrig','rtrig')
DISPLAY_NAMES={
    'a':'A','b':'B','x':'X','y':'Y','start':'START','up':'ARRIBA',
    'down':'ABAJO','left':'IZQUIERDA','right':'DERECHA',
    'c':'C','z':'Z','ltrig':'L','rtrig':'R',
    'joy_x':'STICK I X','joy_y':'STICK I Y',
    'joy2_x':'STICK D X','joy2_y':'STICK D Y',
}


def clamp(value,lo=-1.0,hi=1.0):
    return max(lo,min(hi,value))


def pov_active(pov:int,direction:str)->bool:
    if pov in (0xFFFF,0xFFFFFFFF,-1):return False
    angle=int(pov)%36000
    if direction=='up':return angle>=31500 or angle<=4500
    if direction=='right':return 4500<=angle<=13500
    if direction=='down':return 13500<=angle<=22500
    if direction=='left':return 22500<=angle<=31500
    return False


@dataclass
class PhysicalPad:
    backend:str  # 'xinput' or 'winmm'
    index:int
    buttons:int=0
    pov:int=0xFFFF
    axes:dict|None=None  # raw signed axes, [-32768,32767] range
    lt:int=0
    rt:int=0

    def axis(self,name):
        return int((self.axes or {}).get(name.lower(),0))

    def button(self,index:int):
        if not 0<=index<=31:return False
        if self.backend=='xinput':
            return index<len(XINPUT_MASKS) and bool(self.buttons & XINPUT_MASKS[index])
        return bool(self.buttons & (1<<index))


def logical_button(pad:PhysicalPad,name:str)->bool:
    key=name.lower()
    if pad.backend=='xinput':
        index=XINPUT_LOGICAL.get(key)
        return bool(index is not None and pad.button(index))
    index=WINMM_LOGICAL.get(key)
    if index is not None:return pad.button(index)
    if key in ('up','down','left','right'):return pov_active(pad.pov,key)
    return False


def logical_axis(pad:PhysicalPad,name:str)->int:
    k=name.lower()
    if pad.backend=='xinput':
        keys={'joy_x':'lx','joy_y':'ly','joy2_x':'rx','joy2_y':'ry'}
        raw=pad.axis(keys.get(k,''))
        return -raw if k.endswith('_y') else raw
    keys={'joy_x':'x','joy_y':'y','joy2_x':'z','joy2_y':'r'}
    return pad.axis(keys.get(k,''))


def logical_trigger(pad:PhysicalPad,name:str)->int:
    if pad.backend=='xinput':return pad.lt if name=='ltrig' else pad.rt
    return 255 if pad.button(6 if name=='ltrig' else 7) else 0


def binding_bool(pad:PhysicalPad,binding:str,deadzone=.18)->bool:
    b=(binding or '').lower()
    if b.startswith('logical:'):return logical_button(pad,b[8:])
    if b.startswith('button:'):
        try:return pad.button(int(b[7:]))
        except ValueError:return False
    if b in ('trigger:lt','trigger:rt'):
        return (pad.lt if b=='trigger:lt' else pad.rt)>80
    if b.startswith('pov:'):return pov_active(pad.pov,b[4:])
    if b.startswith('axis:'):
        axis=b[5:].split(':')[0]
        sign=axis[-1:] if axis.endswith(('+','-')) else ''
        raw=pad.axis(axis[:-1] if sign else axis)
        threshold=int(32767*deadzone)
        if sign=='+':return raw>threshold
        if sign=='-':return raw< -threshold
        return abs(raw)>threshold
    return False


def binding_axis(pad:PhysicalPad,binding:str,deadzone=.18)->int:
    b=(binding or '').lower()
    if b.startswith('logical:'):raw=logical_axis(pad,b[8:])
    elif b.startswith('axis:'):
        axis=b[5:].split(':')[0]
        invert=(':invert' in b)
        if axis.endswith(('+','-')):
            invert ^= axis[-1]=='-'
            axis=axis[:-1]
        raw=pad.axis(axis)
        if invert:raw=-raw
    else:return 0
    return 0 if abs(raw)<=32767*deadzone else int(clamp(raw,-32767,32767))


def binding_trigger(pad:PhysicalPad,binding:str,name:str,deadzone=.18)->int:
    b=(binding or '').lower()
    if b.startswith('logical:'):return logical_trigger(pad,b[8:])
    if b=='trigger:lt':return pad.lt
    if b=='trigger:rt':return pad.rt
    if b.startswith('range:'):
        parts=b.split(':')
        if len(parts)==4:
            try:
                raw=pad.axis(parts[1]);rest=int(parts[2]);full=int(parts[3])
                return int(round(clamp((raw-rest)/(full-rest),0,1)*255)) if full!=rest else 0
            except ValueError:return 0
    if b.startswith('axis:'):return int(min(255,abs(binding_axis(pad,b,deadzone))/128))
    return 255 if binding_bool(pad,b,deadzone) else 0


def mapped_state(pad:PhysicalPad,profile:dict)->dict:
    b=profile.get('bindings',{})
    deadzone=float(profile.get('deadzone',.18))
    buttons={name:binding_bool(pad,b.get(name,'logical:'+name),deadzone)
             for name in BUTTON_ACTIONS}
    axes={name:binding_axis(pad,b.get(name,'logical:'+name),deadzone)
          for name in AXIS_ACTIONS}
    triggers={name:binding_trigger(pad,b.get(name,'logical:'+name),name,deadzone)
              for name in TRIGGER_ACTIONS}
    # Physical mappings use positive DOWN as the guest convention. The
    # tester displays Cartesian UP as positive; flip only for visualization.
    left=(clamp(axes['joy_x']/32767),clamp(-axes['joy_y']/32767))
    right=(clamp(axes['joy2_x']/32767),clamp(-axes['joy2_y']/32767))
    return {'buttons':buttons,'sticks':(left,right),
            'triggers':(triggers['ltrig']/255.0,triggers['rtrig']/255.0),
            'bindings':b,'backend':pad.backend,'device':pad.index}


def binding_name(binding:str)->str:
    b=(binding or '').lower()
    if b.startswith('logical:'):
        return DISPLAY_NAMES.get(b[8:],b[8:].upper())+' (lógico)'
    if b.startswith('button:'):
        try:return f'Botón físico {int(b[7:])+1}'
        except ValueError:return b
    if b.startswith('pov:'):return 'Cruceta '+DISPLAY_NAMES.get(b[4:],b[4:].upper())
    if b.startswith('trigger:'):return 'Gatillo físico '+b[8:].upper()
    if b.startswith('axis:'):
        return 'Eje '+b[5:].upper()
    if b.startswith('range:'):
        return 'Rango '+b[6:].upper().replace(':',' / ')
    return binding or 'Sin asignar'


def binding_summary(profile:dict,keys=('a','b','x','y','start','ltrig','rtrig')):
    b=profile.get('bindings',{})
    return '\n'.join(f"{DISPLAY_NAMES.get(k,k.upper())}: {binding_name(b.get(k,'logical:'+k))}" for k in keys)
