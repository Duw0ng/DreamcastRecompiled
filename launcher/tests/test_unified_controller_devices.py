"""Regression checks for the classic cpp36b combined selector restored in v1.2."""
import sys
import unittest
from pathlib import Path
from unittest.mock import Mock
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from core import sanitize_settings,launcher_environment
from controller_calibration import choices_for_player
from controller_devices import named_choices,port_spec,port_environment
from controller_visual import ControllerPreview

class CombinedHostDevices(unittest.TestCase):
    def test_keyboard_mouse_is_one_option_with_connected_pads(self):
        labels=named_choices((1,3), ((2,'DualSense Wireless Controller'),))
        self.assertEqual([n for n,k,_ in labels].count('Teclado + Mouse'),1)
        self.assertTrue(any('DualSense Wireless Controller' in s for s,_,_ in labels))
        self.assertTrue(any(k=='xinput' and i==3 for _,k,i in labels))
        self.assertFalse(any('Sin señal' in label for label,_,_ in labels))
    def test_four_port_mapping_uses_original_maple_environment(self):
        cfg=sanitize_settings({'controller_future':{'player_devices':{
             '1':{'backend':'keyboard_mouse','index':0},
             '2':{'backend':'xinput','index':1},
             '3':{'backend':'winmm','index':5},
             '4':{'backend':'none','index':3}}}})
        result=port_environment(cfg)
        self.assertEqual(result,{'DCR_MAPLE_PORT_A_DEVICE':'keyboard_mouse',
                                 'DCR_MAPLE_PORT_B_DEVICE':'xinput:1',
                                 'DCR_MAPLE_PORT_C_DEVICE':'winmm:5',
                                 'DCR_MAPLE_PORT_D_DEVICE':'none'})
        game_env=launcher_environment(cfg,'/tmp/game.cdi',inherited={'DCR_VIDEO_ASPECT':'16:9'})
        self.assertEqual(game_env['DCR_MAPLE_PORT_A_DEVICE'],'keyboard_mouse')
        self.assertEqual(game_env['DCR_MAPLE_PORT_C_DEVICE'],'winmm:5')
        self.assertNotIn('DCR_VIDEO_ASPECT',game_env)
    def test_default_ports_match_maple_legacy(self):
        cfg=sanitize_settings({})
        self.assertEqual(port_environment(cfg),{
           'DCR_MAPLE_PORT_A_DEVICE':'auto',
           'DCR_MAPLE_PORT_B_DEVICE':'none',
           'DCR_MAPLE_PORT_C_DEVICE':'none',
           'DCR_MAPLE_PORT_D_DEVICE':'none'})
        for n in range(1,5):
            self.assertEqual(choices_for_player(cfg['controller_future']['player_devices'],n)[0],
                             'auto' if n==1 else 'none')
    def test_bad_spec_does_not_enter_runtime_env(self):
        with self.assertRaises(ValueError):port_spec('custom:evil',0)
        with self.assertRaises(ValueError):port_spec('xinput',15)
        self.assertEqual(port_environment({'controller_future':{'player_devices':{'1':{'backend':'INVALID','index':0}}}})['DCR_MAPLE_PORT_A_DEVICE'],'auto')
    def test_keyboard_and_mouse_readback_one_physical_panel(self):
        fake=ControllerPreview.__new__(ControllerPreview)
        down={ord('W'),ord('D'),ord('J'),0x01,ord('Q')}
        fake.user32=Mock()
        fake.user32.GetAsyncKeyState.side_effect=lambda k: -32768 if k in down else 0
        result=fake._keyboard_mouse_state()
        self.assertTrue(result['buttons']['a'])
        self.assertEqual(result['sticks'][0],(1.0,1.0))
        self.assertEqual(result['sticks'][1],(0,0))
        self.assertEqual(result['triggers'],(1.0,0.0))
    def test_keyboard_mouse_or_disconnected_does_not_poll_physical(self):
        fake=ControllerPreview.__new__(ControllerPreview)
        fake._device_for_player=Mock(return_value=('keyboard_mouse',0))
        fake._xinput_state=Mock()
        fake._winmm_state=Mock()
        self.assertIsNone(fake._detect())
        fake._device_for_player.return_value=('none',0)
        self.assertIsNone(fake._detect())
        fake._xinput_state.assert_not_called()
        fake._winmm_state.assert_not_called()
    def test_sanitizer_migrates_combined_virtual_devices(self):
        safe=sanitize_settings({'controller_future':{'player_devices':{'1':{'backend':'keyboard_mouse','index':0},'2':{'backend':'none','index':0}}}})
        self.assertEqual(safe['controller_future']['player_devices']['1']['backend'],'keyboard_mouse')
        self.assertEqual(safe['controller_future']['player_devices']['2']['backend'],'none')

if __name__=='__main__':unittest.main()
