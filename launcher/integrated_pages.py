"""Embedded Aqua settings & controller panel for Dreamcast Recompiled v1.2.

One page inside the existing launcher Canvas.  There are no ordinary
Toplevel settings dialogs or nested notebook menus.  Settings that are not
implemented in stable engine are clearly marked pending; no silent fake flags.
"""
from __future__ import annotations

import ctypes
import json
from pathlib import Path
import subprocess
import sys
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

from core import (VIDEO_DEFAULTS, AUDIO_DEFAULTS, CONTROLLER_DEFAULTS,
                  LAUNCHER_WIDTHS, reset_to_stable, sanitize_settings,
                  config_directory, cache_directory)
from settings_v7 import export_configuration, import_configuration

NAV = {'settings':'Ajustes rápidos','video':'Vídeo y pantalla',
       'display':'Vídeo y pantalla','audio':'Sonido','performance':'Rendimiento',
       'controllers':'Mandos y controles','debug':'Diagnóstico', 'language':'Idioma','themes':'Temas'}
BG='#d9efff'; DARK='#16416e'; LIGHT='#b4dffa'; MUTE='#4776a2'
BTN_BG='#b2ddf9'; BTN_FG=DARK; BTN_ACTIVE='#55b4ed'; STATUS_FG='#217660'; PENDING_FG='#a46b28'

def apply_theme_palette(theme: str):
    """Update page colors to match Aqua theme choices."""
    global BG, DARK, LIGHT, MUTE, BTN_BG, BTN_FG, BTN_ACTIVE, STATUS_FG, PENDING_FG
    if theme in ('dark', 'night'):
        BG = '#192d46'
        DARK = '#ecf3ff'
        LIGHT = '#263e5e'
        MUTE = '#b4c8de'
        BTN_BG = '#314f73'
        BTN_FG = '#ecf5ff'
        BTN_ACTIVE = '#426c99'
        STATUS_FG = '#9ed8bd'
        PENDING_FG = '#d8bc85'
    elif theme == 'starry':
        BG = '#16273b'
        DARK = '#eef4ff'
        LIGHT = '#233853'
        MUTE = '#b8c8df'
        BTN_BG = '#3a5276'
        BTN_FG = '#f4f8ff'
        BTN_ACTIVE = '#5273a2'
        STATUS_FG = '#a7e0ca'
        PENDING_FG = '#d9c48e'
    else:
        BG = '#d9efff'
        DARK = '#16416e'
        LIGHT = '#b4dffa'
        MUTE = '#4776a2'
        BTN_BG = '#b2ddf9'
        BTN_FG = DARK
        BTN_ACTIVE = '#55b4ed'
        STATUS_FG = '#217660'
        PENDING_FG = '#a46b28'


