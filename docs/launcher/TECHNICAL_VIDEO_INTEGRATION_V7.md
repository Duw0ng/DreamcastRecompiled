# Dreamcast Recompiled v1.2 — Integración técnica de vídeo

**Base protegida:** cpp38e HOT_BLOCK_RECOVERY. **Estado:** V7 implementa el panel de ajustes y persistencia; NO añade al runtime las opciones marcadas «Pendiente de motor».

## 1. Lo que ya hace cpp38e

- `run_game.bat` acepta `--renderer=dx11`, `--renderer=software` y deja automática la selección sin un override. Esto ya está conectado al panel «Vídeo».
- En `runtime/dc_runtime.cpp`, `pvr_gpu_ensure_swapchain` crea un `IDXGISwapChain` usando **kSoftWidth × kSoftHeight** (640×480), formato `DXGI_FORMAT_B8G8R8A8_UNORM`, dos buffers, modo ventana y `DXGI_SWAP_EFFECT_DISCARD`.
- `pvr_gpu_present_window_d3d11` copia la textura de scanout a un backbuffer compatible con `CopyResource` y termina con `swapchain->Present(0u, 0u)` (sin espera vertical en esa ruta).
- El programa también contiene una copia generada de la ruta de vídeo en `src/codegen/cpp_emitter.cpp`. Cualquier modificación definitiva del host requiere revisar las rutas generadas y evitar duplicaciones.
- El comportamiento de audio AICA, el guest SH-4 y la ABI1 permanecen congelados.

## 2. Contrato entre launcher y host (siguiente fase)

Crear una configuración de presentación **solo en el host**, independiente del estado Dreamcast:

```cpp
struct HostVideoOptions {
    uint32_t output_width = 640;
    uint32_t output_height = 480;
    enum class Aspect { Original4x3, Wide16x9, Auto } aspect = Aspect::Original4x3;
    enum class Filter { Nearest, Bilinear, Sharp } filter = Filter::Bilinear;
    bool borderless_fullscreen = false;
    bool vsync = false;  // valor actual DXGI: Present(0, 0)
    bool fps_overlay = false;
};
```

**No agregar estos campos a `DCRuntime`, `PVRDeferredState` ni a los encabezados de `runtime/guest_abi/v1`.** Los defaults deben reproducir exactamente la presentación existente. La configuración debe validarse antes de crear el dispositivo gráfico; un valor inválido vuelve a los defaults. El launcher persistirá valores en su JSON de usuario y, cuando exista el nuevo host, podrá pasar un archivo de opciones versionado mediante `DCR_HOST_VIDEO_CONFIG` o flags explícitos implementados. No introducir variables que el host todavía no lea.

## 3. Resolución y reescalado

Separar **resolución nativa de render** (PVR: 640×480, o la dimensión que corresponda a la modalidad de vídeo) y **tamaño de la ventana/backbuffer** (960p, 1080p, 1440p, monitor). Para salida 1920×1080 NO basta con cambiar `desc.BufferDesc.Width`/`Height`: el `CopyResource` actual exige texturas compatibles en formato y tamaño.

En su lugar:

1. Mantener sin cambios la textura de scanout nativo de cpp38e.
2. Crear backbuffers del tamaño de salida solicitado.
3. Dibujar un **triángulo de pantalla completa** que muestree la textura de scanout mediante una SRV (`ID3D11ShaderResourceView`) hacia una RTV del backbuffer.
4. Elegir `D3D11_FILTER_MIN_MAG_MIP_POINT` (nearest) o `D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT` (bilinear) en el *sampler del post-proceso*. No cambiar los samplers que usa el PVR para las texturas de los juegos.
5. Añadir un filtro de nitidez como pixel shader separado y opcional, que pueda desactivarse sin reconstruir ni recompilar SH-4.

El escalado de salida no debe falsear el FPS de emulación ni la cadencia de vblank.

## 4. Aspect ratio 4:3 / 16:9

Calcular un viewport de presentación centrado (letterbox/pillarbox), sin modificar las coordenadas que entrega el PVR:

