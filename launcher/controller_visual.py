"""Single-window controller monitor. Only Tkinter and Win32 system DLLs at runtime.

Uses the bundled full-color Dreamcast controller render; no guest changes or
controller input injection. Device selection is strict: P2 doesn't silently
show the input from P1. Mapping corresponds to the actual controller INI.
"""
from __future__ import annotations
import ctypes
from pathlib import Path
import sys
import tkinter as tk
from tkinter import ttk, messagebox

from controller_profiles import read_profile
from controller_tester import PhysicalPad, mapped_state, binding_name
from controller_devices import named_choices
from controller_calibration import (choices_for_player, PROMPTS, STEPS,
                                    learn_binding, neutral_deadzone_percent)

BG='#d9efff'; DARK='#16416e'; MUTE='#4776a2'
SKY='#c7e8ff'; BLUE='#367eb1'


class ControllerPreview:
    def __init__(self,page):
        self.page=page
        self.alive=True
        self.after_id=None
        self.xinput=None
        self.winmm=None
        self.user32=None
        self._init_win32()
        self.profile_file=Path(__file__).resolve().parent.parent/'profiles'/'controller_profile.ini'
        self.profile=read_profile(self.profile_file)
        self._profile_mtime=None
        self._pad_image=None
        self.origin=(0,0)
        self.scale_x=320/1122
        self.scale_y=365/1280
        self._last_signature=None
        self._device_options={}
        self._neutral=None
        self._draft={}
        self._calibrating=False
        settings=page.app.settings.get('controller_future',{})
        self.player=tk.IntVar(value=max(1,min(4,int(settings.get('player_index',1)))))
        self.device_label=tk.StringVar()
        self._action_labels={
            'joy_x':'Stick izq. X','joy_y':'Stick izq. Y',
            'joy2_x':'Stick der. X','joy2_y':'Stick der. Y',
            'ltrig':'Gatillo L','rtrig':'Gatillo R',
            'a':'Botón A','b':'Botón B','x':'Botón X','y':'Botón Y',
            'start':'START','up':'D-pad Arriba','down':'D-pad Abajo',
            'left':'D-pad Izq.','right':'D-pad Der.','c':'Botón C','z':'Botón Z',
        }
        self._label_actions={v:k for k,v in self._action_labels.items()}
        self.action=tk.StringVar(value=self._action_labels[STEPS[0]])
        self.dz=tk.IntVar(value=int(settings.get('deadzone_pct',15)))
        page.win.bind('<Destroy>',self._destroy,add='+')

    def _init_win32(self):
        if sys.platform!='win32':return
        for name in ('xinput1_4.dll','xinput1_3.dll','xinput9_1_0.dll'):
            try:
                dll=ctypes.WinDLL(name)
                dll.XInputGetState.argtypes=[ctypes.c_uint,ctypes.c_void_p]
                dll.XInputGetState.restype=ctypes.c_uint
                self.xinput=dll
                break
            except (OSError,AttributeError):pass
        try:
            self.winmm=ctypes.WinDLL('winmm.dll')
            self.winmm.joyGetPosEx.argtypes=[ctypes.c_uint,ctypes.c_void_p]
            self.winmm.joyGetPosEx.restype=ctypes.c_uint
        except (OSError,AttributeError):pass
        try:
            self.user32=ctypes.WinDLL('user32.dll')
            self.user32.GetAsyncKeyState.argtypes=[ctypes.c_int]
            self.user32.GetAsyncKeyState.restype=ctypes.c_short
        except (OSError,AttributeError):pass

    def _destroy(self,event):
        if event.widget!=self.page.win:return
        self.alive=False
        if self.after_id is not None:
            try:self.page.win.after_cancel(self.after_id)
            except tk.TclError:pass

    def _select_player(self):
        player=self.player.get()
        self.page.app.settings.setdefault('controller_future',{})['player_index']=player
        self.page.app.persist()
        self._last_signature=None
        self._calibrating=False
        self._neutral=None
        self._draft={}
        self._rebuild_device_choices()
        self._reload_profile(force=True)
        self._render(None)
        self.page._show_status(f'P{player} seleccionado')

    def mount(self):
        r=self.page.row()
        tk.Label(r,text='Jugador',bg=BG,fg=DARK,font=('Segoe UI',11,'bold')).pack(side='left')
        for i in range(1,5):
            tk.Radiobutton(r,text=f'P{i}',value=i,variable=self.player,command=self._select_player,
                           bg=BG,fg=DARK,selectcolor='#f2faff',activebackground=BG,
                           highlightthickness=0,font=('Segoe UI',10,'bold')).pack(side='left',padx=(7,2))
        self.status=tk.Label(r,text='Buscando mando…',bg=BG,fg=DARK,font=('Segoe UI',10,'bold'))
        self.status.pack(side='left',padx=(12,0))

        dev=self.page.row()
        tk.Label(dev,text='Dispositivo',bg=BG,fg=DARK,font=('Segoe UI',10,'bold')).pack(side='left')
        self.device_combo=ttk.Combobox(dev,textvariable=self.device_label,width=43,state='readonly')
        self.device_combo.pack(side='left',padx=(12,8))
        self.device_combo.bind('<<ComboboxSelected>>',self._change_device)
        tk.Button(dev,text='Buscar mandos',command=self._rebuild_device_choices,bg=SKY,fg=DARK,
                  bd=0,relief='flat',padx=9,pady=4).pack(side='left')
        self.device_hint=tk.Label(self.page.body,text='',bg=BG,fg=MUTE,anchor='w',font=('Segoe UI',9))
        self.device_hint.pack(fill='x',padx=23,pady=(0,5))
        self._rebuild_device_choices()
        self._mount_calibration()

        host=tk.Frame(self.page.body,bg=BG)
        host.pack(fill='x',padx=19,pady=(0,10))
        self.panel=tk.Canvas(host,width=500,height=420,bg=SKY,
                             bd=0,highlightthickness=1,highlightbackground='#8bc4e5')
        self.panel.grid(row=0,column=0,sticky='nw')
        right=tk.Frame(host,bg=BG)
        if self.page.app.scene.width<=800:
            # 800-pixel launcher: stack test gauges below instead of cropping.
            right.grid(row=1,column=0,sticky='nw',padx=0,pady=(10,0))
        else:
            right.grid(row=0,column=1,sticky='nw',padx=(12,0))
        self.sticks=tk.Canvas(right,width=288,height=212,bg=SKY,bd=0,highlightthickness=1,highlightbackground='#8bc4e5')
        self.sticks.pack()
        self.triggers=tk.Canvas(right,width=288,height=88,bg=SKY,bd=0,highlightthickness=1,highlightbackground='#8bc4e5')
        self.triggers.pack(pady=(8,0))
        self.mapped_label=tk.Label(right,bg=BG,fg=DARK,text='',font=('Consolas',8),justify='left',anchor='nw')
        self.mapped_label.pack(fill='x',pady=(8,2))
        self.active_label=tk.Label(right,bg=BG,fg=MUTE,text='',font=('Segoe UI',9),justify='left',anchor='nw',wraplength=283)
        self.active_label.pack(fill='x')
        self._init_picture()
        self._reload_profile(force=True)
        self._render(None)
        self._poll()

    def _init_picture(self):
        c=self.panel
        c.create_text(12,18,text='Dreamcast · Mando en vivo',anchor='w',fill=DARK,font=('Segoe UI',11,'bold'))
        try:
            self._pad_image=tk.PhotoImage(file=str(Path(__file__).resolve().parent/'assets'/'controller_v861_preview.png'))
            ox=(500-self._pad_image.width())//2
            oy=46
            self.origin=(ox,oy)
            self.scale_x=self._pad_image.width()/1122
            self.scale_y=self._pad_image.height()/1280
            c.create_image(ox,oy,anchor='nw',image=self._pad_image,tags='base')
        except tk.TclError:
            c.create_text(250,230,text='No se pudo cargar el recurso Dreamcast',fill=MUTE,tags='base')
        # Physical L/R shoulders aren't visible in a frontal Dreamcast photo.
        # Separate glowing indicators make them legible without falsifying the image.
        for key,cx,title in [('l',56,'L'),('r',445,'R')]:
            c.create_oval(cx-27,53,cx+27,95,fill='#a7cbe8',outline='#6c9fc9',width=2,tags='base')
            c.create_text(cx,74,text=title,fill='#42678e',font=('Segoe UI',18,'bold'),tags='base')

    def _reload_profile(self,force=False):
        try:current=self.profile_file.stat().st_mtime_ns
        except OSError:current=None
        if not force and current==self._profile_mtime:return
        self._profile_mtime=current
        self.profile=read_profile(self.profile_file)
        self.mapped_label.configure(text='Perfil cargado: '+self.profile.get('backend','auto').upper())
        self._last_signature=None

    @staticmethod
    def _signed(raw):
        return max(-32767,min(32767,int(raw)-32767))

    def _xinput_state(self,port):
        if not self.xinput:return None
        class Gamepad(ctypes.Structure):
            _fields_=[('buttons',ctypes.c_ushort),('lt',ctypes.c_ubyte),('rt',ctypes.c_ubyte),
                      ('lx',ctypes.c_short),('ly',ctypes.c_short),('rx',ctypes.c_short),('ry',ctypes.c_short)]
        class State(ctypes.Structure):
            _fields_=[('packet',ctypes.c_uint32),('pad',Gamepad)]
        state=State()
        if self.xinput.XInputGetState(port,ctypes.byref(state))!=0:return None
        g=state.pad
        return PhysicalPad('xinput',port,g.buttons,0xFFFF,
                           {'lx':g.lx,'ly':g.ly,'rx':g.rx,'ry':g.ry},g.lt,g.rt)

    def _winmm_state(self,port):
        if not self.winmm:return None
        class JoyInfoEx(ctypes.Structure):
            _fields_=[('size',ctypes.c_uint32),('flags',ctypes.c_uint32),
                      ('x',ctypes.c_uint32),('y',ctypes.c_uint32),('z',ctypes.c_uint32),
                      ('r',ctypes.c_uint32),('u',ctypes.c_uint32),('v',ctypes.c_uint32),
                      ('buttons',ctypes.c_uint32),('number',ctypes.c_uint32),
                      ('pov',ctypes.c_uint32),('reserved1',ctypes.c_uint32),('reserved2',ctypes.c_uint32)]
        j=JoyInfoEx();j.size=ctypes.sizeof(j);j.flags=0xFF
        if self.winmm.joyGetPosEx(port,ctypes.byref(j))!=0:return None
        return PhysicalPad('winmm',port,j.buttons,j.pov,
                           {'x':self._signed(j.x),'y':self._signed(j.y),'z':self._signed(j.z),
                            'r':self._signed(j.r),'u':self._signed(j.u),'v':self._signed(j.v)})

    def _device_for_player(self):
        devices=(self.page.app.settings.get('controller_future',{}).get('player_devices',{})
                 if getattr(self,'page',None) is not None else {})
        return choices_for_player(devices,self.player.get())

    def _detect(self):
        backend,port=self._device_for_player()
        if backend in ('none','keyboard_mouse'):return None  # virtual / disconnected
        if backend=='winmm':return self._winmm_state(port)
        if backend=='xinput':
            return self._xinput_state(port) if port<4 else None
        # Auto probes only the selected index, never another player's port.
        return (self._xinput_state(port) if port<4 else None) or self._winmm_state(port)

    def _winmm_name(self,index):
        if not self.winmm or sys.platform!='win32':return f'Mando {index+1}'
        # JOYCAPSW is a fixed-layout native structure, char[32] name at offset 4.
        # Allocate 2048 bytes to accommodate newer Windows fields safely.
        buf=ctypes.create_string_buffer(2048)
        try:
            func=self.winmm.joyGetDevCapsW
            func.argtypes=[ctypes.c_size_t,ctypes.c_void_p,ctypes.c_uint]
            func.restype=ctypes.c_uint
            if func(index,buf,2048)==0:
                name=ctypes.wstring_at(ctypes.addressof(buf)+4,32).split('\x00',1)[0].strip()
                return name or f'Mando {index+1}'
        except (OSError,ValueError,AttributeError):pass
        return f'Mando {index+1}'

    def _rebuild_device_choices(self):
        # Old cpp36b UX: one keyboard+mouse, only detected physical pads.

        # Preserve a disconnected saved selection visibly for reconnection. No
        # list with 16 fake non-connected WinMM slots by default.
        # (displayed choices are hardware-scanned)
        current=self._device_for_player()
        xin=[i for i in range(4) if self._xinput_state(i) is not None]
        win=[(i,self._winmm_name(i)) for i in range(16) if self._winmm_state(i) is not None]
        options={label:(kind,index) for label,kind,index in named_choices(xin,win)}
        if current not in options.values():
            backend,index=current
            label=(f'Mando no conectado · {backend.upper()} {index+1}')
            options[label]=current
        self._device_options=options
        self.device_combo.configure(values=list(options))
        for label,value in options.items():
            if value==current:
                self.device_label.set(label)
                break
        return options

    def _set_device(self,backend,index):
        self.page.app.settings.setdefault('controller_future',{}).setdefault('player_devices',{})[str(self.player.get())]={
            'backend':backend,'index':index}
        self.page.app.persist()
        self._calibrating=False
        self._neutral=None
        self._draft={}
        self._last_signature=None

    def _change_device(self,_event=None):
        item=self._device_options.get(self.device_label.get())
        if item:
            self._set_device(*item)
            self._reload_profile(force=True)
            self.page._show_status('Dispositivo seleccionado')

    def _active_profile(self):
        from copy import deepcopy
        profile=deepcopy(self.profile)
        player=str(self.player.get())
        settings=self.page.app.settings.get('controller_future',{})
        entry=settings.get('calibrations',{}).get(player,{})
        selected_backend,selected_index=self._device_for_player()
        actual_pad=self._detect() if selected_backend=='auto' else None
        actual_backend=actual_pad.backend if actual_pad else selected_backend
        actual_index=actual_pad.index if actual_pad else selected_index
        from controller_profiles import DEFAULTS
        source_backend=profile.get('backend','auto')
        is_matching_backend=(source_backend in ('auto',actual_backend) or
                            (source_backend=='ps4' and actual_backend=='winmm'))
        if not is_matching_backend or (source_backend!='auto' and profile.get('device',0)!=actual_index):
            # Another player's physical pad must not inherit an unrelated
            # globally active profile's raw bindings.
            profile['bindings']=dict(DEFAULTS)
        if entry.get('backend',actual_backend)==actual_backend and entry.get('index',actual_index)==actual_index:
            profile['bindings'].update(entry.get('bindings',{}))
            profile['deadzone']=entry.get('deadzone_pct',int(profile['deadzone']*100))/100.0
        if self._calibrating:
            profile['bindings'].update(self._draft)
        return profile

    def _mount_calibration(self):
        row=self.page.row()
        tk.Label(row,text='Calibración',bg=BG,fg=DARK,font=('Segoe UI',11,'bold')).pack(side='left')
        self._button(row,'Iniciar',self._begin_calibration)
        tk.Label(row,text='Zona muerta',bg=BG,fg=DARK).pack(side='left',padx=(8,4))
        ttk.Spinbox(row,from_=0,to=50,width=4,textvariable=self.dz).pack(side='left')
        self._button(row,'Guardar jugador',self._save_calibration)
        self._button(row,'Aplicar al juego',self._apply_to_game)
        row2=self.page.row()
        self.action_combo=ttk.Combobox(row2,width=22,state='readonly',textvariable=self.action,
                                      values=list(self._label_actions))
        self.action_combo.pack(side='left',padx=(0,8))
        self._button(row2,'Capturar',self._capture_calibration)
        self._button(row2,'Omitir',self._skip_calibration)
        self._button(row2,'Restablecer',self._reset_calibration)
        self.calib_label=tk.Label(self.page.body,text='',bg=BG,fg=MUTE,anchor='w',font=('Segoe UI',10))
        self.calib_label.pack(fill='x',padx=23,pady=(0,6))
        self._show_calibration_prompt('Iniciá para calibrar')

    @staticmethod
    def _button(frame,label,command):
        b=tk.Button(frame,text=label,command=command,bd=0,bg='#b3dffc',fg=DARK,
                    padx=9,pady=5,relief='flat',cursor='hand2',font=('Segoe UI',9,'bold'))
        b.pack(side='left',padx=(0,6))
        return b

    def _show_calibration_prompt(self,status=''):
        action=self._label_actions.get(self.action.get(),self.action.get())
        detail=PROMPTS.get(action,action)
        self.calib_label.configure(text=(status+'  ·  ' if status else '')+detail)

    def _begin_calibration(self):
        backend,_=self._device_for_player()
        if backend in ('keyboard_mouse','none'):
            self._show_calibration_prompt('Teclado + Mouse usa controles directos; la calibración es para mandos físicos')
            return
        pad=self._detect()
        if not pad:
            self._show_calibration_prompt('Conectá el dispositivo seleccionado')
            return
        self._neutral=pad
        self._calibrating=True
        self._draft={}
        self.dz.set(neutral_deadzone_percent(pad,minimum=10))
        self.action.set(self._action_labels[STEPS[0]])
        self._show_calibration_prompt('Centro registrado')

    def _capture_calibration(self):
        if not self._calibrating or self._neutral is None:
            self._begin_calibration()
            return
        action=self._label_actions.get(self.action.get(),self.action.get())
        try:
            binding=learn_binding(self._neutral,self._detect(),action)
        except ValueError as exc:
            self._show_calibration_prompt(str(exc))
            return
        self._draft[action]=binding
        self._last_signature=None
        if action in STEPS:
            at=STEPS.index(action)
            if at+1<len(STEPS):self.action.set(self._action_labels[STEPS[at+1]])
        self._show_calibration_prompt(action.upper()+' = '+binding)

    def _skip_calibration(self):
        action=self._label_actions.get(self.action.get(),self.action.get())
        if action in STEPS:
            at=STEPS.index(action)
            if at+1<len(STEPS):self.action.set(self._action_labels[STEPS[at+1]])
        self._show_calibration_prompt('Omitido')

    def _save_calibration(self):
        if self._device_for_player()[0] in ('keyboard_mouse','none'):
            self._show_calibration_prompt('Elegí un mando físico para calibrarlo')
            return
        player=str(self.player.get())
        settings=self.page.app.settings.setdefault('controller_future',{})
        entries=settings.setdefault('calibrations',{})
        entry=entries.get(player,{'bindings':{}})
        backend,index=self._device_for_player()
        physical=self._detect() if backend=='auto' else None
        if physical is not None:backend,index=physical.backend,physical.index
        # Never carry bindings from a previously selected physical device.
        bindings=(dict(entry.get('bindings',{}))
                  if entry.get('backend',backend)==backend and entry.get('index',index)==index
                  else {})
        bindings.update(self._draft)
        try:deadzone=int(self.dz.get())
        except (ValueError,tk.TclError):deadzone=15
        deadzone=max(0,min(50,deadzone))
        entries[player]={'bindings':bindings,'deadzone_pct':deadzone,'backend':backend,'index':index}
        self.page.app.persist()
        self._calibrating=False
        self._draft={}
        self._last_signature=None
        self._reload_profile(force=True)
        self._show_calibration_prompt(f'P{player}: calibración guardada')

    def _apply_to_game(self):
        if self._device_for_player()[0] in ('keyboard_mouse','none'):
            self._show_calibration_prompt('Teclado + Mouse ya se aplica al iniciar el juego; no necesita perfil INI')
            return
        # Runtime currently accepts only ONE active global profile (Maple ABI).
        # Preserve other player settings in the launcher; never fake simultaneous mapping.
        self._save_calibration()
        pad=self._detect()
        backend,index=self._device_for_player()
        if backend=='auto':
            if pad is None:
                self._show_calibration_prompt('Elegí un backend o conectá el mando antes de aplicar')
                return
            backend,index=pad.backend,pad.index
        from controller_profiles import write_profile
        active=self._active_profile()
        active['backend']=backend
        active['device']=index
        try:write_profile(self.profile_file,active)
        except (OSError,ValueError) as exc:
            self._show_calibration_prompt('No se pudo guardar: '+str(exc))
            return
        self._reload_profile(force=True)
        self._show_calibration_prompt(f'P{self.player.get()} aplicado al perfil activo del juego')

    def _reset_calibration(self):
        player=str(self.player.get())
        future=self.page.app.settings.setdefault('controller_future',{})
        future.setdefault('calibrations',{}).pop(player,None)
        self.page.app.persist()
        self._calibrating=False
        self._draft={}
        self._neutral=None
        self.dz.set(int(self.profile.get('deadzone',.18)*100))
        self._last_signature=None
        self._show_calibration_prompt('Restaurado')

    def _point(self,x,y):
        ox,oy=self.origin
        return ox+x*self.scale_x,oy+y*self.scale_y

    def _ring(self,x,y,rx,ry,color):
        px,py=self._point(x,y)
        # Only an outline so original painted buttons are preserved.
        self.panel.create_oval(px-rx*self.scale_x,py-ry*self.scale_y,
                               px+rx*self.scale_x,py+ry*self.scale_y,
                               outline=color,width=4,tags='state')

    def _render_pad(self,mapped):
        self.panel.delete('state')
        if mapped is None:
            return
        b=mapped['buttons'];lt,rt=mapped['triggers']
        # The photographed controller has one physical stick. Right stick is a
        # modern host capability, shown as a separate X/Y gauge on the right.
        left=mapped['sticks'][0]
        x,y=self._point(154,400)
        x+=left[0]*24*self.scale_x*3
        y-=left[1]*24*self.scale_y*3
        self.panel.create_oval(x-6,y-6,x+6,y+6,fill='#fb9c55',outline='white',width=2,tags='state')
        positions={'a':(915,610,53,52,'#ff6264'),
                   'b':(1012,525,53,52,'#53aeff'),
                   'x':(813,507,53,52,'#ffdc51'),
                   'y':(918,428,53,52,'#75e6af'),
                   'start':(560,873,65,60,'#d0eaff'),
                   'up':(216,588,45,45,'#78ddff'),
                   'down':(216,745,45,45,'#78ddff'),
                   'left':(125,666,45,45,'#78ddff'),
                   'right':(303,666,45,45,'#78ddff')}
        for key,(px,py,rx,ry,color) in positions.items():
            if b.get(key):self._ring(px,py,rx,ry,color)
        for cx,title,val in ((56,'L',lt),(445,'R',rt)):
            if val>0.04:
                halo='#fbea83' if val>.5 else '#a8f5dd'
                self.panel.create_oval(cx-29,51,cx+29,97,outline=halo,width=5,tags='state')
                self.panel.create_text(cx,74,text=title,fill='#ffac20',font=('Segoe UI',18,'bold'),tags='state')

    def _render_sticks(self,sticks):
        c=self.sticks;c.delete('dynamic')
        if not c.find_withtag('base'):
            c.create_text(12,17,text='Analógicos X / Y',anchor='w',fill=DARK,font=('Segoe UI',11,'bold'),tags='base')
            for cx,label in ((78,'Izquierdo'),(212,'Derecho')):
                cy=106;r=41
                c.create_text(cx,42,text=label,fill=DARK,font=('Segoe UI',10,'bold'),tags='base')
                c.create_oval(cx-r,cy-r,cx+r,cy+r,outline=BLUE,width=2,tags='base')
                c.create_line(cx-r,cy,cx+r,cy,fill='#7faad0',tags='base')
                c.create_line(cx,cy-r,cx,cy+r,fill='#7faad0',tags='base')
        for cx,(sx,sy) in ((78,sticks[0]),(212,sticks[1])):
            cy=106
            dx=cx+sx*35;dy=cy-sy*35
            c.create_oval(dx-7,dy-7,dx+7,dy+7,fill='#f98a43',outline='white',width=2,tags='dynamic')
            c.create_text(cx,174,text=f'X {sx:+.2f}',fill=DARK,font=('Consolas',10),tags='dynamic')
            c.create_text(cx,191,text=f'Y {sy:+.2f}',fill=DARK,font=('Consolas',10),tags='dynamic')

    def _render_triggers(self,triggers):
        c=self.triggers;c.delete('dynamic')
        if not c.find_withtag('base'):
            c.create_text(12,14,text='Gatillos L / R',anchor='w',fill=DARK,font=('Segoe UI',10,'bold'),tags='base')
            for cy,label in ((37,'L'),(70,'R')):
                c.create_text(12,cy,text=label,anchor='w',fill=DARK,font=('Segoe UI',10,'bold'),tags='base')
                c.create_rectangle(46,cy-10,242,cy+10,outline='#80a6c7',fill='#e8f7ff',tags='base')
        for cy,v,color in ((37,triggers[0],'#7ce5ad'),(70,triggers[1],'#ffb363')):
            c.create_rectangle(47,cy-9,47+max(0,min(1,v))*194,cy+9,outline='',fill=color,tags='dynamic')
            c.create_text(276,cy,text=f'{v*100:3.0f}%',anchor='e',fill=DARK,font=('Consolas',9),tags='dynamic')

    def _keyboard_mouse_state(self):
        # Tester view of the keyboard+mouse virtual input already used by cpp36b.
        # No fake PhysicalPad, no hardware calibration.
        def pressed(vk):
            if self.user32 is None:return False
            return bool(self.user32.GetAsyncKeyState(vk) & 0x8000)
        buttons={
            'a':any(pressed(k) for k in (ord('J'),ord('Z'),32,0x01)),
            'b':any(pressed(k) for k in (ord('K'),ord('X'),0x02)),
            'x':any(pressed(k) for k in (ord('U'),ord('C'),0x04)),
            'y':any(pressed(k) for k in (ord('I'),ord('V'),0x05)),
            'start':pressed(0x0D) or pressed(0x06),
            'up':pressed(0x26),'down':pressed(0x28),
            'left':pressed(0x25),'right':pressed(0x27),
        }
        left=(float(pressed(ord('D')))-float(pressed(ord('A'))),
              float(pressed(ord('W')))-float(pressed(ord('S'))))
        # Dreamcast hardware only has a single stick. The right gauge is a
        # host convenience, so keyboard+mouse leaves it centered.
        return {'buttons':buttons,'sticks':(left,(0,0)),
                'triggers':(float(pressed(ord('Q'))),float(pressed(ord('E'))))}

    def _render(self,pad):
        backend,_=self._device_for_player()
        if backend=='keyboard_mouse':
            mapped=self._keyboard_mouse_state()
            self.status.configure(text=f'Teclado + Mouse · P{self.player.get()}')
            self.device_hint.configure(text='WASD: stick · Flechas: cruceta · J/K/U/I: botones · Mouse: clics · Q/E: L/R')
        else:
            mapped=mapped_state(pad,self._active_profile()) if pad else None
            if mapped:
                self.status.configure(text=f"{pad.backend.upper()} · P{self.player.get()}")
                self.device_hint.configure(text='')
            else:
                self.status.configure(text=f'P{self.player.get()} ' + ('desconectado' if backend=='none' else 'sin mando conectado'))
                self.device_hint.configure(text='')
        active=[key.upper() for key,on in mapped['buttons'].items() if on] if mapped else []
        if mapped:active.extend(key for key,val in zip(('L','R'),mapped['triggers']) if val>.1)
        self.active_label.configure(text='Activos: '+(', '.join(active) if active else '—'))
        self._render_pad(mapped)
        self._render_sticks(mapped['sticks'] if mapped else ((0,0),(0,0)))
        self._render_triggers(mapped['triggers'] if mapped else (0,0))

    def _poll(self):
        if not self.alive or not self.page.win.winfo_exists():return
        self._reload_profile()
        backend,_=self._device_for_player()
        pad=self._detect()
        virtual=self._keyboard_mouse_state() if backend=='keyboard_mouse' else None
        signature=(self.player.get(),self._profile_mtime,self._device_for_player(),
                   repr(self.page.app.settings.get('controller_future',{}).get('calibrations',{}).get(str(self.player.get()))),
                   pad.backend if pad else None,pad.index if pad else None,
                   pad.buttons if pad else None,pad.pov if pad else None,
                   tuple(sorted((pad.axes or {}).items())) if pad else None,
                   pad.lt if pad else None,pad.rt if pad else None,
                   tuple(sorted(virtual['buttons'].items())) if virtual else None,
                   tuple(virtual['sticks'][0]) if virtual else None,
                   tuple(virtual['triggers']) if virtual else None)
        if signature!=self._last_signature:
            self._render(pad)
            self._last_signature=signature
        self.after_id=self.page.win.after(85,self._poll)