class InlineContent:
    def __init__(self,app,key):
        self.app=app
        self.key=key
        apply_theme_palette(self.app.settings.get('theme','classic'))
        self.win,self.frame=app.scene.open_inline(NAV[key],key)
        self.vars={}
        # Scroll only the content when needed, not the header/date/main window.
        self.canvas=tk.Canvas(self.frame,bg=BG,bd=0,highlightthickness=0)
        sc=ttk.Scrollbar(self.frame,orient='vertical',command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=sc.set)
        self.canvas.pack(side='left',fill='both',expand=True)
        sc.pack(side='right',fill='y')
        self.body=tk.Frame(self.canvas,bg=BG)
        self.window_id=self.canvas.create_window((0,0),window=self.body,anchor='nw')
        self.body.bind('<Configure>',lambda _e:self.canvas.configure(scrollregion=self.canvas.bbox('all')))
        self.canvas.bind('<Configure>',lambda e:self.canvas.itemconfigure(self.window_id,width=e.width))
        self.canvas.bind('<MouseWheel>',self._wheel)
        self.body.bind('<MouseWheel>',self._wheel)
        # Keep the content clean: each page shows controls, not onboarding paragraphs.

    def _wheel(self,e):
        self.canvas.yview_scroll(-1 if e.delta>0 else 1,'units')

    def summary(self,text):
        tk.Label(self.body,text=text,bg=BG,fg=MUTE,anchor='w',justify='left',
                 wraplength=830,font=('Segoe UI',10)).pack(fill='x',padx=18,pady=(10,7))

    def heading(self,text,subtitle='',pending=False):
        p=tk.Frame(self.body,bg=BG)
        p.pack(fill='x',padx=17,pady=(13,4))
        tk.Label(p,text=text,bg=BG,fg=DARK,font=('Segoe UI',13,'bold')).pack(side='left')
        if pending:
            tk.Label(p,text='  ○ PENDIENTE',bg=BG,fg=PENDING_FG,font=('Segoe UI',9,'bold')).pack(side='right')
        if subtitle:
            tk.Label(self.body,text=subtitle,bg=BG,fg=MUTE,anchor='w',justify='left',
                     wraplength=820,font=('Segoe UI',9)).pack(fill='x',padx=20,pady=(0,3))

    def row(self):
        f=tk.Frame(self.body,bg=BG)
        f.pack(fill='x',padx=22,pady=4)
        return f

    def select(self,title,path,options,enabled=True,note=None):
        # path may be a plain setting or a tuple ('video_future', 'resolution').
        saved=self.app.settings
        current=saved[path] if isinstance(path,str) else saved[path[0]][path[1]]
        var=tk.StringVar(value=str(current))
        p=self.row()
        tk.Label(p,text=title,width=27,anchor='w',bg=BG,fg=DARK,
                 font=('Segoe UI',11)).pack(side='left')
        combo=ttk.Combobox(p,values=options,state='readonly' if enabled else 'disabled',
                           width=24,textvariable=var)
        combo.pack(side='left',padx=(5,18))
        if note:tk.Label(p,text=note,bg=BG,fg=MUTE,anchor='w',font=('Segoe UI',9)).pack(side='left')
        def changed(_e=None):
            if isinstance(path,str):saved[path]=var.get()
            else:saved[path[0]][path[1]]=var.get()
            self.app.persist()
            self._show_status('Guardado · se aplicará al iniciar el juego')
        combo.bind('<<ComboboxSelected>>',changed)
        self.vars[path]=var
        return var

    def check(self,title,path,pending=False,note=None):
        saved=self.app.settings
        current=saved[path] if isinstance(path,str) else saved[path[0]][path[1]]
        var=tk.BooleanVar(value=bool(current))
        p=self.row()
        def changed():
            if isinstance(path,str):saved[path]=var.get()
            else:saved[path[0]][path[1]]=var.get()
            self.app.persist();self._show_status('Preferencia guardada')
        ttk.Checkbutton(p,text=title,variable=var,command=changed).pack(side='left')
        if pending:tk.Label(p,text='  ○ Preparado, sin efecto en el motor',bg=BG,fg=PENDING_FG).pack(side='left')
        if note:tk.Label(p,text='  '+note,bg=BG,fg=MUTE).pack(side='left')
        self.vars[path]=var
        return var

    def number(self,title,path,minimum,maximum,suffix='',pending=True):
        current=self.app.settings[path[0]][path[1]]
        p=self.row()
        tk.Label(p,text=title,width=27,anchor='w',bg=BG,fg=DARK,font=('Segoe UI',11)).pack(side='left')
        var=tk.IntVar(value=current)
        spin=ttk.Spinbox(p,from_=minimum,to=maximum,width=9,textvariable=var)
        spin.pack(side='left',padx=5)
        tk.Label(p,text=suffix+('    ○ Pendiente de motor' if pending else ''),bg=BG,fg=MUTE).pack(side='left')
        def save(_e=None):
            try:v=int(var.get())
            except (tk.TclError,ValueError):return
            if minimum<=v<=maximum:
                self.app.settings[path[0]][path[1]]=v
                self.app.persist();self._show_status('Guardado')
        spin.bind('<FocusOut>',save)
        spin.bind('<Return>',save)
        self.vars[path]=var

    def button(self,title,command,accent=False,container=None):
        host=container or self.row()
        b=tk.Button(host,text=title,command=command,bd=0,relief='flat',
                    bg='#2e8acc' if accent else BTN_BG,
                    fg='white' if accent else BTN_FG,padx=14,pady=6,cursor='hand2',
                    activebackground='#55b4ed' if accent else BTN_ACTIVE,
                    activeforeground='white' if accent else BTN_FG,
                    font=('Segoe UI',10,'bold'), highlightthickness=0)
        b.pack(side='left',padx=(0,9),pady=3)
        return b

    def _show_status(self,text):
        # Only a small status line is changed; never redraw the whole launcher.
        if hasattr(self,'_status') and self._status.winfo_exists():
            self._status.configure(text=text)

    def end(self):
        self._status=tk.Label(self.body,text='',
                              bg=BG,fg=STATUS_FG,anchor='w',font=('Segoe UI',9))
        self._status.pack(fill='x',padx=20,pady=(14,22))