```text
viewport_width = min(window_width, window_height * aspect_w / aspect_h)
viewport_height = viewport_width * aspect_h / aspect_w
si viewport_height > window_height:
    viewport_height = window_height
    viewport_width = viewport_height * aspect_w / aspect_h
viewport_x = (window_width - viewport_width) / 2
viewport_y = (window_height - viewport_height) / 2
```

**16:9 significa salida de aspecto ancho**, no un *widescreen hack* para transformar automáticamente los juegos 4:3. La opción «Auto» puede usar el aspecto del modo de vídeo del juego o, si no se conoce, quedarse en 4:3.

## 5. Pantalla completa

Implementar primero **borderless fullscreen**, manteniendo el mismo `HWND`, usando estilos Win32 y `SetWindowPos` en el monitor elegido. Al salir, restaurar tamaño/posición y estilo anterior. Manejar `WM_SIZE`, cambios de monitor, escala DPI y minimización (`width=0` o `height=0`) sin llamar a un `ResizeBuffers` inválido.

Secuencia segura del resize: desasociar render targets → liberar referencias al backbuffer y sus RTV/SRV → `IDXGISwapChain::ResizeBuffers` → reconstruir RTV y viewport. Manejar `DXGI_ERROR_DEVICE_REMOVED/RESET` con recuperación y el fallback original.

## 6. VSync y límites de FPS

Introducir un parámetro explícito `sync_interval` en la **presentación host**:

- `Present(0, 0)`: valor actual de cpp38e para la ruta DXGI observada.
- `Present(1, 0)`: sincronizar la presentación con el refresco de pantalla compatible.

VSync puede bloquear el hilo del host. No debe alterar el tiempo virtual del SH-4, Holly, GD-ROM o AICA; medir frame pacing, latencia de audio y duración de cargas contra la estable. Mantener el vsync desactivado por defecto en el host mientras no se compruebe el nuevo camino. **No reutilizar `--perf` como overlay de FPS**: es un perfilador pesado. Un indicador futuro debe contar presentaciones con métricas ligeras y actualización lenta.

## 7. Orden de implementación y protección contra regresiones

1. Crear un adaptador de opciones *host-only* sin alterar ABI1. Test: parseo, defaults equivalentes, valores fuera de rango.
2. Añadir posprocesamiento DX11 a 640×480 sin alterar la escena. Test: imagen y rendimiento idénticos o dentro de tolerancia.
3. Añadir escalado/aspecto y resize. Test: 640×480, 1280×960, 1920×1080, pérdida de dispositivo y Alt+Tab.
4. Añadir filtro, pantalla completa y VSync **uno a la vez**. Medir cada incremento.
5. Conectar las opciones actualmente guardadas por V7 con el host, solo cuando exista el soporte y se haya probado en Windows.
6. Estabilizar el camino software con una presentación escalada independiente, sin cargar D3D11 si el usuario eligió software.

Gates de aceptación: preservación del guest ABI1, mismo menú, **cargas GD-ROM sin bloqueos de 2–5 FPS**, sonido sin nuevas interrupciones, al menos el rendimiento cpp38e original (~54–61 FPS en las escenas validadas de ChuChu Rocket!), pruebas de otros juegos y rollback instantáneo a la presentación anterior.

## 8. Qué guardará la V7 ahora

```json
{
  "renderer": "dx11",
  "video_future": {
    "resolution": "1080p",
    "aspect": "4:3",
    "filter": "bilinear",
    "fullscreen": false,
    "vsync": false,
    "fps_counter": false
  }
}
```

**Solo `renderer` tiene efecto en el lanzamiento actual.** El objeto `video_future` se conserva validado, también por juego, para una implementación host posterior. No se generan flags ficticios ni se escribe en `game.cfg` o `dcr_game_module.dll`.

## 9. Audio y mandos

La V7 conecta `--no-audio` y deja las rutas de configuración originales de los mandos. El volumen, el selector de salida de audio, la latencia, las zonas muertas y vibración solicitada se guardan en objetos `audio_future`/`controller_future` **sin afectar al runtime**. Un backend de audio futuro deberá modificar solo mezcla/resampling y la cola de salida, nunca la frecuencia de avance ARM7/AICA. La API de perfiles de entrada debe seguir usando la configuración estable y no afectar a la ABI1.
