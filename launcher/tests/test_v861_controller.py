"""V8.6.1 controller tester regressions, without Windows hardware required."""
from __future__ import annotations
import os
import sys
import tempfile
from pathlib import Path
import unittest
import tkinter as tk
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from controller_tester import PhysicalPad, binding_name, binding_summary, mapped_state
from controller_profiles import read_profile, write_profile
from core import sanitize_settings

class ControllerMappingV861Tests(unittest.TestCase):
    def setUp(self):
        self.profile=read_profile(Path('/nonexistent/controller-v861-profile.ini'))

    def test_sticks_2d_xinput_Y_is_vertical_and_up_positive(self):
        up=PhysicalPad('xinput',0,axes={'lx':12000,'ly':22000,'rx':-16000,'ry':-24000})
        state=mapped_state(up,self.profile)
        self.assertGreater(state['sticks'][0][0],.35)
        self.assertGreater(state['sticks'][0][1],.65)
        self.assertLess(state['sticks'][1][0],-.45)
        self.assertLess(state['sticks'][1][1],-.65)

    def test_sticks_2d_winmm_Y_is_vertical_and_up_positive(self):
        up=PhysicalPad('winmm',0,axes={'x':12000,'y':-23000,'z':-14000,'r':20000})
        state=mapped_state(up,self.profile)
        self.assertGreater(state['sticks'][0][1],.65)
        self.assertLess(state['sticks'][1][1],-.5)

    def test_profile_custom_button_mapping_controls_highlight(self):
        self.profile['bindings']['a']='button:2'
        xinput=PhysicalPad('xinput',0,buttons=0x4000)
        state=mapped_state(xinput,self.profile)
        self.assertTrue(state['buttons']['a'])
        self.assertTrue(state['buttons']['x'])
        self.assertEqual(binding_name('button:2'),'Botón físico 3')
        self.assertIn('A: Botón físico 3',binding_summary(self.profile))

    def test_profile_custom_axes_override_default(self):
        self.profile['bindings']['joy_y']='axis:ry'
        pad=PhysicalPad('xinput',1,axes={'ry':-20000,'ly':16000})
        self.assertGreater(mapped_state(pad,self.profile)['sticks'][0][1],.58)

    def test_triggers_L_and_R_from_profile(self):
        pad=PhysicalPad('xinput',0,lt=240,rt=85)
        state=mapped_state(pad,self.profile)
        self.assertGreater(state['triggers'][0],.9)
        self.assertGreater(state['triggers'][1],.3)

    def test_range_trigger_calibration(self):
        self.profile['bindings']['ltrig']='range:z:-32767:32767'
        pad=PhysicalPad('winmm',0,axes={'z':0})
        self.assertAlmostEqual(mapped_state(pad,self.profile)['triggers'][0],.5,delta=.02)

    def test_player_index_remains_persistent_after_sanitize(self):
        settings=sanitize_settings({'controller_future':{'player_index':3,'deadzone_pct':18}})
        self.assertEqual(settings['controller_future']['player_index'],3)
        settings=sanitize_settings({'controller_future':{'player_index':77}})
        self.assertEqual(settings['controller_future']['player_index'],1)

    def test_player_strict_selection_does_not_fall_back_to_another_port(self):
        from controller_visual import ControllerPreview
        from unittest.mock import Mock
        preview=ControllerPreview.__new__(ControllerPreview)
        preview.player=Mock()
        preview.player.get.return_value=3
        preview.profile={'backend':'auto'}
        preview._device_for_player=Mock(return_value=('auto',2))
        preview._xinput_state=Mock(return_value=None)
        preview._winmm_state=Mock(return_value=None)
        self.assertIsNone(preview._detect())
        preview._xinput_state.assert_called_once_with(2)
        preview._winmm_state.assert_called_once_with(2)

    def test_profile_reader_and_write_preserve_bindings(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'controller_profile.ini'
            self.profile['bindings']['ltrig']='range:z:0:32767'
            write_profile(p,self.profile)
            other=read_profile(p)
            self.assertEqual(other['bindings']['ltrig'],'range:z:0:32767')

@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform=='win32','Graphical display required')
class InlineControllerV861GuiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from main import Launcher
        cls.temp=tempfile.TemporaryDirectory()
        cls.env=patch.dict(os.environ,{'LOCALAPPDATA':cls.temp.name})
        cls.env.start()
        cls.root=tk.Tk()
        cls.app=Launcher(cls.root)
        cls.app.activate('controllers')
        cls.root.update()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy();cls.env.stop();cls.temp.cleanup()

    def test_controller_graphic_uses_actual_image_and_full_xy_panels(self):
        from controller_visual import ControllerPreview
        self.assertTrue(self.app.scene.page_frame.winfo_ismapped())
        self.assertTrue((Path(__file__).resolve().parents[1]/'assets'/'controller_v861_preview.png').is_file())
        self.assertEqual(len(self.root.winfo_children()),1)

    def test_only_requested_port_is_read(self):
        from controller_visual import ControllerPreview
        from integrated_pages import InlineContent
        page=InlineContent(self.app,'controllers') if False else None
        # Locate the visible preview widget inside page body via its player radiobuttons.
        radios=[]
        def collect(w):
            for child in w.winfo_children():
                if isinstance(child,tk.Radiobutton):radios.append(child)
                collect(child)
        collect(self.app.scene.page_frame)
        self.assertEqual(len(radios),4)
        self.assertEqual([int(r.cget('value')) for r in radios],[1,2,3,4])

if __name__=='__main__':unittest.main()
