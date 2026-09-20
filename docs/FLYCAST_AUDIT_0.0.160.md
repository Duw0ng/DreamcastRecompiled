# Auditoría contra Flycast — 0.0.160

Fecha: 2026-09-06. Base: ZIP aportado, DreamcastRecomp 0.0.159.
Referencia inspeccionada: Flycast commit `a75d1cff5337732fcc64e5a5900751b4a68ce8f1`.

## Resultado y alcance

Se revisaron las rutas de caché/texturas, geometría y estados D3D11, TA, temporización PVR y puntos concretos del emisor SH-4. Se aplican dos optimizaciones de caché, acompañadas de regresiones ejecutables. No es una certificación de paridad completa: no se hizo una comparación instrucción por instrucción, ni una auditoría exhaustiva de AICA/GD-ROM/Maple, ni se ejecutó un juego comercial.

La arquitectura ya incluye framebuffer, buffers de render/presentación, geometría indexada, caché de texturas con invalidación por páginas y scanout DXGI. Por tanto, atribuir los FPS simplemente a «falta de framebuffer» no describe esta base. La causa dominante en Mouse Mania requiere un perfil de la misma escena en Windows.

## Cambios aplicados

### 1. Distribución de identidad en la caché

En `src/codegen/cpp_emitter.cpp`, función emitida `pvr_texture_cache_slot`, se conservan 32 conjuntos de cuatro entradas. Antes, un único `h ^= h >> 32` dejaba atributos como ancho, flags y formato de paleta sin influencia en los cinco bits bajos usados para seleccionar conjunto. La identidad completa sí se verificaba: era una fuente de colisiones/expulsiones, no una confusión de colores entre texturas.

Ahora se aplica una mezcla de 64 bits antes de seleccionar el conjunto. No se aumenta la memoria ni se cambia la política de reemplazo. La comprobación completa de identidad, los dirty bits y la propiedad inmutable de los snapshots siguen vigentes. La mezcla cuesta dos multiplicaciones adicionales en búsquedas por conjunto; el atajo de textura activa continúa evitando ese cálculo.

Comparación ejecutada sobre las funciones de selección extraídas de ambas versiones: offset 0x10000, RGB565, altura 8, anchos 8/16/32/64/128/256, flags 0.

| Versión | Conjuntos seleccionados | Ocupación máxima del grupo |
|---|---|---|
| 0.0.159 | 11, 11, 11, 11, 11, 11 | 6 identidades para 4 entradas |
| 0.0.160 | 10, 22, 3, 16, 12, 8 | 1 identidad |

En el runtime nuevo se ejecutaron diez barridos de esas seis variantes, con la primera precargada: seis decodificaciones totales. Es una prueba sintética de reutilización, no una medición de FPS ni una afirmación de mejora universal de distribución.

### 2. Independencia de paleta para texturas no paletizadas

`pvr_texture_cache_identity_matches`, `pvr_texture_cache_slot` y `pvr_prepare_texture_cache` normalizan el formato de paleta a cero cuando PixelFmt no es Pal4/Pal8. Antes PAL_RAM_CTRL cambiaba la identidad incluso de RGB565 y provocaba decodificación/recreación de snapshots sin cambiar los texels.

Flycast distingue estos casos al construir la clave en `BaseTextureCache::getTextureCacheData`. La implementación de DreamcastRecomp sigue siendo propia: no se importó código de Flycast.

Se verificó que cuatro cambios de PAL_RAM_CTRL conservan la misma imagen RGB565 y una sola decodificación. Se verificó también que Pal4 y Pal8 sí actualizan sus colores tras cambios de formato y contenido de paleta y que los snapshots anteriores conservan sus datos.

## Comparación y pendientes confirmados

Los símbolos de DreamcastRecomp citados están en `src/codegen/cpp_emitter.cpp`, salvo indicación contraria.

