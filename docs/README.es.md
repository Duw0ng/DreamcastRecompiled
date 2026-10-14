# DreamcastRecompiled V1.2 Stable Release

**DCRV1.2 Stable — Español/Inglés + VMU Fix**

[English](../README.md) · [Releases](https://github.com/Duw0ng/DreamcastRecompiled/releases) · [Guía de ChuChu Rocket!](CHUCHU_ROCKET_TESTER_V1.2.md)

DreamcastRecompiled recompila código SH-4 a C++ nativo y lo ejecuta junto con un runtime de hardware Dreamcast. Combina recompilación, recuperación de targets, intérprete de respaldo y HLE. La compatibilidad continúa en desarrollo.

## Novedades

- Launcher Aqua con interfaz en español e inglés y selección de temas.
- Distribución Windows x64 con Python/Tk incluidos; no requiere instalar Python.
- ChuChu Rocket! como juego principal de prueba, con módulo nativo precompilado para evitar reconstruirlo en cada inicio.
- VMU independiente con opción siempre visible y botón para recuperar su ventana.
- Configuración, calibración y pruebas de mandos; opciones de diagnóstico y consola.

## Empezar

1. Descargar y extraer completamente el paquete Windows desde Releases cuando esté publicado.
2. Abrir `DreamcastRecompiled.exe` o `ABRIR_LAUNCHER.bat`.
3. Seleccionar una imagen propia y compatible de ChuChu Rocket! en Juegos.
4. Configurar el mando e iniciar. Conservar los logs para reportar problemas.

El paquete incluye código recompilado de ChuChu Rocket!, pero **no incluye el CDI**. La DLL no sustituye el disco: necesita la imagen del usuario para preparar el arranque y acceder a los recursos del juego.

| Campo del disco de referencia | Valor |
|---|---|
| Producto / revisión | `MK-51049 / V1.007` |
| Región del encabezado | `JUE` — Japón, Estados Unidos y Europa |
| Volume ID | `CHU_CHU_ROCKET_KAL` |

Estos datos identifican la copia de referencia; no garantizan compatibilidad con todas las revisiones. El selector automático comprueba título/volumen, no un hash del ejecutable ni la revisión exacta.

Otros juegos pueden necesitar Visual Studio C++ Build Tools, CMake y Windows SDK. Los archivos fuente de GitHub no incluyen los binarios preparados del paquete Windows.

## Estado de validación

ChuChu Rocket! tiene gameplay offline validado por el mantenedor. La revisión del paquete confirmó integridad del ZIP y checksums de sus ocho binarios nativos. La suite del launcher ejecutó 169 pruebas: 132 aprobadas y 37 omitidas por requerir escritorio gráfico. La selección de la VMU en la barra de tareas todavía requiere una comprobación visual final en Windows.

Stable designa la base del proyecto; no significa compatibilidad completa con Dreamcast. Otros títulos pueden fallar al arrancar o presentar defectos gráficos y de audio. DCNet/online sigue siendo experimental.

El código original del proyecto usa [MIT](../LICENSE); las dependencias y el código de terceros conservan sus derechos y licencias. Proyecto independiente de SEGA.
