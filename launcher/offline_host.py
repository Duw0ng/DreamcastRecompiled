"""Stable, reusable PyInstaller host for external Aqua Python/Tkinter sources.

Only build this Windows .exe once per Python runtime update. Future releases
can update launcher/*.py and launcher/assets/* without recompiling the host.
This code does not download software and does not spawn a native Aqua UI.
"""
from __future__ import annotations

# PyInstaller needs to see the standard-library dependencies of the external
# source modules at build time. Do not import or freeze Aqua modules here:
# they must always be loaded from <bundle>/launcher on disk.
import configparser
import copy
import ctypes
import dataclasses
import datetime
import functools
import hashlib
import importlib
import io
import json
import math
import os
import pathlib
import re
import runpy
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import time
import tkinter
from tkinter import ttk, filedialog, messagebox
import traceback
if sys.platform == 'win32':
    import winsound

from pathlib import Path

HOST_ABI = 1

def base_dir() -> Path:
    if getattr(sys, 'frozen', False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parent.parent

def check_layout(root: Path) -> list[str]:
    expected = [
        'launcher/launch.py',
        'launcher/main.py',
        'launcher/aqua_ui.py',
        'launcher/vmu_standalone.py',
        'launcher/assets/water_1152.png',
        'launcher/assets/water_night_1152.png',
        'launcher/assets/water_starry_1152.png',
        'scripts/windows/run_game.bat',
    ]
    return [entry for entry in expected if not (root / entry).is_file()]

def main():
    root = base_dir()
    missing = check_layout(root)
    if '--self-check' in sys.argv:
        result = {'host_abi':HOST_ABI,'python':sys.version.split()[0],
                  'tk':tkinter.TkVersion,'missing':missing}
        (root / '.offline_check.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        if missing:
            raise SystemExit(2)
        return
    if missing:
        details = '\n'.join(missing)
        try:
            alert = tkinter.Tk(); alert.withdraw()
            messagebox.showerror('AQUA portable incompleto',
                                 'Faltan archivos. Extraé el ZIP completo.\n\n'+details)
            alert.destroy()
        except Exception:
            print(details, file=sys.stderr)
        raise SystemExit(2)
    launcher = root / 'launcher'
    sys.path.insert(0,str(launcher))
    os.chdir(root)
    if len(sys.argv) >= 2 and sys.argv[1] == '--auto-game':
        # Separate subprocess: disc probing and native execution cannot freeze Tk.
        from portable_games import main as portable_game_main
        raise SystemExit(portable_game_main())
    runpy.run_path(str(launcher / 'launch.py'),run_name='__main__')

if __name__ == '__main__':
    main()
