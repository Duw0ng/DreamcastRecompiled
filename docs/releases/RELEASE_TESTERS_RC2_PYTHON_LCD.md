# Dreamcast Recompiled v1.2 AQUA — RC2 Python + VMU LCD

## Interfaz única

- Eliminada la interfaz **Aqua Nativo Win32** y sus ejecutables secundarios.
- **Único launcher**: Aqua original en `launcher/` (Python 3.12 + Tkinter).
- El archivo `DreamcastRecompiled.exe` del release Windows se construye con
  PyInstaller y contiene el **programa Python**, no una interfaz escrita en Go.
- Compilación de Windows reproducible: `.github/workflows/build-python-aqua-windows.yml`.
- El ZIP de este entorno es un **candidato fuente**: al no disponer de Windows,
  NO trae el `.exe` PyInstaller ya compilado. Se inicia con `ABRIR_LAUNCHER.bat`,
  que prepara una instalación privada en Windows la primera vez (necesita Internet).
  Después arranca directamente desde `DreamcastRecompiled.exe`.
- Para entregar un release sin preparación, ejecutar el workflow en GitHub (Windows
  runner) y publicar el archivo `DreamcastRecompiled_Aqua_Python_Windows_x64.zip`.

## LCD real de VMU A1

- Antes Maple anunciaba LCD, pero rechazaba `BLOCK WRITE` función `0x04000000`.
- Ahora `dcr_maple` (plugin principal) y el fallback en `DreamcastRuntime`
  reciben frames de 192 bytes (48×32, 1 bpp), y responden `ACK` al juego.
- Un fichero auxiliar `.lcd` (208 bytes) se actualiza por cada transferencia
  real del juego. Guarda cabecera `DCRLCD01`, timestamp Unix en ms y bitmap.
  Este fichero NO es la memoria de partidas `.bin`, y no se escribe en ella.
- La página VMU del Aqua original muestra los bits reales, actualiza cada
  250 ms e indica si la señal es reciente. Permite voltear/invertir la imagen
  **solo visualmente** si la orientación del juego lo requiere.
- Si el juego no manda frames LCD, la pantalla indica **sin señal**. No se simula.
- La lectura `BLOCK READ` del dispositivo LCD devuelve el frame disponible o
  una pantalla en blanco antes de la primera actualización.
- Errores de guardado auxiliar (p. ej. directorio sin permisos) no interrumpen
  las transferencias LCD del juego.

## Integridad y límites

- No se tocó el IR SH-4 ni el código de instrucciones del recompilador.
- Se modifica el camino Maple de plugin, runtime fallback y plantilla generada;
  son necesarias nuevas DLL del runtime al ejecutar. `run_game.bat` detecta cambios
  en fuente/manifiesto e incorpora recompilación selectiva de runtime/plugins.
- No hemos probado una señal LCD enviada por un juego comercial en Windows.
  **ChuChu Rocket!** es una primera prueba; que no muestre píxeles no implica
  automáticamente un fallo: algunos juegos no escriben imágenes LCD.
- No incluye binarios Windows modificados del motor ya recompilados: eso sucede
  en el primer boot cuando existen CMake y MSVC. Los tests aquí son Linux/Tk.
- La base estable AQUA anterior queda conservada por separado.

## Prueba mínima

1. Abrir Aqua Python, entrar a **VMU / Files** y elegir la VMU A1.
2. Iniciar un juego; si la ventana se cierra por Play, reabrir la pestaña VMU.
3. Observar si el mensaje cambia de `Esperando señal LCD` a `LCD activo`.
4. Verificar el archivo `dreamcast_vmu_a1.lcd` junto a la VMU de 128 KiB.
5. Adjuntar el log `[Maple ...]` y ese archivo de 208 bytes si no hay imagen.
6. Comparar con Sega Tetris y Mr. Driller para descartar regresiones del bus Maple.
