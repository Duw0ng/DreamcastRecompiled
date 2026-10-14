"""V8.5 single-window and modern controller-editor regression tests."""
from __future__ import annotations
import os
import sys
from pathlib import Path
import tempfile
import unittest
import tkinter as tk
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from main import Launcher
from controller_profiles import read_profile,write_profile,DEFAULTS
from core import video_output_environment

class ControllerMappingTests(unittest.TestCase):
    def test_visual_tester_range_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'profiles'/'controller_profile.ini'
            settings=read_profile(path)
            settings['backend']='ps4'
            settings['bindings']['ltrig']='range:z:0:32768'
            settings['bindings']['rtrig']='range:r:65535:0'
            settings['bindings']['joy_y']='axis:LY:invert'
            write_profile(path,settings)
            updated=read_profile(path)
            self.assertEqual(updated['backend'],'ps4')
            self.assertEqual(updated['bindings']['ltrig'],'range:z:0:32768')
            self.assertEqual(updated['bindings']['rtrig'],'range:r:65535:0')
            self.assertEqual(updated['bindings']['joy_y'],'axis:LY:invert')
            write_profile(path,updated)
            self.assertTrue(path.with_suffix('.ini.bak').is_file())

    def test_bad_physical_assignment_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'profile.ini'
            d=read_profile(path)
            d['bindings']['a']='abc\nshell=1'
            with self.assertRaises(ValueError):write_profile(path,d)
            self.assertFalse(path.exists())

    def test_defaults_are_native_logical_mappings(self):
        self.assertEqual(set(DEFAULTS),set(read_profile(Path('/nonexistent/dcr-test-controller.ini'))['bindings']))

@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32','Requires a GUI display')
class InlineGuiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory()
        cls.patch=patch.dict(os.environ,{'LOCALAPPDATA':cls.tmp.name})
        cls.patch.start()
        cls.root=tk.Tk();cls.app=Launcher(cls.root);cls.root.update()
    @classmethod
    def tearDownClass(cls):
        cls.root.destroy();cls.patch.stop();cls.tmp.cleanup()

    def tearDown(self):
        self.app.scene.close_inline();self.root.update()

    def test_major_sections_keep_header_and_clock_without_toplevels(self):
        for name in ('games','video','audio','performance','controllers','settings','vmu'):
            self.app.activate(name);self.root.update()
            self.assertIsNotNone(self.app.scene.page_frame)
            self.assertTrue(self.app.scene.page_frame.winfo_ismapped())
            self.assertEqual([x for x in self.root.winfo_children() if isinstance(x,tk.Toplevel)],[])
            self.assertEqual(self.app.scene.canvas.itemcget(self.app.scene.clock_time,'state'),'')
            self.app.scene.close_inline()

    def test_video_set_1080p_16x9_affects_host_environment(self):
        page=self.app.settings_dialog('Vídeo')
        page.vars[('video_future','resolution')].set('1080p')
        page.vars[('video_future','aspect')].set('16:9')
        self.app.settings['video_future']['resolution']='1080p'
        self.app.settings['video_future']['aspect']='16:9'
        self.app.persist()
        env=video_output_environment(self.app.settings,Path(self.tmp.name)/'test.cdi')
        self.assertEqual(env['DCR_VIDEO_RESOLUTION'],'1080p')
        self.assertEqual(env['DCR_VIDEO_ASPECT'],'16:9')
        self.assertNotIn('DCR_VSYNC',env)

    def test_click_back_restores_home_icons_and_advanced_toggle(self):
        scene=self.app.scene
        self.app.activate('settings');self.root.update()
        self.assertEqual(scene.canvas.itemcget(scene.button_items['play'],'state'),'hidden')
        scene.close_inline();self.root.update()
        self.assertEqual(scene.canvas.itemcget(scene.button_items['play'],'state'),'normal')
        before=scene.extended
        scene.toggle_advanced();scene.toggle_advanced()
        self.assertEqual(scene.extended,before)

    def test_controller_page_uses_profile_editor_and_visual_tester(self):
        from integrated_pages import ControllerMappingEditor
        self.app.activate('controllers');self.root.update()
        frame=self.app.scene.page_frame
        found=[]
        def walk(w):
            for child in w.winfo_children():
                if isinstance(child,tk.Button):found.append(child.cget('text'))
                walk(child)
        walk(frame)
        self.assertIn('Guardar asignaciones en el mando',found)
        self.assertIn('Abrir Visual Tester completo',found)
        self.assertFalse(any('Configurador original' in name for name in found))
