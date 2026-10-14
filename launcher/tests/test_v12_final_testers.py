"""Release tester calibration, physical-device selection and UI regressions."""
from __future__ import annotations
import os
import sys
import tempfile
import tkinter as tk
from pathlib import Path
from unittest import TestCase,skipUnless
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from core import sanitize_settings
from controller_tester import PhysicalPad,mapped_state
from controller_profiles import read_profile,write_profile
from controller_calibration import (choices_for_player,learn_axis,learn_trigger,learn_button,
                                    learn_binding,neutral_deadzone_percent)
from controller_visual import ControllerPreview

class DeviceAndCalibrationTests(TestCase):
    def test_per_player_devices_survive_sanitization_and_do_not_swap(self):
        raw={'controller_future':{'player_index':2,
             'player_devices':{'1':{'backend':'winmm','index':6},'2':{'backend':'xinput','index':3},
                               '3':{'backend':'winmm','index':12},'4':{'backend':'auto','index':2}}}}
        safe=sanitize_settings(raw)
        self.assertEqual(choices_for_player(safe['controller_future']['player_devices'],1),('winmm',6))
        self.assertEqual(choices_for_player(safe['controller_future']['player_devices'],2),('xinput',3))
        self.assertEqual(choices_for_player(safe['controller_future']['player_devices'],3),('winmm',12))
        self.assertEqual(choices_for_player(safe['controller_future']['player_devices'],4),('auto',2))

    def test_malformed_device_and_bindings_rejected_on_import(self):
        cfg=sanitize_settings({'controller_future':{
            'player_devices':{'1':{'backend':'malware','index':99},'2':{'backend':'xinput','index':-100}},
            'calibrations':{'1':{'bindings':{'joy_y':'axis:y:invert','a':'bad\r\nxxx=1'},'deadzone_pct':500},'9':{'bindings':{'b':'button:1'}}}}})['controller_future']
        self.assertEqual(cfg['player_devices'],{})
        self.assertEqual(cfg['calibrations']['1']['bindings'],{'joy_y':'axis:y:invert'})
        self.assertNotIn('9',cfg['calibrations'])

    def test_xinput_axis_calibration_positive_guest_down(self):
        neutral=PhysicalPad('xinput',0,axes={'lx':0,'ly':0,'rx':0,'ry':0})
        self.assertEqual(learn_axis(neutral,PhysicalPad('xinput',0,axes={'lx':22000}),'joy_x'),'axis:lx')
        self.assertEqual(learn_axis(neutral,PhysicalPad('xinput',0,axes={'ly':22000}),'joy_y'),'axis:ly:invert')
        self.assertEqual(learn_axis(neutral,PhysicalPad('xinput',0,axes={'rx':18000}),'joy2_x'),'axis:rx')
        self.assertEqual(learn_axis(neutral,PhysicalPad('xinput',0,axes={'ry':21000}),'joy2_y'),'axis:ry:invert')

    def test_winmm_axis_calibration(self):
        neutral=PhysicalPad('winmm',6,axes={'x':0,'y':0,'z':0,'r':0,'u':0,'v':0})
        self.assertEqual(learn_axis(neutral,PhysicalPad('winmm',6,axes={'y':-23000}),'joy_y'),'axis:y')
        self.assertEqual(learn_axis(neutral,PhysicalPad('winmm',6,axes={'r':-23000}),'joy2_y'),'axis:r')

    def test_winmm_combined_triggers_are_two_independent_ranges(self):
        neutral=PhysicalPad('winmm',6,axes={'z':0})
        left=PhysicalPad('winmm',6,axes={'z':-30000})
        right=PhysicalPad('winmm',6,axes={'z':29000})
        self.assertEqual(learn_trigger(neutral,left,'ltrig'),'range:z:0:-30000')
        self.assertEqual(learn_trigger(neutral,right,'rtrig'),'range:z:0:29000')
        profile=read_profile(Path('/nonexistent/controller.ini'))
        profile['bindings']['ltrig']=learn_trigger(neutral,left,'ltrig')
        profile['bindings']['rtrig']=learn_trigger(neutral,right,'rtrig')
        self.assertGreater(mapped_state(left,profile)['triggers'][0],.99)
        self.assertAlmostEqual(mapped_state(left,profile)['triggers'][1],0.0)
        self.assertAlmostEqual(mapped_state(right,profile)['triggers'][0],0.0)
        self.assertGreater(mapped_state(right,profile)['triggers'][1],.99)

    def test_xinput_triggers(self):
        neutral=PhysicalPad('xinput',0)
        self.assertEqual(learn_trigger(neutral,PhysicalPad('xinput',0,lt=200),'ltrig'),'trigger:lt')
        self.assertEqual(learn_trigger(neutral,PhysicalPad('xinput',0,rt=220),'rtrig'),'trigger:rt')

    def test_physical_button_and_pov_learning(self):
        neutral=PhysicalPad('winmm',0)
        self.assertEqual(learn_button(neutral,PhysicalPad('winmm',0,buttons=1<<5),'a'),'button:5')
        self.assertEqual(learn_button(neutral,PhysicalPad('winmm',0,pov=0),'up'),'pov:up')

    def test_wrong_pad_does_not_cross_device(self):
        with self.assertRaises(ValueError):learn_axis(PhysicalPad('xinput',0),PhysicalPad('xinput',1,axes={'lx':20000}),'joy_x')
        with self.assertRaises(ValueError):learn_trigger(PhysicalPad('xinput',0),PhysicalPad('winmm',0),'ltrig')

    def test_significant_motion_required(self):
        with self.assertRaises(ValueError):learn_axis(PhysicalPad('xinput',0),PhysicalPad('xinput',0,axes={'ly':240}),'joy_y')

    def test_read_profile_preserves_calibrated_ranges_and_restore(self):
        with tempfile.TemporaryDirectory() as d:
            profile=Path(d)/'controller_profile.ini'
            data=read_profile(profile)
            data['backend']='winmm';data['device']=6
            data['bindings']['ltrig']='range:z:0:-30000'
            data['bindings']['rtrig']='range:z:0:30000'
            write_profile(profile,data)
            write_profile(profile,data)
            self.assertTrue(profile.with_suffix('.ini.bak').is_file())
            self.assertEqual(read_profile(profile)['device'],6)
            self.assertEqual(read_profile(profile)['bindings']['ltrig'],'range:z:0:-30000')

    def test_neutral_center_drift(self):
        pad=PhysicalPad('winmm',0,axes={'x':900,'y':-2500,'z':0})
        self.assertGreaterEqual(neutral_deadzone_percent(pad),10)
        self.assertLessEqual(neutral_deadzone_percent(pad),50)

@skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32','Tk needs display')
class CalibrationGuiTests(TestCase):
    def test_widget_device_and_apply_to_game(self):
        from main import Launcher
        with tempfile.TemporaryDirectory() as d:
            with patch.dict(os.environ,{'LOCALAPPDATA':d}):
                root=tk.Tk()
                try:
                    app=Launcher(root);app.activate('controllers');root.update()
                    # Locate preview through patching constructor after mounting in app.
                    refs=[]
                    def walk(obj):
                        for w in obj.winfo_children():
                            if isinstance(w,ttk.Combobox):refs.append(w)
                            walk(w)
                    from tkinter import ttk
                    walk(app.scene.page_frame)
                    # Unified cpp36b-style chooser: keyboard+mouse is one item,
                    # and disconnected physical devices are hidden by default.
                    self.assertTrue(any('Teclado + Mouse' in str(c.cget('values')) for c in refs))
                    self.assertFalse(any('WinMM 6' in str(c.cget('values')) for c in refs))
                    self.assertEqual(len([c for c in root.winfo_children() if isinstance(c,tk.Toplevel)]),0)
                finally:root.destroy()

if __name__=='__main__':
    import unittest
    unittest.main()

@skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32','Tk needs display')
class CalibrationSaveGuiTests(TestCase):
    def test_save_remapped_axis_and_apply_explicitly_to_runtime(self):
        from main import Launcher
        from integrated_pages import InlineContent
        with tempfile.TemporaryDirectory() as d:
            with patch.dict(os.environ,{'LOCALAPPDATA':d}):
                root=tk.Tk()
                try:
                    app=Launcher(root)
                    page=InlineContent(app,'controllers')
                    monitor=ControllerPreview(page)
                    neutral=PhysicalPad('winmm',6,axes={'x':0,'y':0,'z':0,'r':0})
                    changing={'state':neutral}
                    with patch.object(monitor,'_xinput_state',return_value=None), \
                         patch.object(monitor,'_winmm_state',side_effect=lambda port:changing['state'] if port==6 else None):
                        monitor.mount()
                        monitor.profile_file=Path(d)/'game_controller.ini'
                        write_profile(monitor.profile_file,read_profile(Path(d)/'nonexistent.ini'))
                        monitor._reload_profile(force=True)
                        monitor._set_device('winmm',6)
                        monitor._begin_calibration()
                        changing['state']=PhysicalPad('winmm',6,axes={'x':21000,'y':0,'z':0,'r':0})
                        monitor._capture_calibration()
                        self.assertEqual(monitor._draft['joy_x'],'axis:x')
                        changing['state']=neutral
                        monitor._save_calibration()
                        self.assertEqual(app.settings['controller_future']['calibrations']['1']['backend'],'winmm')
                        self.assertEqual(monitor._active_profile()['bindings']['joy_x'],'axis:x')
                        monitor._apply_to_game()
                        written=read_profile(monitor.profile_file)
                        self.assertEqual(written['backend'],'winmm')
                        self.assertEqual(written['device'],6)
                        self.assertEqual(written['bindings']['joy_x'],'axis:x')
                        self.assertTrue(monitor.profile_file.with_suffix('.ini.bak').is_file())
                finally:root.destroy()
