# Correcciones de compilación sobre 0.0.203_rebased193

## Compilar y probar ChuChu Rocket en Windows

1. Extrae este ZIP en una carpeta nueva y corta, por ejemplo `C:\DCR203fix`.
   No copies las carpetas `build` o `generated` de una versión anterior.
2. Usa Visual Studio 2022 / Build Tools con Desktop development with C++, Windows SDK y CMake.
   Ejecuta desde Developer PowerShell / x64 Native Tools si CMake no aparece en PATH.
3. En PowerShell, desde la carpeta que contiene el BAT:

```powershell
.\compilar_y_probar_chuchu.bat "C:\Juegos\ChuChu Rocket.cdi"
```

También puedes arrastrar el CDI sobre `compilar_y_probar_chuchu.bat`.
El BAT compila las herramientas, ejecuta 53 pruebas (incluida compilación/enlace
real del C++ generado), extrae tu CDI, regenera el juego, compila en Release x64
y abre la ventana. La compilación comercial conserva el límite de un trabajo.
Las pruebas nuevas pueden tardar varios minutos la primera vez en Windows.

Para repetir la ejecución con compilación incremental:

```powershell
.\run_commercial_recompiled.bat "C:\Juegos\ChuChu Rocket.cdi"
```

Haz clic en la ventana PVR para darle foco. Enter = Start; flechas = dirección;
Z/Espacio/J = A; X/K = B; C/U = X; V/I = Y.
Los logs de la sesión quedan en `logs`. Si falla la compilación, copia el primer
error del compilador con sus líneas de contexto; los errores finales de MSBuild
suelen ser consecuencias. Cerrar manualmente el juego puede devolver RC=130.

## Errores corregidos

- **Runtime generado:** `dc_read64_fmov_hot` llamaba a `dc_guest_read32_hot`,
  un helper definido solamente en los archivos del programa. Se reproducía un
  error de identificador no declarado al compilar `dc_runtime.cpp`.
  Sus dos lecturas de respaldo ahora llaman a `dc_read32_hot`, que sí pertenece
  al runtime. El cambio está en `src/codegen/cpp_emitter.cpp`, para sobrevivir
  a cada regeneración. No basta con modificar un C++ generado temporalmente.
- **Salida de una sola función:** emitía llamadas `dc_guest_*` sin emitir sus
  definiciones. Ambos modos del emisor ahora comparten los mismos helpers;
  la salida individual también incluye `<cstring>` para `std::memcpy`.
- **Herramientas antiguas:** el BAT comercial antes compilaba las herramientas
  solo si faltaba `dc_disc_probe.exe`. Ahora configura y recompila incrementalmente
  `dc_disc_probe`, `dc_boot_prepare` y `dc_raw_recomp` en cada invocación;
  comprueba que los tres ejecutables existan antes de continuar.
- **Diagnósticos PowerShell:** el script de progreso conserva stderr nativo,
  usa el código de salida de CMake y restaura el entorno en `finally`.
  Cambio revisado estáticamente; no ejecutado con PowerShell en este entorno.
- **Pruebas insuficientes:** se añaden `generated_single_build` y
  `generated_program_build`, que configuran, compilan, enlazan y ejecutan el
  smoke test real del runtime generado. Antes se validaba principalmente texto.

`dc_runtime.cpp` ya figuraba en la biblioteca de CMake y las bibliotecas D3D11
ya estaban enlazadas. Añadirlas de nuevo no corrige estos identificadores.
El parche exacto de los archivos modificados está en `COMPILATION_FIX.patch`.

## Alcance

Se mantiene la versión base 0.0.203, sus opciones de juego y la carpeta
experimental CT2 incluida en el original. Esta corrección no aplica los modos
fijos experimentales de CT2 a ChuChu. El CDI de ChuChu no viene en el ZIP.
Las verificaciones locales usan GCC/Linux; no equivalen a validar MSVC, los BAT,
Direct3D, audio de Windows o una partida real de ChuChu. No se promete que todos
los posibles errores específicos del equipo o del juego estén resueltos.
