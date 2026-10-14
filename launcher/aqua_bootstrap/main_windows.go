//go:build windows
package main

import (
 "fmt"
 "os"
 "os/exec"
 "path/filepath"
 "syscall"
 "unsafe"
)

const (
 mbOK = 0x00000000
 mbYesNo = 0x00000004
 mbIconQuestion = 0x00000020
 mbIconError = 0x00000010
 mbIconInfo = 0x00000040
 idYes = 6
)
var user32 = syscall.NewLazyDLL("user32.dll")
var msgBoxW = user32.NewProc("MessageBoxW")
func message(title, body string, flags uintptr) uintptr {
 t, _ := syscall.UTF16PtrFromString(title)
 b, _ := syscall.UTF16PtrFromString(body)
 res, _, _ := msgBoxW.Call(0, uintptr(unsafe.Pointer(b)), uintptr(unsafe.Pointer(t)), flags)
 return res
}
func hasFile(p string) bool { fi, err := os.Stat(p); return err==nil && !fi.IsDir() }
func main() {
 ex, err := os.Executable()
 if err!=nil {message("Dreamcast Recompiled", fmt.Sprintf("No se puede encontrar el ejecutable: %v",err),mbIconError);return}
 root:=filepath.Dir(ex)
 aqua:=filepath.Join(root,"DreamcastRecompiled_Aqua.exe")
 internal:=filepath.Join(root,"_internal")
 if !hasFile(aqua) {
  prepare:=filepath.Join(root,"PREPARAR_AQUA_SIN_PYTHON.bat")
  if !hasFile(prepare) {message("Dreamcast Recompiled", "Falta PREPARAR_AQUA_SIN_PYTHON.bat. Extraé el ZIP completo.",mbIconError);return}
  text:="El launcher Aqua original todavía no está empaquetado en este equipo.\n\n"+
       "¿Prepararlo ahora con una copia privada de Python y PyInstaller?\n"+
       "La primera vez necesita Internet y puede tardar varios minutos.\n"+
       "NO cambia el Python del sistema ni el PATH.\n\n"+
       "Cuando termine, el launcher Aqua abrirá sin Python instalado."
  if message("Dreamcast Recompiled - Preparar Aqua",text,mbYesNo|mbIconQuestion)!=idYes{return}
  // Expose visible console to show download/build progress and actionable errors.
  c:=exec.Command("cmd.exe","/c",prepare)
  c.Dir=root
  c.SysProcAttr=&syscall.SysProcAttr{CreationFlags:0x00000010}
  if err:=c.Run();err!=nil {message("Dreamcast Recompiled",fmt.Sprintf("No se pudo preparar Aqua: %v\n\nEjecutá PREPARAR_AQUA_SIN_PYTHON.bat para ver el diagnóstico.",err),mbIconError);return}
 }
 if !hasFile(aqua) {message("Dreamcast Recompiled", "No se generó DreamcastRecompiled_Aqua.exe. Consultá el log de empaquetado.",mbIconError);return}
 if st,err:=os.Stat(internal);err!=nil || !st.IsDir(){message("Dreamcast Recompiled", "Falta _internal: el paquete Aqua no está completo. Ejecutá el preparador.",mbIconError);return}
 if !hasFile(filepath.Join(root,"run_game.bat")) {message("Dreamcast Recompiled", "Falta run_game.bat: extraé la distribución completa.",mbIconError);return}
 c:=exec.Command(aqua,os.Args[1:]...)
 c.Dir=root
 c.SysProcAttr=&syscall.SysProcAttr{HideWindow:true}
 if err:=c.Start();err!=nil {message("Dreamcast Recompiled",fmt.Sprintf("No se pudo abrir Aqua: %v",err),mbIconError);return}
 _=c.Process.Release()
}