def build_integrated_page(app,key):
    app._page_key=key
    page=InlineContent(app,key)
    if key in ('display','video'):
        page.heading('Presentación y resolución',
                     'Configuración de salida')
        page.select('Motor gráfico','renderer',('auto','dx11','software'),note='● Aplica')
        page.select('Resolución de salida',('video_future','resolution'),('native','960p','1080p','1440p','desktop'),note='● Aplica')
        page.select('Proporciones',('video_future','aspect'),('4:3','16:9','auto'),note='● Aplica')
        page.select('Presentación',('video_future','output_backend'),('dx11','gdi'),note='● Aplica')
        page.heading('Extras',
                     'Filtro de presentación aún no disponible.',pending=True)
        page.select('Filtro de salida',('video_future','filter'),('nearest','bilinear','sharp'),note='○ No aplicado aún')
        page.check('Pantalla completa',('video_future','fullscreen'),pending=True)
        page.check('Mostrar FPS en pantalla',('video_future','fps_counter'),pending=True)
        page.button('Restaurar vídeo original',lambda:reset_category(app,'video'),accent=False)
    elif key=='audio':
        page.heading('Audio AICA y Windows','Salida de audio')
        page.check('Activar audio en los juegos','audio')
        page.heading('Preferencias avanzadas',pending=True)
        page.number('Volumen general',('audio_future','volume'),0,100,'%')
        page.number('Latencia del búfer',('audio_future','latency_ms'),20,250,' ms')
        page.button('Restaurar valores de audio',lambda:reset_category(app,'audio'))
    elif key=='performance':
        page.heading('Perfil DCRV1.2 Stable Release','Valores originales recomendados')
        page.check('HOT BLOCK RECOVERY','hot_recovery')
        page.check('SPG Fast Forward','spg_fast_forward')
        page.check('Perfilador de rendimiento','perf',note='Solo para diagnóstico')
        page.heading('Diagnóstico','Las trazas pueden reducir los FPS')
        for title,k in [('Debug completo','debug'),('Traza PVR','pvr_profile'),
                        ('Traza Maple','maple_trace'),('Traza GD-ROM','gdrom_trace')]:page.check(title,k)
        page.button('Restaurar perfil estable',lambda:reset_category(app,'performance'))
        page.button('Ver logs',lambda:app.activate('debug'))
    elif key=='debug':
        page.heading('Salida de diagnóstico')
        page.check('Mostrar consola externa al jugar','show_console')
        page.check('Heartbeats periódicos (1 s)','heartbeats')
        page.check('Traza de reubicación de código SH-4','code_reloc_trace')
        page.summary('Ambos apagados por defecto. Los errores se conservan en los logs aunque no abras una consola.')
        page.heading('Intérprete SH-4','Código que el AOT no compiló')
        page.check('Intérprete SH-4 de respaldo','sh4_interp')
        page.check('Traza del intérprete SH-4','sh4_interp_trace')
        page.heading('AOT incremental para código dinámico', 'Experimental · Captura código RAM y comprueba los bytes antes de usar cada DLL')
        page.check('AOT dinámico por snapshots de RAM','dynamic_aot')
        page.check('Diagnóstico de saltos SH-4 inválidos','bad_target_trace')
        page.summary('Apagado por defecto. Captura código en RAM cuando el intérprete descubre un target nuevo; en el próximo arranque compila solo esas seeds. Un módulo que no coincide con la memoria actual se omite.')
        page.summary('Activo por defecto: una dirección sin AOT se interpreta en vez de cerrar el juego, y se guarda en interp_seeds.txt. Las seeds válidas se compilan incrementalmente al iniciar otra vez el juego.')
        page.heading('Trazas avanzadas')
        for title,item in [('Debug completo','debug'),('Perfil PVR','pvr_profile'),('Traza Maple','maple_trace'),('Traza GD-ROM','gdrom_trace')]:
            page.check(title,item)
        page.button('Consultar registros', lambda:app.logs_dialog())
    elif key=='controllers':
        page.heading('Mandos y calibración')
        pad=ControllerPreview(page)
        pad.mount()
        page.summary('Calibraciones independientes por jugador. «Aplicar al juego» cambia el perfil global activo.')
        advanced=tk.Frame(page.body,bg=BG)
        before=page.body
        page.body=advanced
        editor=ControllerMappingEditor(page)
        editor.mount()
        page.body=before
        def toggle():
            if advanced.winfo_manager():advanced.pack_forget()
            else:advanced.pack(fill='x',padx=6,pady=(5,3))
        page.button('Asignaciones avanzadas ▾',toggle)
        page.button('Abrir Visual Tester completo',lambda:launch_visual_tester(app),accent=False)
        page.button('Abrir archivo del perfil',lambda:open_profile(app))
    elif key=='settings':
        page.heading('Accesos directos','Acceso rápido')
        for left,right in [[('Vídeo y formato 16:9','video'),('Audio','audio')],
                           [('Rendimiento estable','performance'),('Mandos · Visual Tester','controllers')],
                           [('Biblioteca de juegos','games'),('VMU y guardados','vmu')],
                           [('Idioma','language'),('Temas','themes')]]:
            row=page.row()
            for label,dest in (left,right):page.button(label,lambda k=dest:app.activate(k),container=row)
        page.heading('Configuración y mantenimiento')
        page.select('Tamaño del launcher','launcher_width',LAUNCHER_WIDTHS,note='Requiere reiniciar')
        page.button('Exportar ajustes JSON',lambda:export_settings(app))
        page.button('Importar ajustes JSON',lambda:import_settings(app))
        page.button('Restaurar perfil estable',lambda:reset_category(app,'all'))
    elif key=='language':
        page.heading('Idioma de la interfaz','Seleccioná el idioma y regresá a Inicio. El reloj seguirá mostrando la hora real de Windows.')
        page.select('Idioma','language',('es','en'),note='Guardado automático')
        page.button('Aplicar ahora',lambda:apply_language(app))
    elif key=='themes':
        page.heading('Estilo Dreamcast Classic Aqua','El fondo clásico se mantiene en todas las secciones; no hay ventanas de configuración separadas.')
        page.select('Preferencia','theme',('classic','night','starry'),note='3 temas: Classic Aqua, Aqua Night y Dream Night')
    page.end()
    return page


