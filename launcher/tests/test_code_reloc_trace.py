"""The SGGG copied-IRQ helper trace is explicit opt-in and the mapping is safe."""
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'launcher'))
import core


class CodeRelocationDebugTests(unittest.TestCase):
    def test_trace_defaults_off_and_validated(self):
        self.assertFalse(core.sanitize_settings({})['code_reloc_trace'])
        self.assertFalse(core.sanitize_settings({'code_reloc_trace': 'true'})['code_reloc_trace'])
        self.assertTrue(core.sanitize_settings({'code_reloc_trace': True})['code_reloc_trace'])
        self.assertFalse(core.reset_to_stable({'code_reloc_trace': True})['code_reloc_trace'])

    def test_trace_persists(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'config.json'
            options = core.sanitize_settings({'code_reloc_trace': True})
            core.save_settings(options, path)
            self.assertTrue(core.load_settings(path)['code_reloc_trace'])

    def test_command_line_only_when_requested(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root/'scripts/stable').mkdir(parents=True)
            (root/'scripts/stable/TEST_CPP38E_FAST.bat').write_bytes(b'@echo off\r\n')
            (root/'scripts'/'windows').mkdir(parents=True,exist_ok=True)
            (root/'scripts'/'windows'/'run_game.bat').write_bytes(b'@echo off\r\n')
            disc=root/'sggg.cdi';disc.write_bytes(b'fake')
            for prepare in (True, False):
                self.assertNotIn('--code-reloc-trace', core.build_launch_command(
                    root, disc, core.sanitize_settings({}), prepare=prepare))
                self.assertIn('--code-reloc-trace', core.build_launch_command(
                    root, disc, core.sanitize_settings({'code_reloc_trace': True}), prepare=prepare))

    def test_batch_resets_trace_environment(self):
        bat=(ROOT/'scripts'/'windows'/'run_game.bat').read_text(encoding='utf-8')
        self.assertIn('set "CODE_RELOC_TRACE=0"',bat)
        self.assertIn('set "DCR_CODE_RELOC_TRACE=0"',bat)
        self.assertIn('if "!CODE_RELOC_TRACE!"=="1" set "DCR_CODE_RELOC_TRACE=1"',bat)
        self.assertIn('"--code-reloc-trace"',bat)

    def test_both_runtime_paths_have_validated_32byte_cache(self):
        for relative in ('runtime/dc_runtime.cpp','src/codegen/cpp_emitter.cpp'):
            with self.subTest(path=relative):
                source=(ROOT/relative).read_text(encoding='utf-8')
                self.assertIn('g_relocated_signature_cache',source)
                self.assertIn('memo.erase(saved);',source)
                self.assertIn('saved->second.target_signature.data()',source)
                self.assertIn('return remember_match(delta_match);',source)
                self.assertIn('return remember_match(match);',source)
                for prefix in ('code-rebind', 'code-reloc'):
                    pos=source.index('"[DCR '+prefix+'] 0x"')
                    self.assertIn('if (new_alias && dc_code_relocation_trace_enabled())', source[pos-140:pos])

    def test_diagnostics_page_offers_trace(self):
        source=(ROOT/'launcher/integrated_pages.py').read_text(encoding='utf-8')
        self.assertIn("page.check('Traza de reubicación de código SH-4','code_reloc_trace')",source)

if __name__ == '__main__':
    unittest.main()
