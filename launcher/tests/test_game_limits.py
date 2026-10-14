"""Per-game SH-4 limits: preserve unrelated settings; request targeted rebuild."""
import sys
import os
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from game_limits import (RECOMPILE_MARKER, RECOMMENDED_SGGG,
                         read_limits, save_limits)


class GameLimitsTests(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.folder = Path(self.dir.name) / 'SGGG'
        self.folder.mkdir()
        self.cfg = self.folder / 'game.cfg'
        self.cfg.write_text('''# Local edits
profile_version=1
title=SGGG
renderer=software
max_functions=8192
max_closure_entries=6144
closure_passes=64
extra_run_args=--safe
''', encoding='utf-8')

    def test_read_historic_profile(self):
        self.assertEqual(read_limits(self.cfg),
                         {'max_functions':8192, 'max_closure_entries':6144, 'closure_passes':64})

    def test_save_sggg_targets_only_its_profile(self):
        self.assertTrue(save_limits(self.cfg, **RECOMMENDED_SGGG))
        self.assertEqual(read_limits(self.cfg), RECOMMENDED_SGGG)
        s = self.cfg.read_text(encoding='utf-8')
        self.assertIn('renderer=software', s)
        self.assertIn('extra_run_args=--safe', s)
        self.assertIn('# Local edits', s)
        self.assertIn('profile_version=1', s)
        self.assertTrue((self.folder / RECOMPILE_MARKER).is_file())
        self.assertFalse((self.folder.parent / RECOMPILE_MARKER).exists())

    def test_noop_does_not_recompile(self):
        self.assertFalse(save_limits(self.cfg, 8192, 6144, 64))
        self.assertFalse((self.folder / RECOMPILE_MARKER).exists())

    def test_invalid_values_keep_original_and_no_marker(self):
        old = self.cfg.read_bytes()
        cases = [('anything',6144,64), (8192,8192,64), (8192,20000,64),
                 (8192,6144,0), (0,0,64), ('32 & del C:',6144,64)]
        for fn, closure, passes in cases:
            with self.subTest(fn=fn, closure=closure, passes=passes):
                with self.assertRaises(ValueError):
                    save_limits(self.cfg, fn, closure, passes)
                self.assertEqual(self.cfg.read_bytes(), old)
                self.assertFalse((self.folder / RECOMPILE_MARKER).exists())

    def test_auto_closure_zero_is_supported(self):
        self.assertTrue(save_limits(self.cfg, 12288, 0, 96))
        self.assertEqual(read_limits(self.cfg)['max_closure_entries'], 0)

    def test_repeat_save_does_not_retouch_file(self):
        save_limits(self.cfg, **RECOMMENDED_SGGG)
        stamp = self.cfg.stat().st_mtime_ns
        self.assertFalse(save_limits(self.cfg, **RECOMMENDED_SGGG))
        self.assertEqual(stamp, self.cfg.stat().st_mtime_ns)


if __name__ == '__main__':
    unittest.main()

@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32', 'requires desktop')
class GuiLimitsSmokeTests(unittest.TestCase):
    def test_sggg_profile_editor_applies_recommended_values(self):
        import tkinter as tk
        from tkinter import ttk
        from unittest.mock import patch
        from main import Launcher
        with tempfile.TemporaryDirectory() as tmp:
            cfg = Path(tmp) / 'SGGG' / 'game.cfg'
            cfg.parent.mkdir()
            cfg.write_text('''profile_version=1\ntitle=SGGG\nrenderer=software\nmax_functions=8192\nmax_closure_entries=6144\nclosure_passes=64\n''',encoding='utf-8')
            root = tk.Tk()
            root.withdraw()
            try:
                with patch('main.list_game_profiles', return_value=[cfg]), \
                     patch('main.messagebox.showinfo'):
                    app = Launcher(root)
                    app.profiles_dialog()
                    root.update()
                    def descendants(widget):
                        for child in widget.winfo_children():
                            yield child
                            yield from descendants(child)
                    buttons = [w for w in descendants(root) if isinstance(w, ttk.Button)]
                    for label in ('Valores recomendados SGGG', 'Guardar límites'):
                        next(w for w in buttons if w.cget('text') == label).invoke()
                    self.assertEqual(read_limits(cfg), RECOMMENDED_SGGG)
                    self.assertTrue((cfg.parent / RECOMPILE_MARKER).is_file())
            finally:
                root.destroy()
