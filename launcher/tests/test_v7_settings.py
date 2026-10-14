"""V7 configuration contract: staged options NEVER become cpp38e command args."""
from __future__ import annotations
import tempfile
import os
from pathlib import Path
import sys
import unittest
import tkinter as tk
from unittest.mock import patch

LAUNCHER=Path(__file__).resolve().parents[1]
if str(LAUNCHER) not in sys.path:sys.path.insert(0,str(LAUNCHER))
import core
from settings_v7 import SettingsPanel, export_configuration, import_configuration, TAB_NAMES

class V7ConfigContractTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)
        (self.root/'scripts'/'stable').mkdir(parents=True)
        (self.root/'scripts'/'stable'/'TEST_CPP38E_FAST.bat').write_text('@echo off\n')
        (self.root/'scripts'/'windows').mkdir(parents=True,exist_ok=True)
        (self.root/'scripts'/'windows'/'run_game.bat').write_text('@echo off\n')
        self.game=self.root/'Disc game.cdi'
        self.game.write_bytes(b'dummy')

    def test_history_sanitized_and_reset_preserved(self):
        raw={'history':[{'game':'/fake/a.cdi','when':'2026-10-09T20:00:00','mode':'play'},
                        {'game':42,'when':'bad','mode':'prepare'}]}
        clean=core.sanitize_settings(raw)
        self.assertEqual(len(clean['history']),1)
        self.assertEqual(core.reset_to_stable(clean)['history'],clean['history'])

    def test_video_defaults_and_invalid_inputs(self):
        bad={'resolution':'8k','fullscreen':'TRUE','vsync':13,'aspect':'32:9','filter':'DLSS','fps_counter':True}
        result=core.sanitize_settings({'video_future':bad})
        expected=core.VIDEO_DEFAULTS.copy();expected['fps_counter']=True
        self.assertEqual(result['video_future'],expected)
        self.assertEqual(result['launcher_width'],'auto')
        self.assertEqual(core.sanitize_settings({'launcher_width':'900'})['launcher_width'],'auto')

    def test_settings_nested_are_independent(self):
        x=core.sanitize_settings({}); y=core.sanitize_settings({})
        x['video_future']['resolution']='1080p'
        self.assertEqual(y['video_future']['resolution'],'native')

    def test_future_preferences_do_not_invent_flags(self):
        s=core.sanitize_settings({'renderer':'dx11','video_future':{
            'resolution':'1080p','fullscreen':True,'vsync':False,
            'aspect':'16:9','filter':'nearest','fps_counter':True},
            'audio_future':{'volume':20,'latency_ms':250},
            'controller_future':{'rumble':False,'deadzone_pct':30}})
        args=core.build_launch_command(self.root,self.game,s)
        self.assertIn('--renderer=dx11',args)
        self.assertEqual(args[2:], ['--no-pause', '--renderer=dx11', '--heartbeat-off'])
        self.assertFalse(any(x.startswith('--resolution') or x.startswith('--vsync')
                             or x.startswith('--filter') for x in args))

    def test_roundtrip_export_import_and_strip_unknown(self):
        saved=core.sanitize_settings({'video_future':{'resolution':'1440p'},
                                       'launcher_width':'960','renderer':'software'})
        dst=self.root/'profile.json'
        export_configuration(saved,dst)
        imported=import_configuration(dst)
        self.assertEqual(imported,saved)
        import json
        raw=json.loads(dst.read_text(encoding='utf-8'))
        raw['settings']['unsupported_runtime_hack']=True
        dst.write_text(json.dumps(raw),encoding='utf8')
        imported=import_configuration(dst)
        self.assertNotIn('unsupported_runtime_hack',imported)

    def test_import_refuses_unknown_schema_and_other_files(self):
        p=self.root/'bad.json'
        p.write_text('{"format":"alien","schema":7,"settings":{}}',encoding='utf8')
        with self.assertRaises(ValueError):import_configuration(p)
        p.write_text('{"format":"Dreamcast Recompiled v1.2 launcher","schema":200,"settings":{}}',encoding='utf8')
        with self.assertRaises(ValueError):import_configuration(p)

    def test_reset_preserves_games_and_real_cache_state(self):
        s=core.sanitize_settings({'renderer':'software','perf':True,'audio':False,
            'video_future':{'resolution':'1080p'},
            'library':[str(self.game)],'last_game':str(self.game),
            'game_overrides':{str(self.game):{'renderer':'dx11','audio':'off'}}})
        reset=core.reset_to_stable(s)
        self.assertEqual(reset['renderer'],'auto')
        self.assertTrue(reset['audio'])
        self.assertFalse(reset['perf'])
        self.assertEqual(reset['video_future']['resolution'],'native')
        self.assertEqual(reset['library'],[str(self.game)])
        self.assertEqual(reset['game_overrides'],s['game_overrides'])

    def test_per_game_future_prefs_persist_without_argv_effect(self):
        game=str(self.game.resolve())
        s=core.sanitize_settings({'game_overrides':{game:{
            'renderer':'software','audio':'inherit',
            'video_future':{'resolution':'1080p','fullscreen':True}}}})
        self.assertEqual(s['game_overrides'][game]['video_future']['resolution'],'1080p')
        self.assertEqual(core.effective_game_settings(s,game)['video_future']['resolution'],'1080p')
        argv=core.build_launch_command(self.root,self.game,s)
        self.assertIn('--renderer=software',argv)
        self.assertFalse(any('1080' in a for a in argv))

