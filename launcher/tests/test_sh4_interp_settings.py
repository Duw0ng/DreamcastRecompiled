"""interp-fallback-v1: launcher wiring for the SH-4 interpreter fallback."""
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'launcher'))
import core


class Sh4InterpreterSettingsTests(unittest.TestCase):
    def test_defaults(self):
        settings = core.sanitize_settings({})
        self.assertTrue(settings['sh4_interp'])
        self.assertFalse(settings['sh4_interp_trace'])
        self.assertTrue(core.sanitize_settings({'sh4_interp': 'no'})['sh4_interp'])
        self.assertFalse(core.sanitize_settings({'sh4_interp': False})['sh4_interp'])
        self.assertTrue(core.reset_to_stable({'sh4_interp': False})['sh4_interp'])
        self.assertFalse(core.reset_to_stable({'sh4_interp_trace': True})['sh4_interp_trace'])

    def test_persists(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'config.json'
            core.save_settings(core.sanitize_settings({'sh4_interp': False, 'sh4_interp_trace': True}), path)
            loaded = core.load_settings(path)
            self.assertFalse(loaded['sh4_interp'])
            self.assertTrue(loaded['sh4_interp_trace'])

    def test_command_line(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'scripts/stable').mkdir(parents=True)
            (root / 'scripts/stable/TEST_CPP38E_FAST.bat').write_bytes(b'@echo off\r\n')
            (root / 'scripts' / 'windows').mkdir(parents=True,exist_ok=True)
            (root / 'scripts' / 'windows' / 'run_game.bat').write_bytes(b'@echo off\r\n')
            disc = root / 'game.cdi'
            disc.write_bytes(b'fake')
            for prepare in (True, False):
                default = core.build_launch_command(root, disc, core.sanitize_settings({}), prepare=prepare)
                self.assertNotIn('--no-interp', default)
                self.assertNotIn('--interp-trace', default)
                off = core.build_launch_command(root, disc, core.sanitize_settings({'sh4_interp': False}), prepare=prepare)
                self.assertIn('--no-interp', off)
                traced = core.build_launch_command(root, disc, core.sanitize_settings({'sh4_interp_trace': True}), prepare=prepare)
                self.assertIn('--interp-trace', traced)

    def test_batch_contract(self):
        bat = (ROOT / 'scripts' / 'windows' / 'run_game.bat').read_text(encoding='utf-8-sig')
        for needle in ('set "SH4_INTERP=1"', '"--no-interp"', '"--interp-trace"',
                       'set "DCR_SH4_INTERP=!SH4_INTERP!"',
                       'set "DCR_SH4_INTERP_SEEDS=!INTERP_SEED_FILE!"',
                       'set "INTERP_SEED_FILE=!DCR_GAME_DIR!\\interp_seeds.txt"',
                       'set "SEED_ARGS=!SEED_ARGS! --seed-file="!INTERP_SEED_FILE!""'):
            self.assertIn(needle, bat)
        # The learned seeds are only consumed while preparing SH-4 (recompile step).
        self.assertLess(bat.index(':recompile'), bat.index('--seed-file="!INTERP_SEED_FILE!"'))

    def test_runtime_routes_missing_targets_to_interpreter(self):
        cpp = (ROOT / 'runtime' / 'dc_runtime.cpp').read_text(encoding='utf-8')
        self.assertIn('function = &dc_sh4_interp_function;', cpp)
        self.assertIn('if (!dc_sh4_interp_accepts(runtime, requested_target))', cpp)
        self.assertIn('dc_sh4_interp_set_reference_image(runtime, external_base', cpp)
        cmake = (ROOT / 'runtime' / 'CMakeLists.txt').read_text(encoding='utf-8')
        self.assertIn('dc_sh4_interp.cpp', cmake)

    def test_diagnostics_pages_offer_controls(self):
        pages = (ROOT / 'launcher/integrated_pages.py').read_text(encoding='utf-8')
        self.assertIn("page.check('Intérprete SH-4 de respaldo','sh4_interp')", pages)
        self.assertIn("page.check('Traza del intérprete SH-4','sh4_interp_trace')", pages)
        classic = (ROOT / 'launcher/settings_v7.py').read_text(encoding='utf-8')
        self.assertIn("('sh4_interp','Intérprete temporal para targets faltantes')", classic)


if __name__ == '__main__':
    unittest.main()
