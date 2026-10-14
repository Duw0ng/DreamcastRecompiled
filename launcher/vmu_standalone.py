"""Aqua VMU: separate, keyboard/mouse-controlled Dreamcast-shaped window.

Standalone menu/pet/minigames are original companion software. LCD mode reads
only real Maple .lcd frames when available; buttons are NOT injected into games.
"""
from __future__ import annotations
import json
import os
from pathlib import Path
import shutil
import sys
import tkinter as tk
from tkinter import filedialog,messagebox

from core import config_directory, cache_directory, system_directory
from vmu_inspector import find_vmus
from vmu_lcd import read_frame, lcd_path, bitmap_rows
from vmu_companion import PetStore, VMUShell

HERE=Path(__file__).resolve().parent
ASSETS=HERE/'assets'
VMU_WINDOW_TITLE='DCRV1.2 · Aqua VMU'

def bring_existing_to_front() -> bool:
    """Bring an existing VMU to the foreground via Windows, without duplicates.

    The VMU process may have its own Tcl interpreter and cannot share Tk state.
    """
    if sys.platform!='win32':
        return False
    try:
        import ctypes
        from ctypes import wintypes
        user32=ctypes.windll.user32
        user32.FindWindowW.argtypes=(wintypes.LPCWSTR,wintypes.LPCWSTR)
        user32.FindWindowW.restype=wintypes.HWND
        hwnd=user32.FindWindowW(None,VMU_WINDOW_TITLE)
        if not hwnd:
            return False
        user32.ShowWindow.argtypes=(wintypes.HWND,ctypes.c_int)
        user32.BringWindowToTop.argtypes=(wintypes.HWND,)
        user32.SetForegroundWindow.argtypes=(wintypes.HWND,)
        user32.ShowWindow(hwnd,9)  # SW_RESTORE
        user32.BringWindowToTop(hwnd)
        user32.SetForegroundWindow(hwnd)
        return True
    except (AttributeError,OSError,ValueError):
        return False



def independent_command(selected_image: str | None = None) -> list[str]:
    """Launch same Python Aqua exe as a second independent process."""
    if getattr(sys,'frozen',False):
        command=[sys.executable,'--vmu']
    else:
        command=[sys.executable,str(HERE/'launch.py'),'--vmu']
        if sys.platform=='win32':
            hidden=Path(sys.executable).with_name('pythonw.exe')
            if hidden.is_file():command[0]=str(hidden)
    if selected_image:command+=['--vmu-image',str(selected_image)]
    return command


def start_independent(selected_image: str | None = None):
    # Clicking 'VMU' again focuses the same window instead of spawning duplicates.
    if bring_existing_to_front():
        return None
    import subprocess
    kwargs={'cwd':str(HERE.parent)}
    if sys.platform=='win32':
        kwargs['creationflags']=getattr(subprocess,'CREATE_NEW_PROCESS_GROUP',0)
    return subprocess.Popen(independent_command(selected_image),**kwargs)


