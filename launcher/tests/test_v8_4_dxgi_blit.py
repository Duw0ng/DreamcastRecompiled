"""Regression tests for the shader-free V8.4 windowed DXGI stretch path."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]

class DXGICompatTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text=(ROOT/'runtime/dc_runtime.cpp').read_text(encoding='utf-8')
        first=text.index('bool pvr_gpu_ensure_swapchain(DCRuntime& runtime) {')
        last=text.index('D3D11_BLEND pvr_gpu_blend_factor(', first)
        cls.output=text[first:last]

    def test_native_copy_even_at_scaled_resolution(self):
        self.assertIn('desc.BufferDesc.Width = kSoftWidth;',self.output)
        self.assertIn('desc.BufferDesc.Height = kSoftHeight;',self.output)
        self.assertIn('gpu.context->CopyResource(gpu.swapchain_backbuffer.Get(), gpu.display_scanout.Get());',self.output)
        self.assertIn('DXGI_MODE_SCALING_STRETCHED',self.output)
        self.assertNotIn('D3DCompile(',self.output)
        self.assertNotIn('gpu.context->Draw(3u',self.output)

    def test_present_vsync_unchanged(self):
        self.assertIn('gpu.swapchain->Present(0u, 0u)',self.output)

    def test_has_unique_cache_and_output_markers(self):
        b=(ROOT/'scripts'/'windows'/'run_game.bat').read_text(errors='replace')
        # Any cpp38e v8.x runtime stamp; the exact id changes with every runtime fix.
        self.assertRegex(b, r'set "DCR_RUNTIME_BUILD_ID=cpp38e-v8\.\d+[-.\w]*"')
        self.assertIn('video-scaler=v8.4/dxgi-blit-native-copy',self.output)

    def test_4_3_window_correction_and_gdi_original(self):
        cpp=(ROOT/'runtime/dc_runtime.cpp').read_text()
        self.assertIn('4:3 DXGI compatibility window=',cpp)
        self.assertIn('!dcr_video_compat_gdi() && !video_output.stretch_wide',cpp)
        self.assertIn('presentation=v8.4/GDI',cpp)

if __name__=='__main__': unittest.main()
