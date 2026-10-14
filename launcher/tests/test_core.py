from pathlib import Path
import importlib.util
import json
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('dcrlauncher_core', Path(__file__).resolve().parents[1]/'core.py')
core = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(core)

class LauncherTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)/'Dreamcast Recompiled v1.2'
        (self.root/'scripts/stable').mkdir(parents=True)
        (self.root/'scripts/stable/TEST_CPP38E_FAST.bat').write_bytes(b'@echo off\r\n')
        (self.root/'scripts'/'windows').mkdir(parents=True,exist_ok=True)
        (self.root/'scripts'/'windows'/'run_game.bat').write_bytes(b'@echo off\r\n')
        self.disc=Path(self.tmp.name)/'Mi juego con espacios.cdi'
        self.disc.write_bytes(b'DUMMY_ONLY')

    def test_defaults_safe_and_disk_formats(self):
        opts=core.sanitize_settings({'renderer':'dx12','perf':'True','audio':False,'library':'bad'})
        self.assertEqual(opts['renderer'], 'auto')
        self.assertEqual(opts['perf'], False)
        self.assertEqual(opts['audio'], False)
        self.assertEqual(opts['library'], [])
        self.assertEqual(core.SUPPORTED_SUFFIXES,{'.cdi','.iso'})

    def test_persistent_and_corrupt_json(self):
        path=Path(self.tmp.name)/'test folder'/'settings.json'
        opts=core.load_settings(path)
        opts['renderer']='software';opts['library']=[str(self.disc)]
        core.save_settings(opts,path)
        self.assertEqual(core.load_settings(path)['renderer'],'software')
        self.assertEqual(core.load_settings(path)['library'],[str(self.disc)])
        path.write_text('{broken',encoding='utf8')
        self.assertEqual(core.load_settings(path)['renderer'],'auto')

    def test_library_deduplicate(self):
        opts=core.sanitize_settings({})
        self.assertEqual(core.add_to_library(opts,[self.disc,self.disc]),1)
        self.assertEqual(core.add_to_library(opts,[self.disc]),0)
        self.assertEqual(len(opts['library']),1)

    def test_stable_argv_preserves_guest(self):
        opts=core.sanitize_settings({})
        argv=core.build_launch_command(self.root,self.disc,opts)
        self.assertEqual(Path(argv[0]).name,'run_game.bat')
        self.assertEqual(argv[1],str(self.disc))
        self.assertEqual(argv[2:], ['--no-pause', '--renderer=auto', '--heartbeat-off'])
        self.assertNotIn('--perf',argv)

    def test_explicit_prepare_and_opt_in_flags(self):
        opts=core.sanitize_settings({'renderer':'software','audio':False,'perf':True,
            'hot_recovery':False,'spg_fast_forward':False})
        argv=core.build_launch_command(self.root,self.disc,opts,prepare=True)
        self.assertEqual(Path(argv[0]).name,'run_game.bat')
        for flag in ['--renderer=software','--no-audio','--perf','--spg-ff-off','--hot-recovery-off']:
            self.assertIn(flag,argv)
        self.assertNotIn('--reuse',argv)

    def test_per_game_override(self):
        opts=core.sanitize_settings({'renderer':'dx11','game_overrides': {
            str(self.disc): {'renderer':'software','audio':'off'}}})
        argv=core.build_launch_command(self.root,self.disc,opts)
        self.assertIn('--renderer=software',argv)
        self.assertIn('--no-audio',argv)

    def test_missing_disc_rejected(self):
        with self.assertRaises(ValueError):
            core.build_launch_command(self.root,Path(self.tmp.name)/'missing.cdi',core.sanitize_settings({}))

if __name__=='__main__':
    unittest.main()