def apply_language(app):
    app.lang=app.settings['language']
    app.scene.close_inline()
    app.scene.draw()


def reset_category(app,key):
    defaults=sanitize_settings({})
    targets={'video':['renderer','video_future'],
             'audio':['audio','audio_future'],
             'performance':['hot_recovery','spg_fast_forward','perf','debug','pvr_profile','maple_trace','gdrom_trace'],
             'all':['renderer','video_future','audio','audio_future','controller_future','show_console','heartbeats','code_reloc_trace',
                    'sh4_interp','sh4_interp_trace',
                    'hot_recovery','spg_fast_forward','perf','debug','pvr_profile','maple_trace','gdrom_trace']}
    for item in targets[key]:
        import copy
        app.settings[item]=copy.deepcopy(defaults[item])
    app.persist()
    app.activate('settings' if key=='all' else key)


def export_settings(app):
    p=filedialog.asksaveasfilename(parent=app.root,title='Exportar ajustes',
                                   defaultextension='.json',filetypes=[('JSON','*.json')],
                                   initialfile='dreamcast_recompiled_v1_2.json')
    if p:export_configuration(app.settings,p)


def import_settings(app):
    p=filedialog.askopenfilename(parent=app.root,title='Importar ajustes',filetypes=[('JSON','*.json')])
    if not p:return
    try:settings=import_configuration(p)
    except (ValueError,OSError,UnicodeError) as e:
        messagebox.showerror('Configuración',str(e),parent=app.root);return
    app.settings=settings;app.persist();app.activate('settings')


