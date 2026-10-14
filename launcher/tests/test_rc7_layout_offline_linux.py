"""RC7 regression tests for layout and offline Windows hand-off."""
from __future__ import annotations
import hashlib
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'launcher'))
import core
import offline_host

class RC7LayoutTests(unittest.TestCase):
    def test_only_two_root_batch_entrypoints(self):
        self.assertEqual({p.name for p in ROOT.glob('*.bat')}, {'ABRIR_LAUNCHER.bat', 'COMPILAR_LAUNCHER.bat'})
        self.assertTrue((ROOT/'scripts/windows/run_game.bat').is_file())
        runner = (ROOT/'scripts/windows/run_game.bat').read_text(encoding='utf-8-sig')
        self.assertIn('cd /d "%~dp0..\\.."', runner)
        self.assertIn('DCR_GUEST_ABI=1', runner)
    def test_python_launcher_follows_runner(self):
        self.assertEqual(offline_host.check_layout(ROOT), [])
        source = (ROOT/'launcher/core.py').read_text(encoding='utf-8')
        self.assertIn("root / 'scripts' / 'windows' / 'run_game.bat'", source)
    def test_probe_avoids_quoting_failure(self):
        builder = (ROOT/'scripts/windows/build_offline_host.ps1').read_text(encoding='utf-8')
        self.assertNotIn("-c 'import sys,tkinter;", builder)
        self.assertIn('& $python $probePath', builder)
        for artifact in ('python312.dll','_tkinter.pyd','tcl86t.dll','tk86t.dll'):
            self.assertIn(artifact,builder)
    def test_no_runtime_download_in_entrypoints(self):
        for name in ('ABRIR_LAUNCHER.bat','COMPILAR_LAUNCHER.bat'):
            source = (ROOT/name).read_text(encoding='utf-8').lower()
            self.assertNotIn('invoke-webrequest',source)
            self.assertNotIn('pip install',source)
        self.assertTrue((ROOT/'scripts/windows/package_offline_release.ps1').is_file())
        self.assertTrue((ROOT/'.github/workflows/linux-core-ci.yml').is_file())

if __name__=='__main__': unittest.main()
