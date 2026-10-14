"""Hardware-independent input learning for the v1.2 tester release.

The runtime consumes a single controller_profile.ini; this module only learns
bindings supported by the existing Maple parser. No SH-4 or plugin edits.
"""
from __future__ import annotations
from controller_tester import PhysicalPad

AXIS_ACTIONS=('joy_x','joy_y','joy2_x','joy2_y')
TRIGGER_ACTIONS=('ltrig','rtrig')
BUTTON_ACTIONS=('a','b','x','y','start','up','down','left','right','c','z')
STEPS=('joy_x','joy_y','joy2_x','joy2_y','ltrig','rtrig')
PROMPTS={
 'joy_x':'Stick izquierdo → DERECHA',
 'joy_y':'Stick izquierdo → ARRIBA',
 'joy2_x':'Stick derecho → DERECHA',
 'joy2_y':'Stick derecho → ARRIBA',
 'ltrig':'Mantené presionado L2 / LT',
 'rtrig':'Mantené presionado R2 / RT',
 'a':'Presioná el botón que usás como A',
 'b':'Presioná el botón que usás como B',
 'x':'Presioná el botón que usás como X',
 'y':'Presioná el botón que usás como Y',
 'start':'Presioná START / OPTIONS',
 'up':'Presioná ARRIBA en el D-pad',
 'down':'Presioná ABAJO en el D-pad',
 'left':'Presioná IZQUIERDA en el D-pad',
 'right':'Presioná DERECHA en el D-pad',
 'c':'Presioná el botón C',
 'z':'Presioná el botón Z',
}


def choices_for_player(player_devices,player):
    entry=player_devices.get(str(player),{})
    default='auto' if player==1 else 'none'
    backend=entry.get('backend',default)
    index=entry.get('index',player-1)
    if backend not in ('auto','xinput','winmm','keyboard_mouse','none') or type(index) is not int or not 0<=index<=15:
        return default,player-1
    return backend,index


def ensure_same_device(neutral,active):
    if neutral is None or active is None:
        raise ValueError('Conectá el mando y seleccioná un dispositivo.')
    if neutral.backend!=active.backend or neutral.index!=active.index:
        raise ValueError('Cambió el dispositivo. Iniciá la calibración de nuevo.')


def strongest_axis(neutral,active,threshold=7500):
    ensure_same_device(neutral,active)
    names=('lx','ly','rx','ry') if active.backend=='xinput' else ('x','y','z','r','u','v')
    changes=[(abs(active.axis(n)-neutral.axis(n)),n,active.axis(n)-neutral.axis(n)) for n in names]
    value,name,delta=max(changes,default=(0,None,0))
    if value<threshold:
        raise ValueError('No se detectó suficiente movimiento. Mantené el control en el extremo.')
    return name,delta


def learn_axis(neutral,active,action):
    if action not in AXIS_ACTIONS:raise ValueError('Acción de eje desconocida')
    axis,delta=strongest_axis(neutral,active)
    # Dreamcast guest expects Y to grow DOWN. The user is instructed to push UP.
    expected=-1 if action.endswith('_y') else 1
    inverted=(delta>0)!=(expected>0)
    return 'axis:'+axis+(':invert' if inverted else '')


def learn_trigger(neutral,active,action):
    if action not in TRIGGER_ACTIONS:raise ValueError('Gatillo desconocido')
    ensure_same_device(neutral,active)
    if active.backend=='xinput':
        amount=(active.lt-neutral.lt) if action=='ltrig' else (active.rt-neutral.rt)
        if amount>=45:return 'trigger:lt' if action=='ltrig' else 'trigger:rt'
    else:
        try:
            axis,delta=strongest_axis(neutral,active,threshold=5000)
            return f'range:{axis}:{neutral.axis(axis)}:{active.axis(axis)}'
        except ValueError:pass
    for i in range(32):
        if active.button(i) and not neutral.button(i):return f'button:{i}'
    raise ValueError('No detecté el gatillo. Probá presionarlo hasta el fondo.')


def learn_button(neutral,active,action):
    if action not in BUTTON_ACTIONS:raise ValueError('Botón desconocido')
    ensure_same_device(neutral,active)
    if active.backend=='winmm' and action in ('up','down','left','right'):
        from controller_tester import pov_active
        if pov_active(active.pov,action) and not pov_active(neutral.pov,action):
            return 'pov:'+action
    for i in range(32):
        if active.button(i) and not neutral.button(i):return f'button:{i}'
    try:
        axis,delta=strongest_axis(neutral,active,threshold=16000)
        return 'axis:'+axis+('+' if delta>0 else '-')
    except ValueError:
        raise ValueError('No se detectó el botón. Mantenelo presionado y pulsá Capturar.')


def learn_binding(neutral,active,action):
    if action in AXIS_ACTIONS:return learn_axis(neutral,active,action)
    if action in TRIGGER_ACTIONS:return learn_trigger(neutral,active,action)
    return learn_button(neutral,active,action)


def neutral_deadzone_percent(pad,minimum=10):
    if pad is None:return minimum
    axes=pad.axes or {}
    # Some WinMM drivers expose triggers at their extreme even at rest.
    # Sample the reliably identifiable left stick (X/Y) instead of those
    # trigger channels; the user can still adjust the deadzone manually.
    keys=('lx','ly','rx','ry') if pad.backend=='xinput' else ('x','y')
    value=max((abs(axes.get(k,0)) for k in keys),default=0)
    return min(30,max(minimum,int(value/32767*100+3.99)))
