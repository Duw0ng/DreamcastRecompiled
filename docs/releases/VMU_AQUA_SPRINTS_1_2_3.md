# Dreamcast Recompiled v1.2 AQUA — VMU independiente / Sprints 1, 2 y 3

**Estado: candidato a pruebas RC3.** El motor SH-4, Maple del RC2, PVR, AICA, red y recompiler NO se modificaron. Se utiliza el transporte LCD real introducido en RC2 (`*.lcd`, 208 bytes, 48×32×1 bpp). Ninguna pantalla inventada se escribe en ese transporte.

## Sprint 1 — VMU independiente

- `ABRIR_VMU.bat` abre una **segunda aplicación autónoma**; no hace falta iniciar un juego ni el launcher completo. En el ejecutable Python construido con PyInstaller, el mismo `DreamcastRecompiled.exe --vmu` abre la VMU.
- Desde Aqua > VMU / Archivos, usar **Abrir VMU independiente**. No se utiliza ningún launcher Go/Win32.
- Carcasa original blanca estilizada o variante azul, ventana recortada en Windows, draggable, modo siempre visible, tamaños 75 / 100 / 125 %, modo LCD compacto (`Tab` o clic derecho), opción de voltear la imagen.
- La LCD reproduce los **fotogramas reales de Maple** cuando el juego los entrega. En modo Auto tienen prioridad si llegaron en los últimos 4 segundos. Al dejar de recibirlos vuelve la pantalla autónoma. Puede elegirse «LCD del juego» para ver el último cuadro aun cuando esté antiguo.
- El manager integrado ahora conserva exclusivamente el explorador y diagnósticos de saves; el LCD pasa a la ventana aparte.

## Sprint 2 — Encendido, controles y explorador

- Botón **Sleep** encender/apagar. **Mode** vuelve al inicio; cruceta para elegir; **A** confirmar; **B** retroceder.
- Inicio independiente: reloj y fecha, listado de archivos de una VMU (saves y estado FAT, solo lectura), estadísticas de memoria, apps originales.
- Detecta VMU de 128 KiB disponibles y permite elegir otra por clic derecho > Memorias detectadas / Elegir VMU. Respaldar la VMU genera una **copia**, sin modificar el original.
- La pantalla LCD del juego no es la memoria flash; los botones del gadget NO envían input Maple a juegos. La interacción es de las aplicaciones autónomas.

## Sprint 3 — AquaPet + minijuegos

- **AquaPet** original: alimentar, limpiar, descansar, ver barras de estadísticas, energía, felicidad, experiencia, nivel y objetos desbloqueados.
- Basado en lanzamientos **registrados por el launcher** (`launcher_v1_2/settings.json`): un juego iniciado después de crear la mascota da 5 XP; algunos títulos añaden objetos temáticos (`QUESITO`, `ZAPATILLAS`, `CUBO`, `LINTERNA`). Se muestran en el inventario y como detalles del sprite, sin usar binarios de Sega ni afirmar haber leído el progreso del juego.
- Pet persistente en `%LOCALAPPDATA%\DreamcastRecompiled\launcher_v1_2\aqua_pet.json` (o `~/.dreamcast_recompiled/launcher_v1_2/` sin LOCALAPPDATA). Guarda aparte del archivo flash VMU.
- Dos minijuegos **propios**, jugables desde la pantalla de 48×32 con botones/teclado: `ATRAPA` (moverse a la columna y pulsar A bajo el objetivo) y `MEMORIA` (memorizar y repetir flechas). Dan XP por victoria.
- Evolución leve de estadísticas según horas reales, acotada a un máximo de 24 h por revisión; no se castiga a usuarios que vuelven luego de una ausencia larga.

## Controles

| VMU | Teclado |
|---|---|
| Cruceta | Flechas |
| A | Z, Espacio o Enter |
| B | X, Retroceso o Escape |
| Sleep | S |
| Mode | M |
| Compacto/carcasa | Tab |
| Configuración, slot, backups, pantalla | Clic derecho sobre carcasa |
| Arrastrar ventana | Clic izquierdo sobre zona libre de carcasa |
| Cerrar (Windows sin barra de título) | X pequeña, arriba a la derecha, o clic derecho > Cerrar |

En modo Auto, si la VMU está recibiendo LCD real, clic en la pantalla cambia al modo autónomo para usar las aplicaciones; para volver a modo Auto, clic derecho > Pantalla > Automático.

## Ejecución

Con el `.exe` construido en Windows mediante **GitHub Actions** (workflow `build-python-aqua-windows.yml`):

```powershell
.\DreamcastRecompiled.exe --vmu
# O doble clic en ABRIR_VMU.bat
```

Desde el ZIP fuente de RC3:

```powershell
py -3 .\launcher\launch.py --vmu
# O ABRIR_VMU.bat (si falta Python, preparará el Aqua PyInstaller una vez, con conexión)
```

**Este ZIP creado en Linux es código fuente y recursos**: no contiene un `.exe` Python/Tk para Windows ya compilado. Para descargar la versión Windows lista para testers, ejecutar el workflow de GitHub Actions en runner Windows x64 y bajar su ZIP artifact; el `.exe` publicado sí incluye Python, Tk y las imágenes del Aqua empaquetadas.

## Qué NO incluye (Sprint 4 futuro)

- No hay emulación de CPU VMU ni ejecución de minijuegos VMU binarios originales (`VMS` / `VMI`).
- No hay emulación de buzzer, protocolo de botones físicos VMU real, alarma de hardware ni comunicación entre VMU autónoma y juego mediante los controles.
- El monitor LCD del juego está respaldado por Maple RC2 (A1 verificada con ChuChu Rocket!). Otros slots mostrarán imagen real únicamente si su ruta `.lcd` recibe cuadros efectivamente.
- El minijuego de AquaPet no está guardado dentro de la memoria VMU; es una aplicación propia, separada y reversible.

## Verificaciones en este entorno

- 144 tests del launcher: **139 pass / 5 fail heredados** (pruebas antiguas con expectativas desactualizadas, ya fallaban en RC2), incluyendo 8 pruebas nuevas de VMU independiente.
- Interfaz Tk creada correctamente bajo Xvfb con carcasa, botones y pantalla en modo autónomo y LCD con archivo de prueba.
- Comparación binaria con RC2: **243 archivos de src, runtime, plugins, include, portable y tools idénticos**.
- **Pendiente probar la forma de ventana recortada en Windows 10/11**, la compilación PyInstaller Windows y la experiencia real jugando simultáneamente. El runner no puede simular Windows aquí.
