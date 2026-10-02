# cpp32a — ChuChu callback closure

Base de trabajo: v0.1.1 Official / rama estable de compatibilidad. Esta revisión cambia únicamente el descubrimiento estático de callbacks SH-4.

## Objetivo

Cerrar la familia de `No recompiled/native target registered` que llega al dispatcher indirecto `0x8C0190A4` en menús/estados de ChuChu Rocket!, sin seeds por título ni direcciones hardcodeadas.

Fallos observados que motivan esta revisión:
- `0x8C04FFC0` — Homepage/state path.
- `0x8C034FA0` — otro estado/menu de la misma familia.
- Históricos de la misma familia: `0x8C02DC50`, `0x8C0278CC`, `0x8C050066`.

## Cambio genérico

Se restaura un detector conservador de tablas Katana de dos niveles:

```text
state_table -> row0, row1, row2, ...
rowN        -> method0, method1, method2, metadata...
```

Reglas:
- una fila debe comenzar con exactamente tres punteros locales que validan como entradas SH-4 llamables;
- una fila queda independientemente anclada cuando al menos dos de sus tres métodos ya pertenecen a la closure;
- se permiten como máximo dos filas estrictamente válidas no ancladas entre dos filas independientemente ancladas;
- punteros de fila adyacentes exactamente duplicados se aceptan solo como alias de una fila ya aceptada;
- los alias no pueden iniciar una familia por sí mismos;
- cada destino nuevo vuelve a pasar por `analyze_code_fragment_at` y debe quedar con 0 instrucciones desconocidas antes de entrar a la closure;
- no hay direcciones de ChuChu hardcodeadas ni seeds nuevos.

## No cambia

- PVR/TA/D3D11
- AICA/CDDA/ARM7
- GD-ROM
- Maple/VMU
- timing SH-4
- fast/direct dispatch
- runtime templates
- recompilación incremental

## Telemetría

`dc_raw_recomp` añade:

```text
Nested state callbacks: N
```

La cantidad debe ser pequeña/moderada. Una subida explosiva de miles de funciones es una regresión y esta revisión debe descartarse.

## Prueba Windows

Recompilar ChuChu desde limpio y recorrer:
1. menú principal;
2. Homepage;
3. Puzzle Edit;
4. Options;
5. 4P Battle;
6. el menú que producía `0x8C034FA0`.

Criterios de aceptación:
- sin missing target en esos caminos;
- `RAW_SH4=0`;
- crecimiento acotado de `Reachable functions`;
- gameplay normal sigue alrededor de los 60 FPS de la estable;
- no aparecen regresiones de audio/gráficos.
