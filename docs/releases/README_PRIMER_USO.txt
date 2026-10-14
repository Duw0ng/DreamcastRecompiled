AQUA RC7

1) EXTRAER ZIP en una carpeta nueva.
2) Abrir ABRIR_LAUNCHER.bat. Reutiliza Python local / un Python de RC4/RC5
   si esta instalado, sin descargar nada. Para VMU: ABRIR_LAUNCHER.bat --vmu.
3) COMPILAR_LAUNCHER.bat: necesita Python 3.12, tkinter y PyInstaller ya
   disponibles en Windows. Corrige el error SyntaxError de RC6 y genera
   DreamcastRecompiled.exe + _internal/ + dist/*.zip (DLLs incluidos).
4) Para tester sin Python, entregar ZIP WINDOWS OFFLINE de dist/ o Github
   Actions. NO entregar este ZIP FUENTE afirmando que contiene DLLs.

En Linux el compilador CLI y runtime/plugins compilan, pero NO hay gameplay
validado todavia. Ver docs/platforms/LINUX_PORTABILITY.md.
