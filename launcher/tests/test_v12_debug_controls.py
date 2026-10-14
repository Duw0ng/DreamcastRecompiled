"""Smoke contract: both terminal & heartbeat toggles truly affect runtime launch."""
from pathlib import Path
from unittest import TestCase
from unittest.mock import patch, MagicMock
import tempfile
import os
import sys
import tkinter as tk
from tkinter import ttk
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import core
import main

ROOT=Path(__file__).resolve().parents[2]

class DebugSettingsContracts(TestCase):
    def test_default_off_and_restore_and_persistence(self):
        settings=core.sanitize_settings({})
        self.assertFalse(settings['show_console'])
        self.assertFalse(settings['heartbeats'])
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'user_settings.json'
            settings['show_console']=True
            settings['heartbeats']=True
            core.save_settings(settings,p)
            loaded=core.load_settings(p)
            self.assertTrue(loaded['show_console'])
            self.assertTrue(loaded['heartbeats'])
            self.assertFalse(core.reset_to_stable(loaded)['show_console'])
            self.assertFalse(core.reset_to_stable(loaded)['heartbeats'])

    def test_invalid_types_fail_safe(self):
        settings=core.sanitize_settings({'show_console':'yes','heartbeats':1})
        self.assertFalse(settings['show_console'])
        self.assertFalse(settings['heartbeats'])

    def test_heartbeat_appends_flag_only_when_enabled(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);(root/'scripts/stable').mkdir(parents=True)
            (root/'scripts/stable/TEST_CPP38E_FAST.bat').write_text('@echo off')
            (root/'scripts'/'windows').mkdir(parents=True,exist_ok=True)
            (root/'scripts'/'windows'/'run_game.bat').write_text('@echo off')
            game=root/'chuchu.cdi';game.write_bytes(b'CDI')
            default = core.build_launch_command(root,game,core.sanitize_settings({}))
            self.assertIn('--heartbeat-off', default)
            self.assertNotIn('--heartbeat-on', default)
            settings=core.sanitize_settings({'heartbeats':True})
            self.assertIn('--heartbeat-on',core.build_launch_command(root,game,settings))
            self.assertIn('--heartbeat-on',core.build_launch_command(root,game,settings,prepare=True))
            settings['show_console']=True
            self.assertIn('--heartbeat-on',core.build_launch_command(root,game,settings))
            settings['heartbeats']=False
            self.assertNotIn('--heartbeat-on',core.build_launch_command(root,game,settings))

    def test_batch_sets_zero_heartbeats_independently_of_debug(self):
        text=(ROOT/'scripts'/'windows'/'run_game.bat').read_text(encoding='utf-8-sig')
        self.assertIn('set "HEARTBEAT=0"',text)
        self.assertIn('if "!HEARTBEAT_ENABLED!"=="1" set "HEARTBEAT=1000"',text)
        self.assertIn('"--heartbeat-on"',text)
        self.assertIn('"--heartbeat-off"',text)
        self.assertNotIn('set "HEARTBEAT=500"',text)
        self.assertLess(text.index('!EXTRA_RUN! ^\n  --diag-heartbeat-ms=!HEARTBEAT!'),text.index('set "RC=!ERRORLEVEL!"'))

    def test_gui_has_independent_controls(self):
        text=(ROOT/'launcher/integrated_pages.py').read_text()
        self.assertIn("page.check('Mostrar consola externa al jugar','show_console')",text)
        self.assertIn("page.check('Heartbeats periódicos (1 s)','heartbeats')",text)

@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32', 'requires desktop')
class DebugLauncherGuiContracts(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory()
        cls.env=patch.dict(os.environ,{'LOCALAPPDATA':cls.tmp.name})
        cls.env.start()
        cls.root=tk.Tk()
        cls.app=main.Launcher(cls.root)
    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        cls.env.stop()
        cls.tmp.cleanup()

    def tearDown(self):
        self.app.scene.close_inline()

    def test_debug_page_both_controls_persist(self):
        self.app.activate('debug')
        self.root.update()
        from core import load_settings
        self.assertIsNotNone(self.app.scene.page_frame)
        # Regression: the Debug icon must show the actual toggles, not only logs.
        check_labels = []
        def find_checks(widget):
            for child in widget.winfo_children():
                if isinstance(child, (tk.Checkbutton, ttk.Checkbutton)):
                    check_labels.append(child.cget('text'))
                find_checks(child)
        find_checks(self.app.scene.page_frame)
        self.assertIn('Mostrar consola externa al jugar', check_labels)
        self.assertIn('Heartbeats periódicos (1 s)', check_labels)
        self.assertFalse(self.app.settings['show_console'])
        self.assertFalse(self.app.settings['heartbeats'])
        self.app.settings['show_console']=True
        self.app.settings['heartbeats']=True
        self.app.persist()
        self.assertTrue(load_settings(self.app.state_path)['show_console'])
        self.assertTrue(load_settings(self.app.state_path)['heartbeats'])
        self.app.settings['show_console']=False
        self.app.settings['heartbeats']=False
        self.app.persist()

    def test_hidden_and_visible_creationflags_and_capture(self):
        from pathlib import Path
        with tempfile.TemporaryDirectory() as td:
            game=Path(td)/'chuchu.cdi'
            game.write_bytes(b'placeholder')
            self.app.add_games([str(game)])
            self.app.select_game(str(game))
            self.app.settings['heartbeats']=False
            mockproc=MagicMock(pid=12345, poll=lambda:0, returncode=0)
            with patch.object(main,'ensure_windows',return_value=True),\
                 patch.object(main,'build_launch_command',return_value=['runner.bat',str(game)]),\
                 patch.object(self.app.root,'after',return_value=None),\
                 patch.object(main.messagebox,'showerror'),\
                 patch.object(main.subprocess,'Popen',return_value=mockproc) as popen:
                self.app.settings['show_console']=False
                self.app.play()
                kwargs=popen.call_args.kwargs
                self.assertEqual(kwargs['creationflags'],getattr(main.subprocess,'CREATE_NO_WINDOW',0x08000000))
                self.assertEqual(kwargs['stderr'],main.subprocess.STDOUT)
                self.assertIsNotNone(kwargs['stdout'])
                self.app.settings['show_console']=True
                self.app.play()
                kwargs=popen.call_args.kwargs
                self.assertEqual(kwargs['creationflags'],getattr(main.subprocess,'CREATE_NEW_CONSOLE',0x10))
                self.assertNotIn('stdout',kwargs)