@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32', 'requires desktop')
class V7GuiSmokeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from main import Launcher
        cls.root=tk.Tk()
        cls.root.geometry('1200x900')
        cls.app=Launcher(cls.root)
        cls.root.update()
    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()

    def tearDown(self):
        self.app.scene.close_inline()
        self.root.update()

    def test_all_seven_tabs_and_support_badges(self):
        ui=SettingsPanel(self.app,'Vídeo')
        self.root.update()
        self.assertEqual(len(ui.pages),7)
        self.assertEqual(ui.panel.index(ui.panel.select()),1)
        self.assertEqual(ui.videovars['resolution'].get(),'native')
        self.assertIn('renderer',ui.vars)

    def test_save_future_prefs_and_real_renderer(self):
        ui=SettingsPanel(self.app,'Vídeo')
        ui.vars['renderer'].set('software')
        ui.videovars['resolution'].set('1080p')
        ui.videovars['fullscreen'].set(True)
        with patch.object(self.app,'persist'):
            ui._save()
        self.assertEqual(self.app.settings['renderer'],'software')
        self.assertEqual(self.app.settings['video_future']['resolution'],'1080p')
        self.assertTrue(self.app.settings['video_future']['fullscreen'])
        self.app.settings=core.sanitize_settings({})

    def test_reset_video_tab_isolated_until_save(self):
        ui=SettingsPanel(self.app,'Vídeo')
        before=self.app.settings['renderer']
        ui.vars['renderer'].set('software')
        ui.videovars['resolution'].set('1440p')
        ui._reset_tab('Vídeo')
        self.assertEqual(ui.vars['renderer'].get(),'auto')
        self.assertEqual(ui.videovars['resolution'].get(),'native')
        self.assertEqual(self.app.settings['renderer'],before)
        ui.window.destroy()

    def test_video_audio_performance_quick_navigation(self):
        for tab in ('Video','Audio','Rendimiento','Debug'):
            ui=self.app.settings_dialog(tab)
            self.root.update()
            expected={'Video':'video','Audio':'audio','Rendimiento':'performance','Debug':'debug'}.get(tab,tab)
            self.assertEqual(ui.key,expected)
            self.assertIsNotNone(self.app.scene.page_frame)
            ui.win.destroy()

if __name__=='__main__': unittest.main()
