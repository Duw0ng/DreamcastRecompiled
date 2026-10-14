"""V8.2 output scaling regression checks (including opt-in GDI fallback)."""
from __future__ import annotations
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import core

RUNTIME = Path(__file__).resolve().parents[2] / 'runtime' / 'dc_runtime.cpp'
EXTENT = Path(__file__).resolve().parents[2] / 'runtime' / 'include' / 'dcr_video_output_extent.hpp'

class VideoRecoveryTest(unittest.TestCase):
    def test_environment_is_per_process_and_safe(self):
        game = r'C:\games\CHUCHU_ROCKET.cdi'
        defaults = core.sanitize_settings({})
        incoming = {'DCR_VIDEO_OUTPUT_BACKEND':'gdi','DCR_VIDEO_RESOLUTION':'1080p',
                    'DCR_VIDEO_ASPECT':'16:9','KEEP':'yes'}
        self.assertEqual(core.video_output_environment(defaults, game, incoming), {'KEEP':'yes'})
        custom = core.sanitize_settings({'video_future':{'resolution':'1080p','aspect':'16:9','output_backend':'gdi'}})
        env = core.video_output_environment(custom, game, incoming)
        self.assertEqual(env['DCR_VIDEO_RESOLUTION'], '1080p')
        self.assertEqual(env['DCR_VIDEO_ASPECT'], '16:9')
        self.assertEqual(env['DCR_VIDEO_OUTPUT_BACKEND'], 'gdi')
        self.assertNotIn('DCR_VIDEO_VSYNC', env)
        self.assertEqual(env['KEEP'],'yes')

    def test_backend_can_be_saved_per_game(self):
        game = str((Path(__file__).parent / 'CHUCHU_ROCKET.cdi').resolve())
        config = core.sanitize_settings({'video_future':{'output_backend':'gdi'},
                 'game_overrides':{game:{'video_future':{'resolution':'960p','output_backend':'dx11'}}}})
        self.assertEqual(core.video_output_environment(config,game,{}).get('DCR_VIDEO_RESOLUTION'), '960p')
        self.assertNotIn('DCR_VIDEO_OUTPUT_BACKEND', core.video_output_environment(config,game,{}))

    def test_dxgi_compatible_blit_replaces_broken_fullscreen_shader(self):
        source=RUNTIME.read_text(encoding='utf-8')
        extent=EXTENT.read_text(encoding='utf-8')
        segment=source[source.index('bool pvr_gpu_ensure_swapchain(DCRuntime& runtime) {'):source.index('D3D11_BLEND pvr_gpu_blend_factor(')]
        self.assertIn('D3D11_CULL_NONE',source)  # PVR raster unchanged
        self.assertIn('desc.BufferDesc.Width = kSoftWidth;',segment)
        self.assertIn('desc.BufferDesc.Height = kSoftHeight;',segment)
        self.assertNotIn('D3DCompile(',segment)
        self.assertIn('video-scaler=v8.4/dxgi-blit-native-copy',segment)
        self.assertIn('gpu.context->CopyResource(gpu.swapchain_backbuffer.Get(), gpu.display_scanout.Get());',segment)
        self.assertIn('client_w >= 64 && client_h >= 64',extent)  # legacy helper still available
        self.assertIn('Present(0u, 0u)',segment)

    def test_gdi_backend_explicit_only(self):
        source=RUNTIME.read_text(encoding='utf-8')
        self.assertIn('DCR_VIDEO_OUTPUT_BACKEND',source)
        self.assertIn('if (!dcr_video_compat_gdi() && pvr_gpu_present_window(runtime))',source)
        self.assertIn('compatible_gpu_scanout = pvr_gpu_readback_latest(',source)
        self.assertIn('video[\'output_backend\'] == \'gdi\'', (Path(__file__).resolve().parents[1]/'core.py').read_text())

if __name__=='__main__': unittest.main()
