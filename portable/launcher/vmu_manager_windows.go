//go:build windows
package main
import("os";"os/exec";"path/filepath")
func openVMUWindow(){
 path:=filepath.Join(exeDir(),"VMU_Manager.exe")
 if _,err:=os.Stat(path);err!=nil {msgbox("No se encontró VMU_Manager.exe. Extraé todo el ZIP.","VMU",MB_OK|MB_ICONINFORMATION);return}
 cmd:=exec.Command(path)
 cmd.Dir=exeDir()
 if err:=cmd.Start();err!=nil {msgbox(err.Error(),"VMU",MB_OK|MB_ICONINFORMATION);return}
 _=cmd.Process.Release()
}
