"""Startup wrapper: make Tk launch failures visible even with pythonw.exe."""
from pathlib import Path
import sys
import traceback

try:
    if len(sys.argv) > 1 and sys.argv[1] == '--auto-game':
        # The prebuilt PyInstaller host loads this EXTERNAL script at runtime.
        # Keep this dispatcher here so testers do not have to rebuild the exe.
        from portable_games import main as precompiled_main
        raise SystemExit(precompiled_main())
    if '--vmu' in sys.argv:
        from vmu_standalone import main as vmu_main
        image=None
        if '--vmu-image' in sys.argv:
            index=sys.argv.index('--vmu-image')
            if index+1<len(sys.argv):image=sys.argv[index+1]
        vmu_main(image)
    else:
        from main import main
        main()
except Exception:
    err = traceback.format_exc()
    try:
        from core import config_directory
        target = config_directory()
        target.mkdir(parents=True, exist_ok=True)
        (target / 'launcher_errors.log').open('a', encoding='utf-8').write(err+'\n')
    except Exception:
        pass
    try:
        import tkinter as tk
        from tkinter import messagebox
        tkroot=tk.Tk()
        tkroot.withdraw()
        messagebox.showerror('DCRV1.2 Stable Release',
                             'Error al abrir la interfaz.\n\n'+err[-2200:])
        tkroot.destroy()
    except Exception:
        print(err, file=sys.stderr)
    sys.exit(1)
