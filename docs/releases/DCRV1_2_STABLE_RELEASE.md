# DreamcastRecompiled V1.2 Stable Release — DCRV1.2

Base: AQUA RC8 validada por el usuario (ChuChu Rocket! portable precompilado).

- La marca pública, ventanas, títulos y empaquetado son DCRV1.2.
- `games/CHUCHU_ROCKET/` conserva sus ejecutables, DLLs y checksums SHA-256.
- `safe_guests/` sigue siendo el respaldo sin alteraciones.
- No se recompila SH-4 en la ruta precompilada verificada.
- VMU independiente: icono seleccionable desde barra de tareas en Windows (API Win32), opción siempre visible activada por defecto, y botón **Abrir / mostrar VMU** para recuperar su ventana.
- El modo sin bordes conserva su carcasa original y las preferencias de VMU.

## Identificadores técnicos

Las cadenas `cpp38e`, `cpp35b1`, `DCR_CPP38E_*`, los sellos `RUNTIME=...`, `DCR_GUEST_IMPL_REV` y nombres de funciones/rutas internas **no se renombraron**: son parte de la identidad de compilación, los hashes o de la compatibilidad de cachés. Modificarlos podría provocar recompilación o romper ABI1. Los documentos históricos mantienen sus nombres de desarrollo por trazabilidad.

## Validación

El ZIP se construye reutilizando exactamente el host Python offline, el runtime y el binario del portable ChuChu de RC8. El renderizado nativo, el comportamiento de la barra de tareas de Windows y el juego requieren prueba en Windows.
