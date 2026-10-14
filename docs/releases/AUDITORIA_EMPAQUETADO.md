# Auditoría RC2: Python como única interfaz

- Eliminados `DreamcastRecompiled.exe` antiguo (interfaz Aqua nativa Go),
  `DreamcastRecompiled_Aqua_Setup.exe` (bootstrap Go) y `VMU_Manager.exe` (UI Go).
- El **único** `DreamcastRecompiled.exe` que se publicará para testers es el que
  produce PyInstaller en Windows a partir de `launcher/launch.py`.
- Este ZIP es paquete fuente: para obtenerlo localmente ejecutar `ABRIR_LAUNCHER.bat`
  (requerirá conexión a Internet solo la primera vez), o usar el workflow de Windows.
- `launcher/vmu_lcd.py` recibe archivos auxiliares `.lcd` reales de 208 bytes;
  nunca modifica las imágenes VMU de 128 KiB.
- Código de recepción LCD de Maple modificado en el plugin principal, fallback
  y plantilla generada. No hay rediseño del menú Aqua Python.
- No se incluye una biblioteca Windows precompilada de Maple modificada; CMake
  recompila el plugin cuando se usa la versión nueva con fuentes en Windows.
- Verificación Linux: plugin dcr_maple compila, syntax-check runtime y emisor,
  UI nocturna/VMU funciona en Xvfb, transport LCD 3/3 pruebas y suite heredada
  131/136 (cinco fallas previas en contratos desactualizados).
- Pendiente: ejecutar el ZIP PyInstaller generado por GitHub Actions en Windows
  y realizar prueba con un juego que envíe frames al LCD.
