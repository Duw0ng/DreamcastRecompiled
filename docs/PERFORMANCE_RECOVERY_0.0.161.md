# Performance recovery 0.0.163

## Prioridad de milisegundos observada en el PERF 0.0.159

Sobre 2,566 frames:

| Componente | Total estimado | ms/frame aprox. |
|---|---:|---:|
| Full tick total | 34,116 ms | 13.295 |
| Device total | 25,504 ms | 9.939 |
| PVR clock | 19,168 ms | 7.470 |
| Host sync | 6,336 ms | 2.469 |
| UI/host window | 4,749 ms | 1.851 |
| PVR TA parse | 2,178 ms | 0.849 |
| TMU/GD | 2,047 ms | 0.798 |
| IRQ | 1,814 ms | 0.707 |
| PVR geometry | 1,059 ms | 0.413 |
| GPU render | 497 ms | 0.194 |
| PVR state | 159 ms | 0.062 |

Los subcomponentes están contenidos dentro de categorías mayores; **no deben sumarse**.

## Orden recomendado después de 0.0.163

1. Event-driven/batched PVR clock dentro del scheduler, evitando trabajo por cada full tick cuando no hay borde SPG/IRQ/evento relevante.
2. Host device-clock catch-up: reducir iteraciones/branching conservando AICA/TMU/GD exactos.
3. Host UI/window polling: desacoplar el polling que no necesita ocurrir con la misma frecuencia del scheduler SH-4.
4. TA parse/geometry para Mouse Mania: ya no domina escenas ligeras, pero vuelve importante bajo gran cantidad de ratones.
5. GPU render no es actualmente el objetivo principal: su coste medido es pequeño frente al scheduler/device clock.
