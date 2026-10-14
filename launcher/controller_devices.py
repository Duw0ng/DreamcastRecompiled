"""Dreamcast Maple host device choices, independent of the graphical interface.

Matches the cpp36b host spec used by runtime/plugins/dcr_maple.cpp and
portable/launcher/main_windows.go. Never touches the SH-4 guest.
"""
from __future__ import annotations

BACKENDS=('auto','none','keyboard_mouse','xinput','winmm')

def port_spec(backend: str, index: int=0) -> str:
    if backend not in BACKENDS: raise ValueError('Dispositivo no permitido')
    if backend in ('auto','none','keyboard_mouse'):return backend
    if not isinstance(index,int) or not 0 <= index <= (3 if backend=='xinput' else 15):
        raise ValueError('Puerto físico fuera de rango')
    return f'{backend}:{index}'


def port_environment(settings:dict) -> dict[str,str]:
    """Process-only env: A, B, C, D map to players P1...P4.

    Defaults match the original Maple plugin: A=auto, B/C/D=none.
    Avoid exposing port choices as a decorative UI-only setting.
    """
    cf=settings.get('controller_future',{})
    devices=cf.get('player_devices',{}) if isinstance(cf,dict) else {}
    result={}
    for player,letter in enumerate('ABCD',start=1):
        entry=devices.get(str(player),{}) if isinstance(devices,dict) else {}
        backend=entry.get('backend', 'auto' if player==1 else 'none') if isinstance(entry,dict) else ('auto' if player==1 else 'none')
        index=entry.get('index',player-1) if isinstance(entry,dict) else player-1
        try:spec=port_spec(backend,index)
        except ValueError:spec='auto' if player==1 else 'none'
        result[f'DCR_MAPLE_PORT_{letter}_DEVICE']=spec
    return result


def named_choices(xinput_ports=(),winmm_devices=()):
    """List one combined Keyboard+Mouse and only currently detected physical pads.

    Input: connected XInput slot numbers (0-3), and WinMM pairs (index, name).
    Duplicate names are disambiguated by transport/port; some Windows drivers
    expose the *same* physical device through both APIs and cannot be reliably
    deduplicated without comparing PnP IDs.
    """
    out=[('Automático','auto',0),('Desconectado','none',0),('Teclado + Mouse','keyboard_mouse',0)]
    for n in sorted(set(xinput_ports)):
        if isinstance(n,int) and 0<=n<=3:
            out.append((f'Mando {n+1} · XInput','xinput',n))
    for index,name in sorted(winmm_devices):
        if not isinstance(index,int) or not 0<=index<=15:continue
        description=str(name).strip()[:50] or 'Mando'
        out.append((f'{description} · DirectInput {index+1}','winmm',index))
    return out
