//go:build windows
package main

import (
 "fmt"
 "os"
 "os/exec"
 "path/filepath"
 "syscall"
 "unsafe"
 "strings"
)
var user32 = syscall.NewLazyDLL("user32.dll")
var msgBoxW = user32.NewProc("MessageBoxW")
func message(title, content string, style uint32) int {
 t,_:=syscall.UTF16PtrFromString(title)
 c,_:=syscall.UTF16PtrFromString(content)
 v,_,_:=msgBoxW.Call(0,uintptr(unsafe.Pointer(c)),uintptr(unsafe.Pointer(t)),uintptr(style))
 return int(v)
}
func main() {
 exe,err:=os.Executable()
 if err!=nil {message("Dreamcast Recompiled",fmt.Sprintf("No se puede localizar el ejecutable: %v",err),0x10);return}
 root:=filepath.Dir(exe)
 script:=filepath.Join(root,"launcher","launch.py")
 if st,err:=os.Stat(script);err!=nil||st.IsDir(){
    message("Dreamcast Recompiled","Falta launcher\\launch.py. Extraé el ZIP completo, incluyendo la carpeta launcher/assets.",0x10)
    return
 }
 // Pythonw opens the original Tk/Aqua dashboard with the original assets.
 // Keep the native Win32 GUI separately as a fallback rather than replacing it.
 candidates:=[]struct{name string;prefix []string}{
   {"pyw.exe",[]string{"-3"}},
   {"pythonw.exe",nil},
   {"py.exe",[]string{"-3"}},
   {"python.exe",nil},
 }
 var errs []string
 for _,item:=range candidates {
  found,err:=exec.LookPath(item.name)
  if err!=nil {continue}
  args:=append(append([]string{},item.prefix...),script)
  args=append(args,os.Args[1:]...)
  cmd:=exec.Command(found,args...)
  cmd.Dir=root
  cmd.SysProcAttr=&syscall.SysProcAttr{HideWindow:true}
  if err:=cmd.Start();err==nil { _=cmd.Process.Release();return }
  errs=append(errs,fmt.Sprintf("%s: %v",found,err))
 }
 alternative:=filepath.Join(root,"DreamcastRecompiled_NativeAlt.exe")
 if st,err:=os.Stat(alternative);err==nil&&!st.IsDir() {
  prompt:="No se encontró Python 3 con Tkinter para abrir el launcher Aqua original.\n\n"+
          "El proyecto todavía necesita Python para esta interfaz visual.\n\n"+
          "¿Abrir temporalmente el launcher nativo alternativo sin imágenes?"
  if len(errs)>0 {prompt+="\n\n"+strings.Join(errs,"\n")}
  if message("Dreamcast Recompiled – Aqua Launcher",prompt,0x34)==6 {
   c:=exec.Command(alternative,os.Args[1:]...);c.Dir=root
   if err:=c.Start();err!=nil {message("Dreamcast Recompiled",err.Error(),0x10)} else {_=c.Process.Release()}
  }
  return
 }
 message("Dreamcast Recompiled – Aqua Launcher","No está disponible Python 3 con Tkinter. Instálalo para usar el launcher Aqua.\n"+strings.Join(errs,"\n"),0x10)
}
