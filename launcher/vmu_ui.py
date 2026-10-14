"""Inline VMU management/diagnostics page; safe, read-only inspection."""
from __future__ import annotations
import datetime
from pathlib import Path
import shutil
import tkinter as tk
from tkinter import filedialog,ttk,messagebox
from core import system_directory,cache_directory
from vmu_inspector import find_vmus,read_vmu


def open_vmu_page(app):
    app._page_key='vmu'
    host,outer=app.new_window('VMU · Administrador de memorias')
    head=ttk.Frame(outer);head.pack(fill='x',pady=(0,5))
    ttk.Label(head,text='Memorias virtuales',font=('Segoe UI',16,'bold')).pack(side='left')
    ttk.Label(head,text='Diagnóstico de solo lectura · 128 KiB',font=('Segoe UI',9)).pack(side='right')
    path_var=tk.StringVar()
    files=find_vmus(system_directory(),cache_directory()/'games')
    options={str(p):p for p in files}
    if files:path_var.set(str(files[0]))
    toolbar=ttk.Frame(outer);toolbar.pack(fill='x',pady=4)
    picker=ttk.Combobox(toolbar,textvariable=path_var,values=list(options),state='readonly',width=76)
    picker.pack(side='left',fill='x',expand=True,padx=(0,5))
    status=tk.StringVar(value='Elegí una VMU para inspeccionar.')
    summary=tk.StringVar(value='')
    ttk.Label(outer,textvariable=summary,font=('Segoe UI',10,'bold')).pack(anchor='w',pady=4)
    center=ttk.Frame(outer);center.pack(fill='both',expand=True,pady=4)
    cols=('nombre','tipo','bloques','estado')
    frame=ttk.Frame(center);frame.pack(side='left',fill='both',expand=True)
    tree=ttk.Treeview(frame,show='headings',columns=cols,height=11)
    for c,title,w in zip(cols,('Archivo','Tipo','Bloques','FAT'),(160,65,63,130)):
        tree.heading(c,text=title);tree.column(c,width=w,stretch=c in ('nombre','estado'))
    tree.pack(side='left',fill='both',expand=True)
    bar=ttk.Scrollbar(frame,orient='vertical',command=tree.yview);bar.pack(side='right',fill='y')
    tree.configure(yscrollcommand=bar.set)
    ttk.Label(outer,text='La pantalla LCD y las aplicaciones de la VMU ahora viven en una ventana independiente.\n'
                          'Abrila con «Abrir VMU independiente», incluso sin un juego abierto.',
              wraplength=730).pack(anchor='w',pady=(3,4))
    ttk.Label(outer,textvariable=status,wraplength=720).pack(anchor='w',pady=(5,2))

    def inspect():
        value=path_var.get().strip()
        tree.delete(*tree.get_children())
        if not value:return
        try: r=read_vmu(value)
        except (OSError,ValueError) as exc:
            status.set(f'Error leyendo VMU: {exc}');summary.set('');return
        summary.set(f'{r.filename}  ·  {r.used_blocks}/{r.capacity_blocks} bloques ocupados  ·  '
                    f'{r.free_blocks} libres  ·  {len(r.entries)} archivos')
        for i,entry in enumerate(r.entries):
            tree.insert('','end',iid=str(i),values=(entry.filename,entry.kind,entry.blocks,entry.chain_status))
        status.set(('Formato reconocido.' if r.formatted else 'Formato no verificado.')+
                   (' Avisos: '+'; '.join(r.warnings[:3]) if r.warnings else ' FAT revisada.'))

    def choose():
        p=filedialog.askopenfilename(parent=app.root,title='Abrir imagen VMU (solo lectura)',
               filetypes=[('VMU binaria','*.bin *.vmu'),('Todos los archivos','*.*')])
        if not p:return
        path_var.set(p)
        if p not in options:
            options[p]=Path(p);picker.configure(values=list(options))
        inspect()

    def backup():
        src=Path(path_var.get())
        if not src.is_file():
            messagebox.showwarning('VMU','Primero elegí una memoria existente.',parent=app.root);return
        target=filedialog.asksaveasfilename(parent=app.root,title='Guardar respaldo',
                   defaultextension='.bin',initialfile=src.stem+'_backup_'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S')+'.bin',
                   filetypes=[('Imagen VMU','*.bin')])
        if not target:return
        try:
            if src.resolve()==Path(target).resolve():raise ValueError('Elegí una ruta distinta al archivo original.')
            shutil.copy2(src,target)
            status.set(f'Copia de seguridad creada: {target}')
        except (OSError,ValueError) as exc:messagebox.showerror('VMU',str(exc),parent=app.root)

    def details(_e=None):
        selection=tree.selection()
        if not selection:return
        try:
            r=read_vmu(path_var.get());e=r.entries[int(selection[0])]
            status.set(f'{e.filename} | {e.kind} | {e.blocks} bloques ({e.bytes_allocated} bytes) | '
                       f'bloque inicial {e.first_block} | protegido: {"Sí" if e.copy_protected else "No"} | FAT: {e.chain_status}')
        except (OSError,ValueError,IndexError):pass
    picker.bind('<<ComboboxSelected>>',lambda _e:inspect())
    tree.bind('<<TreeviewSelect>>',details)
    ttk.Button(toolbar,text='Buscar VMU',command=choose).pack(side='left',padx=2)
    foot=ttk.Frame(outer);foot.pack(fill='x',pady=5)
    ttk.Button(foot,text='Actualizar',command=inspect).pack(side='left',padx=3)
    ttk.Button(foot,text='Respaldar .bin',command=backup).pack(side='left',padx=3)
    def open_standalone():
        try:
            from vmu_standalone import start_independent
            start_independent(path_var.get().strip() or None)
        except OSError as exc:
            messagebox.showerror('Aqua VMU',f'No se pudo abrir la VMU: {exc}',parent=app.root)
    ttk.Button(foot,text='Abrir / mostrar VMU',command=open_standalone).pack(side='left',padx=3)
    ttk.Button(foot,text='Abrir carpeta',command=lambda:app.open_config_folder() if not path_var.get() else __import__('os').startfile(str(Path(path_var.get()).parent)) if __import__('sys').platform=='win32' else None).pack(side='left',padx=3)
    ttk.Label(foot,text='Sin escritura · sin formateo').pack(side='right')
    inspect()
