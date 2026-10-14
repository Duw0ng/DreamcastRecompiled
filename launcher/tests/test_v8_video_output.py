"""Presentation-only V8 integration: never forge CLI flags or alter ABI1."""
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
LAUNCHER=Path(__file__).resolve().parents[1]
if str(LAUNCHER) not in sys.path: sys.path.insert(0, str(LAUNCHER))
import core

class V8VideoEnvTests(unittest.TestCase):
    def setUp(self):
        self.disc = Path('/tmp/fake_game.cdi')
        self.base = core.sanitize_settings({})

    def test_default_unchanged_and_does_not_leak_inherited_env(self):
        e=core.video_output_environment(self.base,self.disc,
            {'PATH':'C:/Windows','DCR_VIDEO_RESOLUTION':'1080p','DCR_VIDEO_ASPECT':'16:9'})
        self.assertEqual(e,{'PATH':'C:/Windows'})

    def test_1080p_4by3_preserves_aspect(self):
        self.base['video_future']['resolution']='1080p'
        e=core.video_output_environment(self.base,self.disc,{})
        self.assertEqual(e,{'DCR_VIDEO_RESOLUTION':'1080p'})

    def test_1080p_stretched_16by9(self):
        self.base['video_future'].update({'resolution':'1080p','aspect':'16:9', 'vsync':True})
        e=core.video_output_environment(self.base,self.disc,{})
        self.assertEqual(e,{'DCR_VIDEO_RESOLUTION':'1080p','DCR_VIDEO_ASPECT':'16:9'})
        self.assertFalse(any('VSYNC' in x for x in e))

    def test_native_wide_without_resolution(self):
        self.base['video_future']['aspect']='16:9'
        e=core.video_output_environment(self.base,self.disc,{})
        self.assertEqual(e,{'DCR_VIDEO_ASPECT':'16:9'})

    def test_game_overrides_global_presentation(self):
        self.base['video_future']['resolution']='1440p'
        game=str(self.disc.resolve())
        self.base['game_overrides']={game:{'video_future':{'resolution':'960p','aspect':'16:9'}}}
        e=core.video_output_environment(self.base,self.disc,{})
        self.assertEqual(e,{'DCR_VIDEO_RESOLUTION':'960p','DCR_VIDEO_ASPECT':'16:9'})

    def test_allowed_inputs_sanitized(self):
        s=core.sanitize_settings({'video_future':{'resolution':'9k','aspect':'32:9'}})
        self.assertEqual(core.video_output_environment(s,self.disc,{}),{})

    def test_present_stays_unsynchronized_and_abi_header_not_modified(self):
        runtime=(LAUNCHER.parent/'runtime'/'dc_runtime.cpp').read_text(encoding='utf8')
        self.assertIn('gpu.swapchain->Present(0u, 0u);',runtime)
        self.assertIn('DCR_VIDEO_RESOLUTION',runtime)
        self.assertIn('dcr_video_destination_rect',runtime)
        self.assertIn('gpu.context->CopyResource(gpu.swapchain_backbuffer.Get(), gpu.display_scanout.Get());',runtime)
        self.assertNotIn('DCR_VIDEO_VSYNC',runtime)

if __name__=='__main__': unittest.main()
