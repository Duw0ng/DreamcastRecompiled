"""Regression tests: the library must be VISIBLE, not just saved in JSON.

V8 created a ttk.Treeview as a sibling of the panel that visually covered it.
The model test passed while Windows users saw an empty rectangle.
"""
from __future__ import annotations

import os
from pathlib import Path
import sys
import tempfile
import tkinter as tk
from tkinter import ttk
import unittest
from unittest.mock import patch

BASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(BASE))
from main import Launcher
from core import load_settings


def descendants(widget, cls):
    return [child for direct in widget.winfo_children()
            for child in ([direct] if isinstance(direct, cls) else []) + descendants(direct, cls)]


@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform == 'win32', 'GUI display required')
class VisibleLibraryTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.env = patch.dict(os.environ, {'LOCALAPPDATA': self.tmp.name})
        self.env.start()
        self.addCleanup(self.env.stop)
        self.disc = Path(self.tmp.name) / 'ChuChu Rocket!.cdi'
        self.disc.write_bytes(b'fixture only')
        self.root = tk.Tk()
        self.addCleanup(self.root.destroy)
        self.app = Launcher(self.root)
        self.root.update()

    def open_library(self):
        self.app.games_dialog()
        self.root.update()
        windows = [w for w in self.root.winfo_children() if isinstance(w, tk.Toplevel)]
        self.assertEqual(windows, [], 'Library should not open a separate window')
        win = self.app.scene.page_frame
        self.assertIsNotNone(win)
        trees = descendants(win, ttk.Treeview)
        self.assertEqual(len(trees), 1)
        tree = trees[0]
        return win, tree

    def test_existing_saved_game_has_visible_table_rows(self):
        self.app.add_games([self.disc])
        win, tree = self.open_library()
        # The key regression: the Treeview is created INSIDE its packed panel,
        # not as a sibling later moved with pack(in_=...) and covered by it.
        self.assertIsInstance(tree.master, ttk.Frame)
        self.assertNotEqual(tree.master, win.winfo_children()[0])
        self.assertTrue(tree.winfo_ismapped())
        self.assertGreater(tree.winfo_height(), 100)
        self.assertEqual(len(tree.get_children()), 1)
        row = tree.item(tree.get_children()[0])['values']
        self.assertIn('ChuChu Rocket!', str(row[0]))
        self.assertIn('Disponible', str(row[2]))
        count_labels = [str(x.cget('text')) for x in descendants(win, ttk.Label)]
        self.assertTrue(any('1 juego(s)' in x for x in count_labels))

    def test_add_button_immediately_updates_visible_row_and_file(self):
        win, tree = self.open_library()
        self.assertEqual(len(tree.get_children()), 0)
        buttons = descendants(win, ttk.Button)
        add = next(b for b in buttons if b.cget('text') == 'Agregar CDI/ISO')
        with patch('main.filedialog.askopenfilename', return_value=str(self.disc)):
            add.invoke()
        self.root.update()
        self.assertEqual(len(tree.get_children()), 1)
        self.assertEqual(len(load_settings()['library']), 1)
        self.assertEqual(tree.selection()[0], tree.get_children()[0])

    def test_existing_file_moved_still_shown_as_missing(self):
        self.app.add_games([self.disc])
        self.disc.unlink()
        win, tree = self.open_library()
        self.assertEqual(len(tree.get_children()), 1)
        self.assertEqual(tree.item(tree.get_children()[0])['values'][2], 'No encontrado')
        self.assertIn(str(self.disc), load_settings()['library'])

    def test_add_and_select_does_not_prepare_or_recompile(self):
        win, tree = self.open_library()
        add = next(b for b in descendants(win, ttk.Button) if b.cget('text') == 'Agregar CDI/ISO')
        with patch('main.filedialog.askopenfilename', return_value=str(self.disc)), \
             patch.object(self.app, 'play') as play:
            add.invoke()
            self.root.update()
            self.assertEqual(len(tree.get_children()), 1)
            self.assertEqual(self.app.settings['last_game'], str(self.disc.resolve()))
            play.assert_not_called()


if __name__ == '__main__':
    unittest.main()
