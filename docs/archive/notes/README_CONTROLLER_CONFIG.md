# Configurador de mando experimental — DreamcastRecomp 0.0.210

Ejecuta `Configurar_Mando.bat` desde la raiz.

- `xinput`: Xbox, DualShock/DualSense mediante DS4Windows/Steam Input y mandos compatibles XInput.
- `winmm`: ruta experimental para mandos DirectInput/WinMM, incluido DualShock 4 cuando Windows lo expone como joystick clasico.
- `auto`: intenta XInput y luego WinMM.

Selecciona cada control Dreamcast y pulsa **Capturar**, luego mueve/pulsa la entrada fisica. Guarda el perfil en `profiles/controller_profile.ini`.
El runner universal lo carga automaticamente si existe.

La ruta WinMM es experimental: algunos drivers HID pueden exponer ejes combinados o no publicar gatillos por separado. En esos casos DS4Windows/Steam Input + backend XInput es la ruta recomendada para la primera prueba.