| Área | Evidencia en esta base | Referencia Flycast / conclusión |
|---|---|---|
| Texturas | `pvr_prepare_texture_cache`: 128 entradas; páginas sucias; epoch global de paleta | `core/rend/TexCache.h/.cpp`: clave específica por formato y hashes de bancos Pal4/Pal8. Se corrige la identidad no paletizada; queda invalidación de paleta más precisa por banco. |
| Geometría | `PVRDeferredOpaqueRun`, arena de vértices e índices; D3D11 triangle lists | `core/rend/dx11/dx11_renderer.cpp`, `drawList`: strips para listas no ordenadas. Cambiar topología podría reducir índices, pero exige validar winding, culling y límites de strips. |
| Transparencias | `pvr_flush_deferred_translucent`: orden estable por suma de Z | `core/rend/sorter.cpp` y `core/rend/dx11/oit/`: rutas de ordenamiento y OIT más amplias. No hay equivalencia general demostrada. |
| Modifier volumes | `pvr_submit_packet` cuenta y consume paquetes, con `pvr_modifier_volume_packets_skipped`; stencil desactivado en `pvr_gpu_depth_state` | `DX11Renderer::drawModVols`: procesamiento de volúmenes. Leer paquetes no equivale a dibujar sus sombras. Pendiente funcional importante. |
| Mipmaps | Se resuelve offset del nivel base, pero `pvr_gpu_texture_srv` crea `MipLevels = 1` | `core/rend/TexCache.cpp` y `core/rend/dx11/dx11_texture.cpp`: tratamiento de niveles. El sampler trilineal no crea una cadena que no existe. |
| Fog | Se conserva `fog_ctrl`, sin implementación equivalente de LUT/densidad/color en el shader actual | `core/rend/dx11/dx11_shaders.cpp`, `fog_mode2`: usa textura de fog y densidad. Pendiente visual. |
| Profundidad/blending | `pvr_gpu_depth_state` fuerza ciertas decisiones por tipo de lista; `pvr_gpu_blend_state` cambia ZERO/ZERO a alpha blending | Flycast deriva estados de ISP/TSP en `setRenderState`. Son puntos de fidelidad que requieren casos visuales dirigidos antes de cambiar defaults compatibles con ChuChu. No se alteraron aquí. |
| SPG/scanout | `mmio_read32` sintetiza raster y conserva un avance por polling de 263 lecturas cuando no está activo el reloj de dispositivos | `core/hw/pvr/spg.cpp` usa eventos del scheduler SH-4 y líneas configuradas. No se puede afirmar temporización equivalente. |
| SH-4 | Caché GPR estática con barreras/reloads; `LDTLB` termina en `dc_unimplemented`; FIPR/FTRV rechazan RM distinto de round-to-nearest | `core/hw/sh4/dyna/ssa.cpp`, `ssa_regalloc.h`, modelo MMU y scheduler: arquitectura más completa. No se cambiaron barreras, FPU ni caché de registros en esta revisión. |

Estos pendientes de fidelidad no implican que todos expliquen la caída de FPS de ChuChu. Implementar una función de hardware puede mejorar compatibilidad y también añadir coste.

## Validación realizada

- Configuración y compilación Linux Release: PASS.
- CTest del proyecto: 50/50 PASS.
- Generación y compilación del proyecto C++ de `sh4_literal_pool.elf`: PASS.
- `generated_compile_test`: exit 0, incluyendo las nuevas pruebas de identidad, snapshots, Pal4/Pal8 y la regresión existente de regiones VRAM sucias.
- Runner generado: exit 0, muestra versión 0.0.160 y devuelve el control al host.
- Comparación de los conjuntos elegidos por el hash anterior/nuevo: valores de la tabla anterior.

No se compiló con MSVC ni se ejecutó D3D11 en Windows. No se midieron FPS, audio, arranque comercial ni el segundo demo. El ZIP no incluye imagen del juego. Los cambios afectan al runtime generado: para probarlos hay que reconstruir el recompilador y regenerar/recompilar el runner; reutilizar un EXE anterior no aplica el cambio.

## Prueba sugerida en tu PC

1. Extraer 0.0.160 en carpeta propia y compilar con `build_windows.bat`.
2. Usar el mismo disco y el mismo lanzador habitual; dejar que regenere/recompile el runner.
3. Comparar con 0.0.159 en igual resolución, configuración y escena: menú, Mouse Mania y segundo demo tras 3–4 minutos.
4. Para diagnóstico utilizar `run_commercial_recompiled_profile.bat` en ambas versiones, por separado de la medición de FPS del lanzador normal. Comparar decodificaciones/subidas de texturas y tiempos CPU/PVR en fragmentos de duración y carga similares.

Prioridad siguiente: medir si dominan SH-4, construcción de geometría, subidas de textura o GPU. La invalidación de paleta por banco es una candidata delimitada; strips y SSA requieren un trabajo más amplio y pruebas de equivalencia. No conviene activar fast-math/FMA o recortar barreras SH-4 como una optimización genérica.

## Fuentes reproducibles

Todos los enlaces fijan el commit inspeccionado:

- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/rend/TexCache.h
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/rend/TexCache.cpp
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/rend/dx11/dx11_renderer.cpp
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/rend/dx11/dx11_texture.cpp
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/rend/dx11/dx11_shaders.cpp
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/rend/sorter.cpp
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/hw/pvr/spg.cpp
- https://github.com/flyinghead/flycast/blob/a75d1cff5337732fcc64e5a5900751b4a68ce8f1/core/hw/sh4/dyna/ssa_regalloc.h
