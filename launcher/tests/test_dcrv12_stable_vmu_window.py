"""Stable branding and standalone VMU visibility regression checks."""
from __future__ import annotations
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import vmu_standalone as module

class VMUVisibilityTests(unittest.TestCase):
    def test_stable_window_name(self):
        self.assertIn('DCRV1.2', module.VMU_WINDOW_TITLE)

    def test_reuse_existing_vmu_without_second_process(self):
        with patch.object(module,'bring_existing_to_front',return_value=True), \
             patch('subprocess.Popen') as spawn:
            self.assertIsNone(module.start_independent())
            spawn.assert_not_called()

    def test_new_vmu_process_still_supported(self):
        with patch.object(module,'bring_existing_to_front',return_value=False), \
             patch('subprocess.Popen') as spawn:
            module.start_independent('C:/test/a1.bin')
            args,kwargs=spawn.call_args
            self.assertIn('--vmu',args[0])
            self.assertIn('C:/test/a1.bin',args[0])

    def test_existing_topmost_preference_is_migrated(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'vmu_window.json'
            path.write_text(json.dumps({'topmost':False,'skin':'sapphire'}))
            obj=module.StandaloneVMU.__new__(module.StandaloneVMU)
            obj.prefs_path=path
            result=obj._prefs()
            self.assertTrue(result['topmost'])
            self.assertTrue(result['window_ui_v2'])
            self.assertEqual(result['skin'],'sapphire')

    def test_user_can_opt_out_after_migration(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'vmu_window.json'
            path.write_text(json.dumps({'topmost':False,'window_ui_v2':True}))
            obj=module.StandaloneVMU.__new__(module.StandaloneVMU)
            obj.prefs_path=path
            result=obj._prefs()
            self.assertFalse(result['topmost'])

    def test_stable_guest_identity_unchanged(self):
        from portable_games import EXPECTED_RUNTIME
        assert EXPECTED_RUNTIME=='cpp38e-v8.6-sh4-dynamic-ram-aot-v1'

if __name__=='__main__':unittest.main()