class StandaloneVMU:
    W=416;H=620
    COMPACT_W=300;COMPACT_H=230
    CONTROLS=(('UP',(102,418,151,455)),('DOWN',(101,498,150,536)),
              ('LEFT',(64,447,102,498)),('RIGHT',(150,448,190,498)),
              ('A',(215,448,287,518)),('B',(294,448,362,510)),
              ('SLEEP',(228,400,275,445)),('MODE',(297,398,351,448)))

    def __init__(self,root:tk.Tk,initial_image:str|None=None):
        self.root=root
        config=config_directory();config.mkdir(parents=True,exist_ok=True)
        self.prefs_path=config/'vmu_window.json'
        self.store=PetStore(config/'aqua_pet.json')
        self.shell=VMUShell(self.store)
        self.prefs=self._prefs()
        self.skin=self.prefs['skin']
        self.scale=self.prefs['scale']
        self.lcd_mode=self.prefs['lcd_mode']
        self.compact=self.prefs['compact']
        self.flip_x=self.prefs['flip_x'];self.flip_y=self.prefs['flip_y']
        self.always_on_top=self.prefs['topmost']
        self.image_path:Path|None=None
        self._frame_key=None;self._drag=None;self._job=None
        self.shell_image=None;self.pixel_source=None;self.pixel_zoomed=None
        self.root.title(VMU_WINDOW_TITLE)
        if sys.platform=='win32':
            try:self.root.iconbitmap(str(ASSETS/'DreamcastRecompiled.ico'))
            except tk.TclError:pass
        self.root.configure(bg='#ff00ff' if sys.platform=='win32' else '#1c2f49')
        if sys.platform=='win32':
            # This is a shaped window, not a rectangular blue Tk panel.
            try:self.root.wm_attributes('-transparentcolor','#ff00ff')
            except tk.TclError:pass
            self.root.overrideredirect(True)
        self.root.attributes('-topmost',self.always_on_top)
        self.root.geometry(f'{int(self.W*self.scale)}x{int(self.H*self.scale)}+180+100')
        self.canvas=tk.Canvas(self.root,width=int(self.W*self.scale),height=int(self.H*self.scale),
                              bg='#ff00ff' if sys.platform=='win32' else '#1c2f49',
                              bd=0,highlightthickness=0)
        self.canvas.pack()
        self.img_id=self.canvas.create_image(0,0,anchor='nw')
        self.pixel_id=self.canvas.create_image(0,0,anchor='nw')
        self.close_id=self.canvas.create_text(0,0,text='×',fill='#71889c',font=('Segoe UI',15,'bold'))
        self.status_id=self.canvas.create_text(0,0,text='',fill='#3c4f5e',font=('Segoe UI',8),anchor='center')
        self._build_menu()
        self.canvas.bind('<Button-1>',self._click)
        self.canvas.bind('<B1-Motion>',self._move)
        self.canvas.bind('<ButtonRelease-1>',lambda _e:setattr(self,'_drag',None))
        self.canvas.bind('<Button-3>',self._popup)
        self.root.bind('<KeyPress>',self._key)
        self.root.protocol('WM_DELETE_WINDOW',self.close)
        self._set_scale(self.scale)
        entries=find_vmus(system_directory(),cache_directory()/'games')
        target=initial_image or self.prefs.get('image')
        if target and Path(target).is_file():self._set_image(Path(target),save=False)
        elif entries:self._set_image(entries[0],save=False)
        # The window is deliberately borderless, but must still be selectable
        # from Windows' taskbar/Alt+Tab like an ordinary app window.
        if sys.platform=='win32':
            self.root.after(150,self._register_windows_taskbar)
        self._tick()

    def _register_windows_taskbar(self):
        """Expose the shaped Tk root as a regular Windows application window.

        Tk's overrideredirect(True) marks its native wrapper as a tool window,
        which hides it from the taskbar. Removing WS_EX_TOOLWINDOW and adding
        WS_EX_APPWINDOW restores access without removing the VMU-shaped skin.
        """
        if sys.platform!='win32':
            return
        try:
            import ctypes
            from ctypes import wintypes
            user32=ctypes.windll.user32
            user32.GetParent.argtypes=(wintypes.HWND,)
            user32.GetParent.restype=wintypes.HWND
            user32.GetWindowLongW.argtypes=(wintypes.HWND,ctypes.c_int)
            user32.GetWindowLongW.restype=ctypes.c_long
            user32.SetWindowLongW.argtypes=(wintypes.HWND,ctypes.c_int,ctypes.c_long)
            user32.SetWindowLongW.restype=ctypes.c_long
            hwnd=user32.GetParent(self.root.winfo_id()) or self.root.winfo_id()
            GWL_EXSTYLE=-20; WS_EX_TOOLWINDOW=0x00000080; WS_EX_APPWINDOW=0x00040000
            current=user32.GetWindowLongW(hwnd,GWL_EXSTYLE)
            desired=(current & ~WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW
            if current!=desired:
                user32.SetWindowLongW(hwnd,GWL_EXSTYLE,desired)
                self.root.withdraw()
                self.root.after(50,self._restore_taskbar_window)
        except (OSError,AttributeError,tk.TclError):
            # Even if the shell rejects the taskbar style, topmost is still on.
            pass

    def _restore_taskbar_window(self):
        try:
            self.root.deiconify()
            self.root.lift()
            self.root.attributes('-topmost',self.always_on_top)
        except tk.TclError:
            pass

    def _prefs(self):
        defaults={'skin':'white','scale':1.0,'topmost':True,'window_ui_v2':True,'flip_x':False,'flip_y':True,
                  'lcd_mode':'auto','image':'','compact':False}
        try:
            if self.prefs_path.stat().st_size<8192:
                loaded=json.loads(self.prefs_path.read_text(encoding='utf-8'))
                if isinstance(loaded,dict):
                    defaults.update({k:v for k,v in loaded.items() if k in defaults})
                    # One-time migration from older borderless, unselectable VMU:
                    # bring it to the front by default. Later opt-outs persist.
                    if loaded.get('window_ui_v2') is not True:
                        defaults['topmost']=True
                        defaults['window_ui_v2']=True
        except (OSError,ValueError,TypeError):pass
        if defaults['skin'] not in ('white','sapphire'):defaults['skin']='white'
        if defaults['lcd_mode'] not in ('auto','shell','game'):defaults['lcd_mode']='auto'
        if defaults['scale'] not in (0.75,1.0,1.25):defaults['scale']=1.0
        for key in ('topmost','flip_x','flip_y','compact','window_ui_v2'):
            if type(defaults[key]) is not bool:defaults[key]=False
        return defaults

    def _save_prefs(self):
        p={'skin':self.skin,'scale':self.scale,'topmost':self.always_on_top,
           'window_ui_v2':True,
           'flip_x':self.flip_x,'flip_y':self.flip_y,'lcd_mode':self.lcd_mode,
           'image':str(self.image_path) if self.image_path else '','compact':self.compact}
        try:
            from tempfile import NamedTemporaryFile
            self.prefs_path.parent.mkdir(parents=True,exist_ok=True)
            with NamedTemporaryFile(mode='w',encoding='utf-8',delete=False,
                                   dir=self.prefs_path.parent,suffix='.tmp') as f:
                json.dump(p,f,indent=2);tmp=Path(f.name)
            os.replace(tmp,self.prefs_path)
        except OSError:pass

    def _set_image(self,path:Path,save=True):
        if not path.is_file() or path.suffix.lower() not in ('.bin','.vmu') or path.stat().st_size!=128*1024:
            return
        self.image_path=path
        self.shell.set_vmu(path)
        self._frame_key=None
        if save:self._save_prefs()

    def _choose_image(self):
        value=filedialog.askopenfilename(parent=self.root,title='Seleccionar memoria VMU',
                 filetypes=[('Imagen VMU','*.bin *.vmu')])
        if value:
            if Path(value).stat().st_size!=128*1024:
                messagebox.showerror('Aqua VMU','La memoria debe tener 128 KiB.',parent=self.root)
                return
            self._set_image(Path(value))

    def _backup(self):
        if not self.image_path or not self.image_path.is_file():
            messagebox.showinfo('Aqua VMU','Seleccioná primero una VMU.',parent=self.root);return
        target=filedialog.asksaveasfilename(parent=self.root,defaultextension='.bin',
                            initialfile=self.image_path.stem+'_backup.bin',
                            filetypes=[('Imagen VMU','*.bin')])
        if target:
            if Path(target).resolve()==self.image_path.resolve():
                messagebox.showwarning('Aqua VMU','No se puede sobrescribir la VMU activa.',parent=self.root)
                return
            try:shutil.copy2(self.image_path,target)
            except OSError as e:messagebox.showerror('Aqua VMU',str(e),parent=self.root)

    def _menu_variable(self,name,val):
        if name=='skin':self.skin=val;self._set_scale(self.scale)
        elif name=='scale':self._set_scale(val)
        elif name=='lcd_mode':self.lcd_mode=val
        elif name=='topmost':self.always_on_top=val;self.root.attributes('-topmost',val)
        elif name=='flip_x':self.flip_x=val
        elif name=='flip_y':self.flip_y=val
        elif name=='compact':self.compact=val;self._set_scale(self.scale)
        self._frame_key=None
        self._save_prefs()

    def _build_menu(self):
        menu=tk.Menu(self.root,tearoff=False)
        skin=tk.Menu(menu,tearoff=False)
        for label,val in [('Blanco original','white'),('Azul transparente','sapphire')]:
            skin.add_command(label=label,command=lambda v=val:self._menu_variable('skin',v))
        menu.add_cascade(label='Carcasa',menu=skin)
        modes=tk.Menu(menu,tearoff=False)
        for label,val in [('Automático (juego si hay señal)','auto'),('VMU autónoma','shell'),
                          ('LCD del juego','game')]:
            modes.add_command(label=label,command=lambda v=val:self._menu_variable('lcd_mode',v))
        menu.add_cascade(label='Pantalla',menu=modes)
        zoom=tk.Menu(menu,tearoff=False)
        for val in (0.75,1.0,1.25):zoom.add_command(label=f'{int(val*100)} %',command=lambda v=val:self._menu_variable('scale',v))
        menu.add_cascade(label='Tamaño',menu=zoom)
        menu.add_command(label='Modo LCD compacto: activar/desactivar',
                         command=lambda:self._menu_variable('compact',not self.compact))
        menu.add_command(label='Siempre visible: activar/desactivar',
                         command=lambda:self._menu_variable('topmost',not self.always_on_top))
        menu.add_command(label='Voltear X',command=lambda:self._menu_variable('flip_x',not self.flip_x))
        menu.add_command(label='Voltear Y',command=lambda:self._menu_variable('flip_y',not self.flip_y))
        menu.add_separator()
        slots=tk.Menu(menu,tearoff=False)
        menu.add_cascade(label='Memorias detectadas',menu=slots)
        self.slots_menu=slots
        menu.add_command(label='Elegir VMU .bin / .vmu',command=self._choose_image)
        menu.add_command(label='Respaldar memoria .bin (solo copia)',command=self._backup)
        menu.add_command(label='Actividad / AquaPet',command=self._stats)
        menu.add_command(label='Controles y ayuda',command=self._help)
        menu.add_command(label='Traer al frente',command=lambda:(self.root.deiconify(),self.root.lift(),self.root.focus_force()))
        menu.add_separator()
        menu.add_command(label='Cerrar VMU',command=self.close)
        self.menu=menu

    def _stats(self):
        pet=self.store.pet
        image=self.image_path.name if self.image_path else 'ninguna'
        messagebox.showinfo('AquaPet',
             f'Nivel {pet["level"]} · XP {pet["xp"]}\n'
             f'Alimento {pet["full"]}% · Alegría {pet["happy"]}%\n'
             f'Higiene {pet["clean"]}% · Energía {pet["energy"]}%\n'
             f'Lanzamientos registrados: {pet["launches"]}\n'
             f'Objetos: {", ".join(pet["items"]) or "ninguno"}\n\n'
             f'VMU: {image}\n'
             'Los progresos del juego no se leen ni modifican.',parent=self.root)

    def _help(self):
        messagebox.showinfo('Controles de VMU',
             'Flechas: cruceta   Z/Enter: A   X/Backspace: B\n'
             'S: Sleep (encender/apagar)   M: Mode (menú)\n'
             'Clic derecho: carcasa, tamaño, slot, visor y backups.\n'
             'Podés arrastrar la carcasa para moverla.\n'
             'En Windows, accedé también desde la barra de tareas.\n'
             'Por defecto permanece siempre visible; clic derecho para cambiarlo.\n\n'
             'Los minijuegos y AquaPet son apps propias de AQUA, NO\n'
             'ejecución de programas binarios de la VMU original.\n'
             'Los botones de esta ventana NO envían comandos al juego.',parent=self.root)

    def _set_scale(self,val):
        self.scale=val
        suffix={0.75:'75',1.0:'100',1.25:'125'}[val]
        basename='vmu_compact' if self.compact else 'vmu_shell'
        name=ASSETS/f'{basename}_{self.skin}_{suffix}.png'
        self.shell_image=tk.PhotoImage(file=str(name))
        w=self.COMPACT_W if self.compact else self.W
        h=self.COMPACT_H if self.compact else self.H
        self.canvas.config(width=int(w*val),height=int(h*val))
        self.root.geometry(f'{int(w*val)}x{int(h*val)}')
        self.canvas.itemconfigure(self.img_id,image=self.shell_image)
        self.canvas.coords(self.close_id,(283 if self.compact else 371)*val,(17 if self.compact else 66)*val)
        self.canvas.coords(self.status_id,(150 if self.compact else 208)*val,(216 if self.compact else 558)*val)
        self.canvas.itemconfigure(self.status_id,font=('Segoe UI',max(6,int(8*val))))
        self.canvas.coords(self.pixel_id,(30 if self.compact else 88)*val,(42 if self.compact else 182)*val)
        self._frame_key=None
        self._save_prefs()
        self._paint(bytes(192))

    def _popup(self,event):
        self.slots_menu.delete(0,'end')
        entries=find_vmus(system_directory(),cache_directory()/'games')
        if not entries:self.slots_menu.add_command(label='No hay VMU detectadas',state='disabled')
        for path in entries[:40]:
            selected='✓ ' if path==self.image_path else ''
            self.slots_menu.add_command(label=selected+path.name[:40],
                                         command=lambda p=path:self._set_image(p))
        self.menu.tk_popup(event.x_root,event.y_root)
        self.menu.grab_release()

    def _click(self,event):
        x=event.x/self.scale;y=event.y/self.scale
        if self.compact:
            if 267<x<300 and 0<y<33:self.close();return
            if 25<=x<=275 and 34<=y<=205:
                self.lcd_mode='shell';self._frame_key=None;self._save_prefs();self._redraw()
                return
            self._drag=(event.x_root,event.y_root,self.root.winfo_x(),self.root.winfo_y())
            return
        if 352<x<391 and 49<y<88:self.close();return
        for key,(x0,y0,x1,y1) in self.CONTROLS:
            if x0<=x<=x1 and y0<=y<=y1:
                self.shell.press(key)
                if self.lcd_mode=='game':self.lcd_mode='shell';self._save_prefs()
                self._frame_key=None;self._redraw();return
        if 86<=x<=331 and 178<=y<=347:
            # Click LCD to switch to the autonomous app when watching game frames.
            self.lcd_mode='shell';self._frame_key=None;self._save_prefs();self._redraw();return
        self._drag=(event.x_root,event.y_root,self.root.winfo_x(),self.root.winfo_y())

    def _move(self,event):
        if self._drag:
            ox,oy,wx,wy=self._drag
            self.root.geometry(f'+{wx+event.x_root-ox}+{wy+event.y_root-oy}')

    def _key(self,event):
        key=event.keysym.lower()
        selected={'up':'UP','down':'DOWN','left':'LEFT','right':'RIGHT',
                  'z':'A','return':'A','space':'A','x':'B','backspace':'B',
                  's':'SLEEP','m':'MODE','escape':'B'}.get(key)
        if key=='tab':
            self._menu_variable('compact',not self.compact);return
        if selected:
            self.shell.press(selected)
            if self.lcd_mode=='game':self.lcd_mode='shell';self._save_prefs()
            self._frame_key=None;self._redraw()

    def _paint(self,bitmap:bytes):
        if len(bitmap)!=192:return
        rows=bitmap_rows(bitmap,flip_x=self.flip_x,flip_y=self.flip_y)
        source=tk.PhotoImage(width=48,height=32)
        off='#a3c490' if self.skin=='white' else '#9fc8be'
        on='#314a36' if self.skin=='white' else '#234e58'
        for y,row in enumerate(rows):
            source.put('{'+' '.join(on if bit else off for bit in row)+'}',to=(0,y))
        # Integer-friendly resampling: keep 48x32 nearest-neighbor pixels.
        numerator,denominator={0.75:(15,4),1.0:(5,1),1.25:(25,4)}[self.scale]
        shown=source.zoom(numerator,numerator)
        if denominator>1:shown=shown.subsample(denominator,denominator)
        self.canvas.itemconfigure(self.pixel_id,image=shown)
        self.pixel_source=source;self.pixel_zoomed=shown

    def _redraw(self):
        target=lcd_path(self.image_path) if self.image_path else None
        frame=read_frame(target) if target else None
        recent=frame is not None and frame.age_seconds()<4
        show_game=frame is not None and (self.lcd_mode=='game' or (self.lcd_mode=='auto' and recent))
        bitmap=frame.bitmap if show_game else self.shell.render()
        signature=(bitmap,self.flip_x,self.flip_y,self.skin,self.scale)
        if signature!=self._frame_key:
            self._paint(bitmap);self._frame_key=signature
        state=('LCD REAL' if recent else 'LCD ANTERIOR') if show_game else (
              'ENCENDIDA' if self.shell.power else 'DORMIDA')
        if not self.image_path:state+=' · SIN VMU'
        elif '_a1' in self.image_path.stem.lower():state+=' · A1'
        else:state+=' · '+self.image_path.stem[-9:].upper()
        self.canvas.itemconfigure(self.status_id,text=state,
                   fill='#d6eafa' if self.skin=='sapphire' else '#4b6578')

    def _tick(self):
        try:
            self.shell.tick()
            if self.shell.frame_count%18==1:
                self.store.track_launcher_history(config_directory()/'settings.json')
            self._redraw()
            self._job=self.root.after(250,self._tick)
        except tk.TclError:
            self._job=None

    def close(self):
        if self._job:
            try:self.root.after_cancel(self._job)
            except tk.TclError:pass
        self._save_prefs()
        self.root.destroy()


def main(initial_image: str | None = None):
    root=tk.Tk()
    app=StandaloneVMU(root,initial_image)
    root.mainloop()
    return app

if __name__=='__main__':main()
