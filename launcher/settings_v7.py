"""Expanded, honest settings front-end for the stable v1.2 runtime.

Only keys listed in core.SUPPORTED_RUNTIME_KEYS become switches in scripts/windows/run_game.bat.
V8 presentation-only resolution/aspect settings are passed through the host
process environment. VSync/fullscreen and advanced filters remain staged.
"""
from __future__ import annotations
import json
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from core import (AUDIO_DEFAULTS, CONTROLLER_DEFAULTS, VIDEO_DEFAULTS,
                  LAUNCHER_WIDTHS, config_directory, cache_directory,
                  reset_to_stable, sanitize_settings, sanitize_video,
                  system_directory)

TAB_NAMES = ('General', 'Vídeo', 'Audio', 'Rendimiento', 'SH-4 y caché', 'Red', 'Mandos', 'Sistema', 'Diagnóstico')


def export_configuration(settings: dict, filename: str | Path) -> Path:
    """Export only known, sanitized launcher settings. Never export guest cache."""
    result = Path(filename)
    payload = {'format': 'Dreamcast Recompiled v1.2 launcher',
               'schema': 8, 'settings': sanitize_settings(settings)}
    result.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    return result


def import_configuration(filename: str | Path) -> dict:
    """Reject unrelated or malformed JSON, silently stripping unknown options."""
    result = json.loads(Path(filename).read_text(encoding='utf-8'))
    if not isinstance(result, dict) or result.get('format') != 'Dreamcast Recompiled v1.2 launcher':
        raise ValueError('Este archivo no es una configuración del launcher v1.2.')
    if type(result.get('schema')) is not int or result['schema'] > 8 or result['schema'] < 1:
        raise ValueError('Versión de configuración incompatible.')
    if not isinstance(result.get('settings'), dict):
        raise ValueError('Falta el objeto settings.')
    return sanitize_settings(result['settings'])


