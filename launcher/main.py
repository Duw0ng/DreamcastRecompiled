"""Dreamcast Recompiled v1.2 GUI (stdlib Tkinter, Windows).

The artwork is a navigational layer; every click opens a working dialog or action.
The stable renderer, AICA and recompiler code are not modified.
"""
from __future__ import annotations

import datetime as dt
import os
from pathlib import Path
import shutil
import subprocess
import sys
import traceback
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

from game_limits import RECOMMENDED_SGGG, read_limits, save_limits

from core import (SUPPORTED_SUFFIXES, add_to_library, build_launch_command, cache_directory,
                  config_directory, is_supported_disc, list_game_profiles, list_log_files,
                  load_settings, normalized_disc, save_settings, system_directory,
                  launcher_environment)

# A standalone PyInstaller build uses the installed EXE directory for scripts/windows/run_game.bat
# and _MEIPASS (via __file__) for its bundled Aqua artwork.
ROOT = Path(sys.executable).resolve().parent if getattr(sys, 'frozen', False) else Path(__file__).resolve().parent.parent
HERE = Path(__file__).resolve().parent

LABELS = {
    'es': {
        'ready': 'Listo', 'play': 'Jugar', 'games': 'Juegos', 'vmu': 'VMU / Archivos',
        'music': 'Música', 'settings': 'Configuración', 'controllers': 'Mandos',
        'display': 'Pantalla', 'network': 'Red', 'audio': 'Audio', 'video': 'Vídeo',
        'performance': 'Rendimiento', 'saves': 'Guardados', 'system': 'BIOS / Sistema',
        'profiles': 'Perfiles por juego', 'debug': 'Diagnóstico', 'language': 'Idioma',
        'themes': 'Temas', 'cancel': 'Cancelar', 'save': 'Guardar', 'close': 'Cerrar',
        'choose': 'Elegir imagen CDI o ISO', 'selected': 'Juego seleccionado',
    },
    'en': {
        'ready': 'Ready', 'play': 'Play', 'games': 'Games', 'vmu': 'VMU / Files',
        'music': 'Music', 'settings': 'Settings', 'controllers': 'Controllers',
        'display': 'Display', 'network': 'Network', 'audio': 'Audio', 'video': 'Video',
        'performance': 'Performance', 'saves': 'Saves', 'system': 'BIOS / System',
        'profiles': 'Per-game profiles', 'debug': 'Debug', 'language': 'Language',
        'themes': 'Themes', 'cancel': 'Cancel', 'save': 'Save', 'close': 'Close',
        'choose': 'Select CDI or ISO disc image', 'selected': 'Selected game',
    }
}

BG_LIGHT = '#eaf4ff'
BG_DARK = '#17283e'
FG_LIGHT = '#102945'
FG_DARK = '#e8f6ff'


def ensure_windows() -> bool:
    return sys.platform == 'win32'


def open_folder(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)
    if ensure_windows():
        os.startfile(str(path))  # Windows-only, no shell commands.
    else:
        subprocess.Popen(['xdg-open', str(path)])