def launch_visual_tester(app):
    """The newer 36 KB visual-learning tester, never the old Configurar_Mando.bat.

    It is an explicit advanced operation; the everyday controller panel is inline.
    """
    if sys.platform!='win32':
        messagebox.showinfo('Mandos','Visual Tester requiere Windows.',parent=app.root)
        return
    script=app.root.tk.call('file','normalize',str(Path(__file__).resolve().parent.parent /
                         'tools/controller_test/DreamcastControllerTest.ps1'))
    profile=Path(__file__).resolve().parent.parent/'profiles/controller_profile.ini'
    subprocess.Popen(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass',
                      '-File',str(script),'-ProfilePath',str(profile)],
                     cwd=str(Path(__file__).resolve().parent.parent),
                     creationflags=subprocess.CREATE_NEW_CONSOLE)


def open_profile(app):
    profile=Path(__file__).resolve().parent.parent/'profiles/controller_profile.ini'
    if sys.platform=='win32':
        import os
        if profile.exists():os.startfile(str(profile))
        else:messagebox.showinfo('Mandos','Primero guardá el perfil desde Visual Tester.',parent=app.root)
    else:messagebox.showinfo('Mandos',str(profile),parent=app.root)


from controller_visual import ControllerPreview


class ControllerMappingEditor:
    """Native in-page editor for real stable controller mappings.

    Unlike the staged controller_future preferences this saves the profile
    consumed by the Maple plugin, without recompiling the SH-4 guest.
    """
    def __init__(self,page):
        self.page=page
        from controller_profiles import read_profile
        self.path=Path(__file__).resolve().parent.parent/'profiles/controller_profile.ini'
        self.data=read_profile(self.path)
        self.map_vars={}

    def mount(self):
        from controller_profiles import CONTROLS, BACKENDS
        row=self.page.row()
        tk.Label(row,text='Dispositivo',bg=BG,fg=DARK,font=('Segoe UI',10)).pack(side='left')
        self.backend=tk.StringVar(value=self.data['backend'])
        ttk.Combobox(row,textvariable=self.backend,values=BACKENDS,width=12,state='readonly').pack(side='left',padx=9)
        tk.Label(row,text='Puerto',bg=BG,fg=DARK).pack(side='left')
        self.device=tk.IntVar(value=self.data['device'])
        ttk.Spinbox(row,from_=0,to=15,textvariable=self.device,width=4).pack(side='left',padx=6)
        tk.Label(row,text='Deadzone',bg=BG,fg=DARK).pack(side='left')
        self.deadzone=tk.IntVar(value=round(100*self.data['deadzone']))
        ttk.Spinbox(row,from_=0,to=50,textvariable=self.deadzone,width=4).pack(side='left',padx=6)
        grid=tk.Frame(self.page.body,bg=BG)
        grid.pack(fill='x',padx=22,pady=6)
        for i,key in enumerate(CONTROLS):
            col=i//9;line=i%9
            cell=tk.Frame(grid,bg=BG)
            cell.grid(row=line,column=col,sticky='ew',padx=(0,22),pady=3)
            label={'joy_x':'Stick izq. X','joy_y':'Stick izq. Y',
                   'joy2_x':'Stick der. X','joy2_y':'Stick der. Y',
                   'ltrig':'Gatillo L','rtrig':'Gatillo R'}.get(key,key.upper())
            tk.Label(cell,text=label,width=13,anchor='w',bg=BG,fg=DARK).pack(side='left')
            v=tk.StringVar(value=self.data['bindings'][key]);self.map_vars[key]=v
            ttk.Entry(cell,textvariable=v,width=19).pack(side='left',padx=(4,0))
        self.page.button('Guardar asignaciones en el mando',self.save,accent=True)
        self.page.button('Valores lógicos por defecto',self.reset)

    def save(self):
        from controller_profiles import CONTROLS, write_profile
        try:
            info={'backend':self.backend.get(),'device':int(self.device.get()),
                  'deadzone':int(self.deadzone.get())/100.,
                  'bindings':{k:v.get().strip() for k,v in self.map_vars.items()}}
            write_profile(self.path,info)
        except (ValueError,OSError,tk.TclError) as exc:
            messagebox.showerror('Mandos',str(exc),parent=self.page.app.root)
            return
        self.page._show_status('Perfil del mando guardado para el próximo inicio · copia .ini.bak creada cuando corresponde')

    def reset(self):
        from controller_profiles import DEFAULTS
        for k,v in self.map_vars.items():v.set(DEFAULTS[k])
        self.page._show_status('Asignaciones predeterminadas preparadas · pulsá Guardar para aplicarlas')