class SettingsPanel:
    def __init__(self, app, initial='General'):
        self.app = app
        self.window, outer = app.new_window('Configuración avanzada · V7', '945x690')
        if hasattr(self.window,'minsize'):self.window.minsize(760, 520)
        self.original = sanitize_settings(app.settings)
        self.vars = {}
        self.videovars = {}
        self.audiovars = {}
        self.padvars = {}
        self.panel = ttk.Notebook(outer)
        ttk.Label(outer, text='DCRV1.2 · Configuración',
                  font=('Segoe UI', 17, 'bold')).pack(anchor='w', pady=(0,4))
        ttk.Label(outer, text='● Activo = soportado    ○ Preparado = guardado, todavía sin implementación en el motor',
                  wraplength=850).pack(anchor='w', pady=(0,10))
        self.panel.pack(fill='both', expand=True)
        self.pages = {}
        for name in TAB_NAMES:
            self.pages[name] = self._scrolling_page(name)
        normalized = {'Video':'Vídeo', 'Debug':'Diagnóstico', 'Controls':'Mandos', 'Rendimiento':'Rendimiento'}
        title = normalized.get(initial, initial)
        self.panel.select(TAB_NAMES.index(title) if title in TAB_NAMES else 0)
        self._build_tabs()
        bar=ttk.Frame(outer)
        bar.pack(fill='x', pady=(11,0))
        ttk.Label(bar, text='Los cambios de juego se aplican en el próximo inicio.').pack(side='left')
        ttk.Button(bar, text='Cancelar', command=self.window.destroy).pack(side='right', padx=4)
        ttk.Button(bar, text='Guardar', command=self._save).pack(side='right', padx=4)
        ttk.Button(bar, text='Restaurar estable', command=self._restore).pack(side='right', padx=4)

    def _scrolling_page(self, title):
        outer = ttk.Frame(self.panel)
        self.panel.add(outer, text=title)
        canvas = tk.Canvas(outer, highlightthickness=0, bg=self.app.bg)
        scrollbar=ttk.Scrollbar(outer, orient='vertical', command=canvas.yview)
        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side='right', fill='y')
        canvas.pack(side='left', fill='both', expand=True)
        body = ttk.Frame(canvas, padding=(17,14,22,20))
        win_id = canvas.create_window((0,0), window=body, anchor='nw')
        body.bind('<Configure>', lambda _e,c=canvas: c.configure(scrollregion=c.bbox('all')))
        canvas.bind('<Configure>', lambda e,c=canvas,i=win_id: c.itemconfigure(i, width=e.width))
        # Scrolls only the page under the pointer; never global bind_all.
        def wheel(e, c=canvas):
            c.yview_scroll(-1 if e.delta>0 else 1, 'units')
        for w in (canvas, body):w.bind('<MouseWheel>', wheel)
        return body

    def _section(self, page, heading, state='applied', note=''):
        line=ttk.Frame(page);line.pack(fill='x', pady=(9,5))
        ttk.Label(line, text=heading, font=('Segoe UI',12,'bold')).pack(side='left')
        ttk.Label(line, text='● Activo en v1.2' if state=='applied' else '○ Pendiente de motor',
                  foreground='#087d58' if state=='applied' else '#a56523').pack(side='right')
        if note:
            ttk.Label(page, text=note, wraplength=780).pack(anchor='w', pady=(0,8))

    def _label(self, parent, value):
        ttk.Label(parent, text=value, wraplength=790).pack(anchor='w', pady=(5,4))

    def _combo(self, parent, title, choices, selected, target, key, text_map=None):
        row=ttk.Frame(parent);row.pack(fill='x',pady=5)
        ttk.Label(row,text=title,width=27).pack(side='left')
        selected_var=tk.StringVar(value=selected)
        # choices are stable backend keys; can be persisted without locale risk.
        ttk.Combobox(row,textvariable=selected_var,values=choices,state='readonly',width=29).pack(side='left')
        target[key]=selected_var
        return selected_var

    def _checkbox(self, parent, title, value, target, key):
        var=tk.BooleanVar(value=value)
        ttk.Checkbutton(parent,text=title,variable=var).pack(anchor='w',pady=4)
        target[key]=var
        return var

    def _number(self, parent, title, value, target, key, low, high, unit):
        row=ttk.Frame(parent);row.pack(fill='x',pady=5)
        ttk.Label(row,text=title,width=27).pack(side='left')
        var=tk.IntVar(value=value)
        ttk.Spinbox(row,from_=low,to=high,textvariable=var,width=10).pack(side='left')
        ttk.Label(row,text=unit).pack(side='left',padx=8)
        target[key]=(var,low,high)

    def _build_tabs(self):
        settings=self.original
        # GENERAL
        tab=self.pages['General']
        self._section(tab,'Información de versión')
        self._label(tab,'DCRV1.2 Stable Release · Base estable. El launcher no modifica runtime, guest SH-4 ni DLLs.')
        self._label(tab,'Jugar utiliza scripts/windows/run_game.bat: conserva la caché ABI1 y puede compilar solo nuevas seeds. El modo completo es explícito.')
        ttk.Button(tab,text='Biblioteca de juegos',command=self.app.games_dialog).pack(anchor='w',pady=4)
        ttk.Button(tab,text='Historial de ejecuciones',command=self.app.history_dialog).pack(anchor='w',pady=4)
        self._section(tab,'Tamaño de la ventana del launcher')
        self._combo(tab,'Resolución del launcher',LAUNCHER_WIDTHS,settings['launcher_width'],self.vars,'launcher_width')
        self._label(tab,'Este ajuste SÍ cambia el launcher tras reiniciarlo; no modifica la resolución de los juegos.')
        self._section(tab,'Configuraciones y copias')
        row=ttk.Frame(tab);row.pack(anchor='w',pady=4)
        ttk.Button(row,text='Exportar JSON',command=self._export).pack(side='left',padx=(0,9))
        ttk.Button(row,text='Importar JSON',command=self._import).pack(side='left')
        ttk.Button(tab,text='Abrir configuración local',command=lambda:self.app.open_config_folder()).pack(anchor='w',pady=4)
        # VIDEO
        tab=self.pages['Vídeo']
        self._section(tab,'Renderizador PVR',note='El launcher emite --renderer=dx11 / software. Automático utiliza el valor del runner.')
        self._combo(tab,'Motor de gráficos',('auto','dx11','software'),settings['renderer'],self.vars,'renderer')
        self._section(tab,'Resolución de salida y formato · V8',
                      note='ACTIVO: escalado de presentación en software y DirectX 11. No cambia el PVR interno de 640×480. El 16:9 estira el juego: NO amplía el campo de visión.')
        video=settings['video_future']
        self._combo(tab,'Resolución de salida',('native','960p','1080p','1440p','desktop'),video['resolution'],self.videovars,'resolution')
        self._combo(tab,'Relación de aspecto',('4:3','16:9','auto'),video['aspect'],self.videovars,'aspect')
        self._combo(tab,'Salida gráfica',('dx11','gdi'),video['output_backend'],self.videovars,'output_backend')
        self._label(tab,'dx11: escalado en GPU (experimental). gdi: compatibilidad mediante lectura del fotograma GPU y escalado Windows; puede reducir FPS, pero evita el shader DX11.')
        self._label(tab,'4:3 conserva las proporciones (con bandas negras). 16:9 estira la imagen horizontalmente. Auto equivale a 4:3.')
        self._section(tab,'Extras pendientes',state='future',
                      note='No implementamos VSync en esta etapa. Los filtros, fullscreen y FPS todavía solo se guardan.')
        self._combo(tab,'Filtro final de escalado',('nearest','bilinear','sharp'),video['filter'],self.videovars,'filter')
        for key, title in [('fullscreen','Pantalla completa sin bordes'),('fps_counter','Superposición de FPS')]:
            self._checkbox(tab,title,video[key],self.videovars,key)
        self._label(tab,'La resolución interna del PVR sigue siendo 640×480; la imagen se escala al tamaño seleccionado sin modificar SH-4, GD-ROM o AICA.')
        ttk.Button(tab,text='Restaurar ajustes de vídeo',command=lambda:self._reset_tab('Vídeo')).pack(anchor='w',pady=8)
        # AUDIO
        tab=self.pages['Audio']
        self._section(tab,'Salida de sonido',note='El audio controla --no-audio/--audio-basic. AICA sigue avanzando virtualmente sin audio host.')
        self._checkbox(tab,'Audio del juego activado',settings['audio'],self.vars,'audio')
        self._combo(tab,'Modo de audio',('full','basic','off'),settings['audio_mode'],self.vars,'audio_mode')
        ttk.Button(tab,text='Probar WAV del host',command=self.app.music_dialog).pack(anchor='w',pady=6)
        self._section(tab,'Mezcla y dispositivo',state='future',note='Estos valores aún NO ajustan AICA/XAudio/WinMM. Están preparados para un futuro backend.')
        audio=settings['audio_future']
        self._number(tab,'Volumen principal',audio['volume'],self.audiovars,'volume',0,100,'%')
        self._number(tab,'Objetivo de latencia',audio['latency_ms'],self.audiovars,'latency_ms',20,250,'ms')
        self._combo(tab,'Dispositivo',('default',),audio['device'],self.audiovars,'device')
        ttk.Button(tab,text='Restaurar ajustes de audio',command=lambda:self._reset_tab('Audio')).pack(anchor='w',pady=8)
        # PERF
        tab=self.pages['Rendimiento']
        self._section(tab,'Perfil estable',note='Mantener activadas las dos primeras opciones salvo diagnóstico específico.')
        for k,t in [('hot_recovery','HOT BLOCK RECOVERY'),('spg_fast_forward','SPG WAIT Fast Forward')]:
            self._checkbox(tab,t,settings[k],self.vars,k)
        self._section(tab,'Perfilado opcional')
        self._checkbox(tab,'Perfilador --perf (puede reducir los FPS)',settings['perf'],self.vars,'perf')
        self._label(tab,'El modo normal mantiene HOTPC off y evita instrumentar el SH-4 en el camino caliente. El FPS no se limita artificialmente.')
        ttk.Button(tab,text='Restaurar rendimiento',command=lambda:self._reset_tab('Rendimiento')).pack(anchor='w',pady=8)
        # SH-4 / CACHE
        tab=self.pages['SH-4 y caché']
        self._section(tab,'Recompilación incremental nativa',
                      note='La opción predeterminada reutiliza el guest ABI1, y solo compila los módulos de seeds nuevas. No fuerza un rebuild completo.')
        self._combo(tab,'Modo de compilación',('incremental','reuse','full'),settings['build_mode'],self.vars,'build_mode')
        self._label(tab,'incremental: automático y por módulos; reuse: nunca recompilar el guest; full: reconstruye TODO en cada ejecución (solo diagnóstico).')
        for k,t in [('native_seeds','Compilar seeds nuevas automáticamente'),
                    ('native_patches','Cargar DLLs incrementales cacheadas'),
                    ('sh4_interp','Intérprete temporal para targets faltantes'),
                    ('sh4_interp_trace','Registrar nuevas entradas interpretadas')]:
            self._checkbox(tab,t,settings[k],self.vars,k)
        self._label(tab,'Desactivar carga incremental también desactiva su compilación en el runner actual.')
        self._section(tab,'FPU y bloques',note='Perfil normal recomendado: superblock.')
        self._combo(tab,'Perfil FPU',('superblock','regionplus'),settings['fpu_mode'],self.vars,'fpu_mode')
        self._checkbox(tab,'HOTPC profiler',settings['hotpc'],self.vars,'hotpc')
        self._section(tab,'Herramientas seguras')
        ttk.Button(tab,text='Abrir caché del juego',command=self.app.open_game_cache_folder).pack(anchor='w',pady=4)
        ttk.Button(tab,text='Ver logs',command=self.app.logs_dialog).pack(anchor='w',pady=4)
        # NETWORK
        tab=self.pages['Red']
        self._section(tab,'Conectividad Dreamcast / DCNet',
                      note='Seleccioná el modo para el puente existente dcr_network.dll. No se instala un segundo stack de red.')
        self._combo(tab,'Modo de red',('auto','dcnet','off'),settings['network_mode'],self.vars,'network_mode')
        self._label(tab,'auto: red habilitada; dcnet: perfil módem PPP/Dreamcast; off: red desactivada.')
        ttk.Button(tab,text='Abrir acceso directo de red',command=self.app.network_dialog).pack(anchor='w',pady=7)
        # MANDOS
        tab=self.pages['Mandos']
        self._section(tab,'Configurador original')
        ttk.Button(tab,text='Abrir configurador de mandos',command=self.app.controllers_dialog).pack(anchor='w',pady=6)
        self._label(tab,'El configurador estable administra perfiles y pruebas de mando. No lo sustituimos con una emulación adicional.')
        self._section(tab,'Preferencias del futuro configurador',state='future')
        pad=settings['controller_future']
        self._number(tab,'Zona muerta',pad['deadzone_pct'],self.padvars,'deadzone_pct',0,40,'%')
        self._checkbox(tab,'Vibración solicitada',pad['rumble'],self.padvars,'rumble')
        self._label(tab,'Estas preferencias no sustituyen los perfiles .ini ni activan vibración por sí solas.')
        ttk.Button(tab,text='Restaurar preferencias de mando',command=lambda:self._reset_tab('Mandos')).pack(anchor='w',pady=8)
        # SISTEMA
        tab=self.pages['Sistema']
        self._section(tab,'Biblioteca, sistema y VMU')
        for text,action in [('Biblioteca / Preparar juego',self.app.games_dialog),
                            ('VMU y respaldos',self.app.vmu_dialog),
                            ('Perfiles por juego',self.app.profiles_dialog),
                            ('Historial de juegos',self.app.history_dialog),
                            ('Sistema BIOS / HLE',self.app.system_dialog),
                            ('Cachés por juego',lambda:self.app.open_game_cache_folder())]:
            ttk.Button(tab,text=text,command=action).pack(anchor='w',pady=4)
        self._label(tab,'Los perfiles game.cfg y la caché del SH-4 se guardan fuera del launcher. No se borran al restaurar ajustes.')
        # DEBUG
        tab=self.pages['Diagnóstico']
        self._section(tab,'Salida y diagnósticos')
        self._checkbox(tab,'Mostrar consola externa',settings['show_console'],self.vars,'show_console')
        self._checkbox(tab,'Habilitar heartbeats (desactivado por defecto)',settings['heartbeats'],self.vars,'heartbeats')
        self._section(tab,'SH-4 dinámico experimental')
        self._checkbox(tab,'AOT incremental desde RAM',settings['dynamic_aot'],self.vars,'dynamic_aot')
        self._checkbox(tab,'Traza de saltos inválidos SH-4',settings['bad_target_trace'],self.vars,'bad_target_trace')
        self._section(tab,'Registros opcionales',note='Estas opciones generan sobrecarga; no se recomiendan para jugar.')
        for key,title in [('debug','Diagnóstico --debug'),('pvr_profile','Traza PVR --profile'),
                          ('maple_trace','Traza de controles Maple'),('gdrom_trace','Traza GD-ROM'),
                          ('code_reloc_trace','Traza de reubicación SH-4')]:
            self._checkbox(tab,title,settings[key],self.vars,key)
        ttk.Button(tab,text='Consultar últimos logs',command=self.app.logs_dialog).pack(anchor='w',pady=12)
        ttk.Button(tab,text='Desactivar trazas y depuración',command=lambda:self._reset_tab('Diagnóstico')).pack(anchor='w',pady=8)

    def _reset_tab(self, title):
        """Reset only controls in the selected tab; Save commits the changes."""
        defaults=sanitize_settings({})
        if title=='Vídeo':
            self.vars['renderer'].set(defaults['renderer'])
            for key,var in self.videovars.items():var.set(defaults['video_future'][key])
        elif title=='Audio':
            self.vars['audio'].set(True)
            for key,var in self.audiovars.items():
                (var[0] if isinstance(var,tuple) else var).set(defaults['audio_future'][key])
        elif title=='Rendimiento':
            for key in ('hot_recovery','spg_fast_forward','perf'):
                self.vars[key].set(defaults[key])
        elif title=='Mandos':
            for key,var in self.padvars.items():
                (var[0] if isinstance(var,tuple) else var).set(defaults['controller_future'][key])
        elif title=='Diagnóstico':
            for key in ('debug','pvr_profile','maple_trace','gdrom_trace','code_reloc_trace','show_console','heartbeats'):
                self.vars[key].set(defaults[key])

    def _values(self):
        candidate = sanitize_settings(self.original)
        for key,var in self.vars.items():candidate[key]=var.get()
        video={key:var.get() for key,var in self.videovars.items()}
        candidate['video_future']=sanitize_video(video)
        audio={}
        for key,var in self.audiovars.items():
            if isinstance(var,tuple):
                value,low,high=var
                try:audio[key]=int(value.get())
                except (ValueError,tk.TclError):audio[key]=-1
            else:audio[key]=var.get()
        candidate['audio_future']=audio
        pad={}
        for key,var in self.padvars.items():
            if isinstance(var,tuple):
                value,low,high=var
                try:pad[key]=int(value.get())
                except (ValueError,tk.TclError):pad[key]=-1
            else:pad[key]=var.get()
        candidate['controller_future']=pad
        return sanitize_settings(candidate)

    def _save(self):
        next_settings=self._values()
        changes=next_settings.get('launcher_width') != self.app.settings.get('launcher_width')
        self.app.settings=next_settings
        self.app.persist()
        self.window.destroy()
        if changes:
            messagebox.showinfo('Tamaño del launcher','Reiniciá el launcher para aplicar el nuevo tamaño de ventana.',parent=self.app.root)

    def _restore(self):
        if not messagebox.askyesno('Restaurar perfil estable',
            '¿Restaurar los ajustes globales a los valores recomendados de v1.2?\n'
            'Se conservan biblioteca y perfiles por juego; estos últimos pueden tener excepciones.',parent=self.window):
            return
        self.app.settings=reset_to_stable(self.app.settings)
        self.app.persist()
        self.window.destroy()
        self.app.settings_dialog('Rendimiento')

    def _export(self):
        path=filedialog.asksaveasfilename(parent=self.window,title='Exportar ajustes del launcher',
            defaultextension='.json',filetypes=[('Configuración JSON','*.json')],
            initialfile='DreamcastRecompiled_launcher_v1.2.json')
        if not path:return
        try:
            export_configuration(self._values(),path)
        except OSError as exc:
            messagebox.showerror('Exportar',str(exc),parent=self.window)
        else:messagebox.showinfo('Exportar',f'Configuración exportada a:\n{path}',parent=self.window)

    def _import(self):
        path=filedialog.askopenfilename(parent=self.window,title='Importar ajustes del launcher',
                                       filetypes=[('Configuración JSON','*.json')])
        if not path:return
        try:opts=import_configuration(path)
        except (OSError, ValueError, UnicodeError) as exc:
            messagebox.showerror('Importar',str(exc),parent=self.window);return
        if not messagebox.askyesno('Importar',
            '¿Reemplazar la configuración del launcher? No se toca el guest SH-4 ni las VMU.',parent=self.window):return
        self.app.settings=opts
        self.app.persist()
        self.window.destroy()
        messagebox.showinfo('Importación completa','Reabrí los ajustes para ver los cambios. Si cambió el tamaño, reiniciá el launcher.',parent=self.app.root)