class Launcher:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.settings = load_settings()
        self.lang = self.settings['language']
        self.state_path = config_directory() / 'settings.json'
        self.running: subprocess.Popen | None = None
        self.windows: set[tk.Widget] = set()
        self._page_key = 'settings'
        self._music_path: Path | None = None
        self._windows_theme()
        self._setup_main()
        def close_launcher():
            # Cancel the live clock before Tcl destroys the root; prevents a
            # pending callback from firing against a deleted interpreter.
            job=self.scene._clock_job
            if job is not None:
                try:self.root.after_cancel(job)
                except tk.TclError:pass
                self.scene._clock_job=None
            self.root.destroy()
        self.root.protocol('WM_DELETE_WINDOW',close_launcher)

        # Opening launcher with a CDI/ISO passed from Explorer selects that disc;
        # launching still requires an explicit Play click.
        if len(sys.argv) >= 2 and is_supported_disc(sys.argv[1]):
            self.add_games([sys.argv[1]])
            self.select_game(normalized_disc(sys.argv[1]))
        self.refresh_status()

    def tr(self, key: str) -> str:
        return LABELS[self.lang].get(key, key)

    def _windows_theme(self) -> None:
        style = ttk.Style(self.root)
        if 'clam' in style.theme_names():
            style.theme_use('clam')
        dark_theme = self.settings['theme'] in ('dark','night','starry')
        self.bg = BG_DARK if dark_theme else BG_LIGHT
        self.fg = FG_DARK if dark_theme else FG_LIGHT
        style.configure('TFrame', background=self.bg)
        style.configure('TLabel', background=self.bg, foreground=self.fg, font=('Segoe UI', 10))
        style.configure('TLabelFrame', background=self.bg, foreground=self.fg)
        style.configure('TLabelFrame.Label', background=self.bg, foreground=self.fg)
        style.configure('TCheckbutton', background=self.bg, foreground=self.fg)
        style.configure('TRadiobutton', background=self.bg, foreground=self.fg)
        style.configure('TButton', padding=6, font=('Segoe UI', 10))
        style.configure('TNotebook', background=self.bg)
        style.configure('TNotebook.Tab', padding=(13, 7), font=('Segoe UI', 10))
        if self.settings['theme'] in ('dark','night','starry'):
            style.configure('TButton', background='#314f73', foreground='#ecf5ff')
            style.map('TButton', background=[('active','#426c99')], foreground=[('active','white')])
            style.map('TCheckbutton', background=[('active', self.bg)], foreground=[('active', self.fg)])
            style.map('TRadiobutton', background=[('active', self.bg)], foreground=[('active', self.fg)])
            style.configure('TSpinbox', fieldbackground='#253c58', foreground='#ecf5ff', arrowcolor='#ecf5ff')
            style.configure('TCombobox', fieldbackground='#253c58', foreground='#ecf5ff', arrowcolor='#ecf5ff')
            style.map('TCombobox', fieldbackground=[('readonly','#253c58')], foreground=[('readonly','#ecf5ff')])
            style.configure('Treeview', background='#203550', fieldbackground='#203550', foreground='#e9f4ff', rowheight=25)
            style.map('Treeview', background=[('selected','#286da6')], foreground=[('selected','#ffffff')])
            style.configure('Treeview.Heading', background='#294a6d', foreground='#ffffff')
        else:
            style.configure('TButton', background='#edf4ff', foreground='#102945')
            style.map('TCheckbutton', background=[('active', self.bg)], foreground=[('active', self.fg)])
            style.map('TRadiobutton', background=[('active', self.bg)], foreground=[('active', self.fg)])
            style.configure('TSpinbox', fieldbackground='#ffffff', foreground='#102945', arrowcolor='#102945')
            style.configure('TCombobox', fieldbackground='#ffffff', foreground='#102945', arrowcolor='#102945')
            style.map('TCombobox', fieldbackground=[('readonly','#ffffff')], foreground=[('readonly','#102945')])
            style.configure('Treeview', background='#ffffff', fieldbackground='#ffffff', foreground='#102945', rowheight=25)
            style.configure('Treeview.Heading', background='#ddeefe', foreground='#102945')

    def _setup_main(self) -> None:
        """Create native Aqua controls, retaining the proven stable runtime."""
        self.root.title('DreamcastRecompiled V1.2 Stable Release')
        try:
            if ensure_windows():
                self.root.iconbitmap(str(HERE / 'assets' / 'DreamcastRecompiled.ico'))
            else:
                self.icon = tk.PhotoImage(file=str(HERE / 'assets' / 'icon.png'))
                self.root.iconphoto(True, self.icon)
        except tk.TclError:
            pass
        from aqua_ui import AquaDashboard
        self.scene = AquaDashboard(self)
        self.canvas = self.scene.canvas
        self.width = self.scene.width
        self.height = round(864 * self.scene.scale)
        self.hotspots = self.scene.hotspots

    def _hit_test(self, x: int, y: int):
        return self.scene.hit_test(x, y)

    def refresh_status(self):
        game = self.settings.get('last_game')
        if not hasattr(self, 'scene'):
            return
        label = Path(game).stem if game else ('Elegí un juego desde Juegos' if self.lang == 'es' else 'Choose a game from Games')
        self.scene.set_message(label)

    def persist(self):
        save_settings(self.settings, self.state_path)
        self.refresh_status()

    def new_window(self, title: str, size='790x590') -> tuple[tk.Frame, ttk.Frame]:
        """Legacy content now lives inside the Aqua dashboard, not a popup.

        Previous dialogs keep their business logic, file pickers and actions;
        only their host is replaced with a frame inside the same window.
        """
        host,area = self.scene.open_inline(title, self._page_key)
        frm=ttk.Frame(area,padding=14)
        frm.pack(fill='both',expand=True)
        self.windows.clear()
        self.windows.add(host)
        host.bind('<Escape>',lambda _e:host.destroy())
        return host,frm


    def info(self, title: str, message: str):
        messagebox.showinfo(title, message, parent=self.root)

    def activate(self, key: str):
        """One-click navigation: no settings dialog or nested Notebook."""
        self._page_key=key
        if key=='play':
            self.play()
            return
        # Reuse the stable library/VMU/profile logic through the inline host.
        classic={'games':self.games_dialog,'vmu':self.vmu_dialog,
                 'music':self.music_dialog,'network':self.network_dialog,
                 'saves':self.vmu_dialog,'system':self.system_dialog,
                 'profiles':self.profiles_dialog}
        if key in classic:
            classic[key]()
            return
        if key in ('settings','display','video','audio','performance','controllers','debug','language','themes'):
            from integrated_pages import build_integrated_page
            build_integrated_page(self,key)
            return
        raise ValueError(f'Unknown launcher section {key}')


    def select_game(self, value: str):
        self.settings['last_game'] = value
        self.persist()

    def add_games(self, paths: list[str | Path]):
        count = add_to_library(self.settings, paths)
        if count and not self.settings['last_game']:
            self.settings['last_game'] = self.settings['library'][0]
        self.persist()
        return count

    def pick_game(self, initial=None):
        kwargs = {'title': self.tr('choose'), 'filetypes': [('Dreamcast CDI / ISO', '*.cdi *.iso'), ('All files', '*.*')]}
        game = filedialog.askopenfilename(parent=initial or self.root, **kwargs)
        if game:
            if not is_supported_disc(game):
                messagebox.showerror('Dreamcast', 'Formato no soportado. Esta version acepta CDI e ISO.', parent=initial or self.root)
                return
            self.add_games([game])
            self.select_game(normalized_disc(game))
            return game
        return None

    def play(self, prepare: bool = False):
        game = self.settings.get('last_game')
        if not game or not is_supported_disc(game):
            game = self.pick_game()
            if not game:
                return
        if prepare:
            yes = messagebox.askyesno('Preparar juego / Compile SH-4',
                'Esta acción fuerza regenerar el SH-4 COMPLETO y puede tardar varios minutos.\n'
                'Para compilar solo las seeds nuevas, usa modo incremental en Ajustes.\n\n¿Continuar?', parent=self.root)
            if not yes:
                return
        try:
            argv = build_launch_command(ROOT, game, self.settings, prepare=prepare)
            if not prepare and self.settings.get('build_mode') != 'full' \
                    and (ROOT / 'games' / 'CHUCHU_ROCKET' / 'portable_manifest.txt').is_file():
                from portable_games import auto_game_command
                argv = auto_game_command(ROOT, Path(game))
            if not ensure_windows():
                raise RuntimeError('El runner de juegos requiere Windows. La GUI puede probarse en Linux.')
            # Separate console visibility from gameplay/graphics and diagnostic
            # heartbeat. Hidden mode still retains the BAT/compiler output.
            # The game window is created by the runtime independently.
            visible = self.settings.get('show_console', False)
            flags = (getattr(subprocess, "CREATE_NEW_CONSOLE", 0x00000010) if visible
                     else getattr(subprocess, "CREATE_NO_WINDOW", 0x08000000))
            if visible:
                proc = subprocess.Popen(argv, cwd=str(ROOT),
                                        env=launcher_environment(self.settings, game),
                                        creationflags=flags)
            else:
                console_log = config_directory() / 'launcher_console.log'
                console_log.parent.mkdir(parents=True, exist_ok=True)
                with console_log.open('ab') as output:
                    output.write((f'\n--- DCR {dt.datetime.now().isoformat(timespec="seconds")} '
                                  f'{Path(game).name} ---\n').encode('utf-8'))
                    output.flush()
                    proc = subprocess.Popen(argv, cwd=str(ROOT),
                                            env=launcher_environment(self.settings, game),
                                            stdout=output, stderr=subprocess.STDOUT,
                                            stdin=subprocess.DEVNULL,
                                            creationflags=flags)
        except Exception as exc:
            messagebox.showerror('No se pudo iniciar el juego', str(exc), parent=self.root)
            return
        # Local launch history records only successful process creation; not FPS
        # or guest state. No data is uploaded or written to the SH-4 cache.
        self.settings.setdefault('history',[]).append({
            'game':normalized_disc(game), 'when':dt.datetime.now().isoformat(timespec='seconds'),
            'mode':'prepare' if prepare else 'play'})
        self.settings['history']=self.settings['history'][-250:]
        self.persist()
        self.running = proc
        self.scene.set_message(f'Iniciando {Path(game).stem} (PID {proc.pid}) · {self.settings.get("build_mode","incremental")}')
        self.root.after(900, self._monitor_process)

    def _monitor_process(self):
        proc = self.running
        if proc is None:
            return
        if proc.poll() is None:
            self.root.after(1500, self._monitor_process)
            return
        self.running = None
        self.refresh_status()
        if proc.returncode:
            messagebox.showwarning('Dreamcast Recompiled',
                f'La ejecución terminó con código {proc.returncode}.\n'
                'Revisa los logs de la caché del juego y launcher_console.log en la carpeta de ajustes.\n\n'
                'Si falta el guest ABI1, usa el modo incremental o prepara el juego una vez.', parent=self.root)

    def games_dialog(self):
        self._page_key='games'
        win, parent = self.new_window(self.tr('games'), '890x590')
        ttk.Label(parent, text='Biblioteca CDI / ISO · recompilación incremental', font=('Segoe UI', 17, 'bold')).pack(anchor='w', pady=(0, 9))
        ttk.Label(parent, text='Los juegos no se copian: se guardan sus rutas y se conserva la caché SH-4.').pack(anchor='w')

        # All Treeview widgets MUST be children of the library container.  In
        # V8 the tree/scrollbar were created as siblings of this frame, then
        # packed with "in_=panel". Tk could place the opaque frame above the
        # sibling Treeview, so the library looked empty even with saved games.
        panel = ttk.Frame(parent)
        panel.pack(fill='both', expand=True, pady=(10, 8))
        cols = ('game', 'type', 'status')
        tree = ttk.Treeview(panel, show='headings', columns=cols, selectmode='browse', height=14)
        tree.heading('game', text='Nombre / Ruta')
        tree.heading('type', text='Tipo')
        tree.heading('status', text='Estado')
        tree.column('game', width=580, minwidth=180, stretch=True)
        tree.column('type', width=65, minwidth=55, stretch=False, anchor='center')
        tree.column('status', width=118, minwidth=105, stretch=False, anchor='center')
        vs = ttk.Scrollbar(panel, orient='vertical', command=tree.yview)
        tree.configure(yscrollcommand=vs.set)
        tree.pack(side='left', fill='both', expand=True)
        vs.pack(side='right', fill='y')
        tree.tag_configure('missing', foreground='#9d5361')
        info_line = ttk.Label(parent, text='', font=('Segoe UI', 9))
        info_line.pack(fill='x', pady=(0, 8))
        ids = {}

        def redraw(select=None):
            items = tree.get_children()
            if items:
                tree.delete(*items)
            ids.clear()
            available = 0
            for i, item in enumerate(self.settings['library']):
                exists = is_supported_disc(item)
                available += bool(exists)
                label = 'Disponible' if exists else 'No encontrado'
                iid = str(i)
                ids[iid] = item
                tree.insert('', 'end', iid=iid,
                            values=(f'{Path(item).stem}   —   {item}',
                                    Path(item).suffix.upper().removeprefix('.'), label),
                            tags=() if exists else ('missing',))
                if item == select or (select is None and item == self.settings.get('last_game')):
                    tree.selection_set(iid)
                    tree.focus(iid)
                    tree.see(iid)
            total = len(self.settings['library'])
            info_line.configure(text=f'{total} juego(s) en biblioteca  ·  {available} disponible(s)'
                                + ('  ·  Agregá un CDI o ISO para comenzar.' if total == 0 else ''))
        redraw()

        def current():
            selected = tree.selection()
            return ids.get(selected[0]) if selected else None

        def choose():
            game = current()
            if game:
                self.select_game(game)
                win.destroy()

        def add_file():
            p = self.pick_game(initial=win)
            if p:
                redraw(p)

        def add_folder():
            directory = filedialog.askdirectory(parent=win, title='Carpeta de imágenes CDI / ISO')
            if not directory:
                return
            files = []
            # Cap broad library scans and avoid traversing huge drives indefinitely.
            for p in Path(directory).rglob('*'):
                if p.is_file() and p.suffix.lower() in SUPPORTED_SUFFIXES:
                    files.append(p)
                if len(files) >= 500:
                    break
            count = self.add_games(files)
            redraw(normalized_disc(files[0]) if files else None)
            messagebox.showinfo('Biblioteca', f'Agregados: {count} juegos. Límite por búsqueda: 500.', parent=win)

        def remove():
            p = current()
            if not p:
                return
            self.settings['library'].remove(p)
            if self.settings['last_game'] == p:
                self.settings['last_game'] = self.settings['library'][0] if self.settings['library'] else ''
            self.persist()
            redraw()

        def launch():
            game = current()
            if not game:
                return
            self.select_game(game)
            win.destroy()
            self.play()

        def prepare():
            game = current()
            if not game:
                return
            self.select_game(game)
            win.destroy()
            self.play(prepare=True)

        def game_options():
            game = current()
            if game:
                self.per_game_dialog(game)

        controls = ttk.Frame(parent)
        controls.pack(fill='x')
        for title, callback in [('Agregar CDI/ISO', add_file), ('Añadir carpeta', add_folder),
                                ('Seleccionar', choose), ('Jugar', launch), ('Preparar SH-4', prepare),
                                ('Opciones por juego', game_options), ('Quitar', remove)]:
            ttk.Button(controls, text=title, command=callback).pack(side='left', padx=2)
        tree.bind('<Double-1>', lambda _e: choose())

    def per_game_dialog(self, disc: str):
        from core import VIDEO_DEFAULTS, sanitize_video
        win, box = self.new_window('Opciones por juego', '680x520')
        ttk.Label(box, text=Path(disc).name, font=('Segoe UI', 14, 'bold')).pack(anchor='w', pady=(5,10))
        saved=self.settings['game_overrides'].get(disc,{})
        ren=tk.StringVar(value=saved.get('renderer','inherit'))
        aud=tk.StringVar(value=saved.get('audio','inherit'))
        for title, var, choices in [
            ('Renderizador (aplicado)',ren,('inherit','auto','dx11','software')),
            ('Audio (aplicado)',aud,('inherit','on','off'))]:
            row=ttk.Frame(box);row.pack(fill='x',pady=7)
            ttk.Label(row,text=title,width=28).pack(side='left')
            ttk.Combobox(row,textvariable=var,values=choices,state='readonly').pack(side='left',fill='x',expand=True)
        ttk.Separator(box).pack(fill='x',pady=15)
        ttk.Label(box,text='Presentación de vídeo por juego · V8 (resolución y aspecto activos)',
                  font=('Segoe UI',11,'bold')).pack(anchor='w',pady=4)
        ttk.Label(box,text='Resolución y 16:9 estirado funcionan al iniciar el juego. No cambian la resolución interna 640×480 ni el campo de visión.',
                  wraplength=600).pack(anchor='w',pady=3)
        vf=sanitize_video(saved.get('video_future',self.settings['video_future']))
        entries={}
        for label,key,opts in [
            ('Resolución','resolution',('native','960p','1080p','1440p','desktop')),
            ('Aspecto','aspect',('4:3','16:9','auto')),
            ('Salida gráfica','output_backend',('dx11','gdi')),
            ('Filtro','filter',('nearest','bilinear','sharp'))]:
            row=ttk.Frame(box);row.pack(fill='x',pady=5)
            ttk.Label(row,text=label,width=28).pack(side='left')
            var=tk.StringVar(value=vf[key]);entries[key]=var
            ttk.Combobox(row,textvariable=var,values=opts,state='readonly').pack(side='left',fill='x',expand=True)
        ttk.Label(box,text='Salida 4:3: bandas laterales en 16:9. Salida 16:9: imagen 4:3 estirada (no widescreen real). Filtros, VSync y fullscreen siguen pendientes.',
                  wraplength=600).pack(anchor='w',pady=10)
        def save():
            from core import sanitize_video
            self.settings['game_overrides'][disc]={
                'renderer':ren.get(), 'audio':aud.get(),
                'video_future':sanitize_video({**vf,**{key:var.get() for key,var in entries.items()}})}
            self.persist()
            win.destroy()
        ttk.Button(box,text='Guardar perfil',command=save).pack(side='right',pady=8)
        def reset_game():
            if not messagebox.askyesno('Restaurar juego','¿Quitar las excepciones de este juego y usar los ajustes globales?',parent=win):return
            self.settings['game_overrides'].pop(disc,None)
            self.persist();win.destroy()
        ttk.Button(box,text='Usar valores globales',command=reset_game).pack(side='left',pady=8)

    def settings_dialog(self, tab='General'):
        from integrated_pages import build_integrated_page
        translate={'Video':'video','Vídeo':'video','Audio':'audio',
                   'Rendimiento':'performance','Mandos':'controllers',
                   'Debug':'debug','Diagnóstico':'debug','General':'settings'}
        return build_integrated_page(self,translate.get(tab,'settings'))


    def open_config_folder(self):
        open_folder(config_directory())

    def open_game_cache_folder(self):
        open_folder(cache_directory() / 'games')

    def controllers_dialog(self):
        from integrated_pages import build_integrated_page
        self._page_key='controllers'
        return build_integrated_page(self,'controllers')

    def _controller_legacy_dialog(self):
        win, box = self.new_window(self.tr('controllers'), '660x350')
        ttk.Label(box, text='Controles Dreamcast / Xbox / DualShock / DualSense', font=('Segoe UI', 14, 'bold')).pack(anchor='w', pady=10)
        ttk.Label(box, text='Usa el configurador original de la estable.\nIncluye perfiles de botones, puertos y controles de prueba.').pack(anchor='w', pady=10)
        script = ROOT / 'scripts' / 'controller' / 'Configurar_Mando.bat'
        def run():
            if not ensure_windows():
                self.info('Mandos', 'El configurador requiere Windows.')
                return
            try:
                subprocess.Popen([str(script)], cwd=str(ROOT), creationflags=subprocess.CREATE_NEW_CONSOLE)
            except OSError as exc:
                messagebox.showerror('Mandos', str(exc), parent=win)
        ttk.Button(box, text='Abrir configurador del mando', command=run).pack(anchor='w', pady=6)
        ttk.Button(box, text='Abrir perfiles de mandos', command=lambda: open_folder(ROOT / 'profiles')).pack(anchor='w', pady=6)

    def vmu_dialog(self):
        from vmu_ui import open_vmu_page
        return open_vmu_page(self)

    def music_dialog(self):
        win, box = self.new_window(self.tr('music'), '640x300')
        ttk.Label(box, text='Music · Reproductor de prueba WAV', font=('Segoe UI', 15, 'bold')).pack(anchor='w', pady=8)
        ttk.Label(box, text='Reproduce WAV locales; no modifica las pistas CDDA de los juegos.').pack(anchor='w', pady=8)
        selected = ttk.Label(box, text='Ningún archivo seleccionado')
        selected.pack(anchor='w', pady=8)
        def pick():
            file = filedialog.askopenfilename(parent=win, filetypes=[('Audio WAV', '*.wav')])
            if file:
                self._music_path = Path(file)
                selected.configure(text=self._music_path.name)
        def start():
            if not self._music_path:
                pick()
            if not self._music_path:
                return
            if not ensure_windows():
                self.info('Music', 'La reproducción WAV utiliza la API WinMM de Windows.')
                return
            try:
                import winsound
                winsound.PlaySound(str(self._music_path), winsound.SND_FILENAME | winsound.SND_ASYNC | winsound.SND_NODEFAULT)
            except Exception as exc:
                messagebox.showerror('Music', str(exc), parent=win)
        def stop():
            if ensure_windows():
                import winsound
                winsound.PlaySound(None, 0)
        buttons = ttk.Frame(box)
        buttons.pack(anchor='w', pady=8)
        for label, action in [('Elegir WAV', pick), ('▶ Reproducir', start), ('■ Detener', stop)]:
            ttk.Button(buttons, text=label, command=action).pack(side='left', padx=5)

    def network_dialog(self):
        self._page_key = 'network'
        win, box = self.new_window(self.tr('network'), '770x520')
        ttk.Label(box, text='Red / DCNet', font=('Segoe UI', 16, 'bold')).pack(anchor='w', pady=(7,10))
        ttk.Label(box, text='El modo DCNet utiliza el puente de módem V.42/LAPM y PPP del runtime actual.',
                  wraplength=750).pack(anchor='w', pady=(0,8))
        ttk.Label(box, text='La conexión entre instancias ya funcionó en pruebas anteriores; la disponibilidad de servidores externos depende del servicio.',
                  wraplength=750).pack(anchor='w', pady=(0,10))
        mode = tk.StringVar(value=self.settings.get('network_mode', 'auto'))
        options = [('auto','Automático — red habilitada'),
                   ('dcnet','DCNet — módem/PPP'),('off','Sin conexión — desactivar red')]
        for value, caption in options:
            ttk.Radiobutton(box, text=caption, variable=mode, value=value).pack(anchor='w', pady=5)
        def save_network():
            self.settings['network_mode'] = mode.get()
            self.persist()
            self.scene.set_message('Modo de red: ' + mode.get())
            self.scene.close_inline()
        ttk.Button(box, text='Guardar configuración de red', command=save_network).pack(anchor='w', pady=(18,9))
        ttk.Button(box, text='Ver registros', command=self.logs_dialog).pack(anchor='w', pady=4)
        ttk.Label(box, text='Estos cambios se aplican en el siguiente arranque y no recompilan el guest SH-4.',
                  wraplength=750).pack(anchor='w', pady=10)

    def profiles_dialog(self):
        win, box = self.new_window('Perfiles por juego · Funciones SH-4', '890x650')
        ttk.Label(box, text='Límites de recompilación por juego',
                  font=('Segoe UI', 15, 'bold')).pack(anchor='w', pady=8)
        ttk.Label(box, text='Seleccioná un game.cfg para configurar su descubrimiento SH-4. '
                  'Solo ese juego se recompilará si cambiás los límites.',
                  wraplength=810).pack(anchor='w')
        lst = tk.Listbox(box, height=9, font=('Consolas', 10), exportselection=False)
        lst.pack(fill='both', expand=True, pady=8)
        profiles = list_game_profiles()
        for profile in profiles:
            lst.insert('end', profile.parent.name)
        if not profiles:
            ttk.Label(box, text='Aún no hay perfiles. Ejecutá cada juego una vez para crear su game.cfg.').pack(anchor='w')
        editors = ttk.Frame(box)
        editors.pack(fill='x', pady=7)
        limits = {}
        for col, (key, label) in enumerate((
                ('max_functions', 'Máximo de funciones finales'),
                ('max_closure_entries', 'Entradas de closure (0 = auto)'),
                ('closure_passes', 'Pasadas de análisis'))):
            item = ttk.Frame(editors)
            item.grid(row=0, column=col, padx=8, sticky='ew')
            ttk.Label(item, text=label).pack(anchor='w')
            value = tk.StringVar(value='')
            ttk.Entry(item, textvariable=value, width=23).pack(anchor='w', pady=(3, 0))
            limits[key] = value
            editors.columnconfigure(col, weight=1)
        hint = ttk.Label(box, text='La reserva entre max_functions y closure evita superar el límite final.',
                         wraplength=810)
        hint.pack(anchor='w', pady=(2, 8))
        info = tk.StringVar(value='Elegí un juego para ver los límites actuales.')
        ttk.Label(box, textvariable=info, wraplength=810).pack(anchor='w', pady=(0, 10))

        def selected_profile():
            return profiles[lst.curselection()[0]] if lst.curselection() else None

        def refresh(_event=None):
            path = selected_profile()
            if path is None:
                return
            try:
                values = read_limits(path)
                for key, val in values.items():
                    limits[key].set(str(val))
                reserve = values['max_functions'] - values['max_closure_entries']
                info.set(f'Juego: {path.parent.name} | Reserva de funciones: {reserve:,} | '
                         f'Config: {path}')
            except (ValueError, OSError) as exc:
                info.set(f'No se pudo leer el perfil: {exc}')

        def save():
            path = selected_profile()
            if path is None:
                messagebox.showinfo('Perfiles SH-4', 'Seleccioná un juego primero.', parent=win)
                return
            try:
                changed = save_limits(path, *(limits[key].get() for key in
                                              ('max_functions', 'max_closure_entries', 'closure_passes')))
            except (OSError, ValueError) as exc:
                messagebox.showerror('Límites SH-4', str(exc), parent=win)
                return
            refresh()
            if changed:
                messagebox.showinfo('Límites SH-4',
                                    'Guardado para ' + path.parent.name + '.\n'
                                    'Para aplicar el cambio, usá Juegos > Preparar SH-4 '
                                    'o scripts/windows/run_game.bat sin --reuse. Solo se recompilará este juego. '
                                    'El botón Jugar reutiliza caché y no recompila.', parent=win)
            else:
                messagebox.showinfo('Límites SH-4', 'Sin cambios; no se recompilará el guest.', parent=win)

        def recommended_sggg():
            path = selected_profile()
            if path is None or 'SGGG' not in path.parent.name.upper().replace(' ', '') and 'SEGAGAGA' not in path.parent.name.upper().replace(' ', ''):
                messagebox.showinfo('SGGG', 'Seleccioná primero el perfil de Segagaga.', parent=win)
                return
            for key, value in RECOMMENDED_SGGG.items():
                limits[key].set(str(value))
            info.set('SGGG: 10240 funciones finales, 7680 entradas de closure y 64 pasadas. Presioná Guardar.')

        actions = ttk.Frame(box)
        actions.pack(fill='x', pady=6)
        ttk.Button(actions, text='Guardar límites', command=save).pack(side='left', padx=3)
        ttk.Button(actions, text='Valores recomendados SGGG', command=recommended_sggg).pack(side='left', padx=3)
        def open_selected():
            path = selected_profile()
            if path and ensure_windows():
                os.startfile(str(path))
        ttk.Button(actions, text='Abrir game.cfg', command=open_selected).pack(side='left', padx=3)
        ttk.Button(actions, text='Abrir cachés', command=lambda: open_folder(cache_directory() / 'games')).pack(side='left', padx=3)
        lst.bind('<<ListboxSelect>>', refresh)
        if profiles:
            lst.selection_set(0)
            refresh()

    def history_dialog(self):
        win,box=self.new_window('Historial de juegos','790x490')
        ttk.Label(box,text='Últimos lanzamientos desde el launcher V7',
                  font=('Segoe UI',15,'bold')).pack(anchor='w',pady=7)
        ttk.Label(box,text='Registro local, sin estadísticas inventadas de FPS ni lectura del CDI.').pack(anchor='w')
        items=list(reversed(self.settings.get('history',[])))
        grid=ttk.Treeview(box,columns=('when','game','mode'),show='headings')
        for key,title,width in [('when','Fecha',165),('game','Juego',410),('mode','Acción',105)]:
            grid.heading(key,text=title)
            grid.column(key,width=width,stretch=(key=='game'))
        grid.pack(fill='both',expand=True,pady=9)
        for row in items:
            grid.insert('','end',values=(row['when'],Path(row['game']).name,row['mode']))
        ttk.Button(box,text='Abrir biblioteca',command=self.games_dialog).pack(anchor='e',pady=6)

    def logs_dialog(self):
        win, box = self.new_window('Logs / Diagnóstico', '900x600')
        ttk.Label(box, text='Logs del runtime de los juegos', font=('Segoe UI', 15, 'bold')).pack(anchor='w', pady=8)
        logs = list_log_files()
        names = ttk.Combobox(box, state='readonly', values=[str(p) for p in logs])
        names.pack(fill='x')
        view = tk.Text(box, font=('Consolas', 9), wrap='none', bg='#12243b', fg='#cfeafb', insertbackground='white')
        view.pack(fill='both', expand=True, pady=9)
        def show():
            if not names.get():
                return
            p = Path(names.get())
            try:
                with p.open('rb') as fp:
                    fp.seek(max(0, p.stat().st_size - 220000))
                    data = fp.read().decode('utf-8', 'replace')
                view.delete('1.0', 'end')
                view.insert('end', data)
                view.see('end')
            except OSError as exc:
                messagebox.showerror('Logs', str(exc), parent=win)
        names.bind('<<ComboboxSelected>>', lambda _e: show())
        if logs:
            names.current(0)
            show()
        else:
            view.insert('end', 'Sin logs. Inicia un juego y regresa a esta pantalla.')
        ttk.Button(box, text='Actualizar vista', command=show).pack(anchor='e')

    def system_dialog(self):
        win, box = self.new_window(self.tr('system'), '690x360')
        ttk.Label(box, text='Sistema Dreamcast · v1.2', font=('Segoe UI', 15, 'bold')).pack(anchor='w', pady=9)
        ttk.Label(box, text='La versión estable utiliza sus rutinas de boot HLE y la configuración persistente.\nNo se requiere que el launcher modifique o distribuya una BIOS original.').pack(anchor='w', pady=11)
        ttk.Button(box, text='Abrir archivos de sistema', command=lambda: open_folder(system_directory())).pack(anchor='w', pady=6)
        ttk.Button(box, text='Abrir caché por juego', command=lambda: open_folder(cache_directory() / 'games')).pack(anchor='w', pady=6)
        ttk.Button(box, text='Acerca de la versión', command=self.help_dialog).pack(anchor='w', pady=6)

    def language_dialog(self):
        win, box = self.new_window(self.tr('language'), '560x270')
        ttk.Label(box, text='Idioma del panel de control', font=('Segoe UI', 15, 'bold')).pack(anchor='w', pady=11)
        var = tk.StringVar(value=self.lang)
        ttk.Radiobutton(box, text='Español', variable=var, value='es').pack(anchor='w', pady=5)
        ttk.Radiobutton(box, text='English', variable=var, value='en').pack(anchor='w', pady=5)
        ttk.Label(box, text='Los nombres y botones principales cambian de idioma al guardar.').pack(anchor='w', pady=8)
        def save():
            self.settings['language'] = var.get()
            self.lang = var.get()
            self.scene.draw()
            self.persist()
            win.destroy()
        ttk.Button(box, text=self.tr('save'), command=save).pack(anchor='e')

    def theme_dialog(self):
        win, box = self.new_window(self.tr('themes'), '600x280')
        ttk.Label(box, text='Temas del panel de ajustes', font=('Segoe UI', 15, 'bold')).pack(anchor='w', pady=11)
        var = tk.StringVar(value=self.settings['theme'])
        ttk.Radiobutton(box, text='Classic Aqua', variable=var, value='classic').pack(anchor='w', pady=6)
        ttk.Radiobutton(box, text='Aqua Night', variable=var, value='night').pack(anchor='w', pady=6)
        ttk.Radiobutton(box, text='Dream Night (Starry)', variable=var, value='starry').pack(anchor='w', pady=6)
        ttk.Label(box, text='Ahora podés probar 3 temas: Classic Aqua, Aqua Night y Dream Night con cielo estrellado.').pack(anchor='w', pady=8)
        def save():
            self.settings['theme'] = var.get()
            self.persist()
            self._windows_theme()
            self.scene.apply_theme()
        ttk.Button(box, text=self.tr('save'), command=save).pack(anchor='e')

    def help_dialog(self):
        self.info('DreamcastRecompiled V1.2 Stable Release',
            'DreamcastRecompiled V1.2 Stable Release · DCRV1.2.\n\n'
            'El launcher está desarrollado en Python/Tkinter y es independiente del runtime.\n'
            'Jugar: reutiliza el guest ABI1 sin recompilar SH-4.\n'
            'Preparar SH-4: solo con confirmación explícita.\n\n'
            'Atajos: Ctrl+O (agregar CDI/ISO), Enter (jugar), F1 (ayuda).')


def main():
    app = tk.Tk()
    def callback_error(exc_type, exc, tb):
        details = ''.join(traceback.format_exception(exc_type, exc, tb))
        try:
            folder = config_directory()
            folder.mkdir(parents=True, exist_ok=True)
            with (folder / 'launcher_errors.log').open('a', encoding='utf-8') as fp:
                fp.write(details + '\n')
        except OSError:
            pass
        messagebox.showerror('Dreamcast Recompiled · Launcher', details[-2000:], parent=app)
    app.report_callback_exception = callback_error
    Launcher(app)
    app.mainloop()


if __name__ == '__main__':
    main()
