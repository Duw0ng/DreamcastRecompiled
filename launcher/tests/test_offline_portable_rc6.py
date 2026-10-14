"""Release packaging: external-source host and no accidental Internet installs."""
from __future__ import annotations
import ast
import json
from pathlib import Path
import sys
import unittest
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'launcher'))
import offline_host

class OfflinePackagingTests(unittest.TestCase):
    def test_source_host_uses_external_files_and_all_themes(self):
        self.assertEqual(offline_host.base_dir(), ROOT)
        self.assertEqual(offline_host.check_layout(ROOT), [])
        self.assertEqual(offline_host.HOST_ABI, 1)
        for width in (800,960,1152):
            for theme in ('water','water_night','water_starry'):
                self.assertTrue((ROOT / 'launcher' / 'assets' / f'{theme}_{width}.png').is_file())
    def test_no_auto_python_downloads_in_entrypoints(self):
        for filename in ('ABRIR_LAUNCHER.bat','COMPILAR_LAUNCHER.bat'):
            script=(ROOT / filename).read_text(encoding='utf-8').lower()
            self.assertNotIn('invoke-webrequest',script)
            self.assertNotIn('https://',script)
            self.assertNotIn('pip install',script)
        self.assertTrue((ROOT / 'scripts' / 'windows' / 'build_offline_host.ps1').is_file())
    def test_offline_host_does_not_bundle_gui_sources(self):
        source=(ROOT / 'launcher' / 'offline_host.py').read_text(encoding='utf-8')
        tree=ast.parse(source)
        frozen_imports={n.name for x in ast.walk(tree) if isinstance(x,ast.Import) for n in x.names}
        self.assertNotIn('main',frozen_imports)
        self.assertNotIn('aqua_ui',frozen_imports)
        self.assertNotIn('vmu_standalone',frozen_imports)
        self.assertIn('runpy', frozen_imports)
    def test_engine_layout(self):
        for path in ('scripts/windows/run_game.bat','src','runtime','safe_guests','profiles','tools/native/dcr_native_tools.exe'):
            self.assertTrue((ROOT/path).exists())

if __name__=='__main__':unittest.main()
