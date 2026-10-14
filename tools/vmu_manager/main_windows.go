//go:build windows
package main

import (
    "fmt"
    "os"
    "path/filepath"
    "strings"
    "syscall"
    "unsafe"
)

var user32=syscall.NewLazyDLL("user32.dll")
var kernel32=syscall.NewLazyDLL("kernel32.dll")
var procCreate=user32.NewProc("CreateWindowExW")
var procShow=user32.NewProc("ShowWindow")
var procUpdate=user32.NewProc("UpdateWindow")
var procGetMsg=user32.NewProc("GetMessageW")
var procTranslate=user32.NewProc("TranslateMessage")
var procDispatch=user32.NewProc("DispatchMessageW")
var procModule=kernel32.NewProc("GetModuleHandleW")
var procMsgBox=user32.NewProc("MessageBoxW")
var procRegisterClass=user32.NewProc("RegisterClassExW")
var procDefWndProc=user32.NewProc("DefWindowProcW")
var procPostQuit=user32.NewProc("PostQuitMessage")
type wndClass struct {
    Size uint32
    Style uint32
    WndProc uintptr
    ClsExtra int32
    WndExtra int32
    Instance uintptr
    Icon uintptr
    Cursor uintptr
    Background uintptr
    MenuName *uint16
    ClassName *uint16
    SmallIcon uintptr
}
func wndProc(h,m,wp,lp uintptr)uintptr{
    if m==0x0002{procPostQuit.Call(0);return 0}
    v,_,_:=procDefWndProc.Call(h,m,wp,lp);return v
}
func ptr(s string)*uint16{return syscall.StringToUTF16Ptr(s)}
func read16(b []byte,at int)int{return int(b[at])|int(b[at+1])<<8}
func vmureport(path string)string{
    b,err:=os.ReadFile(path)
    if err!=nil {return fmt.Sprintf("No se pudo abrir la memoria:\r\n%s\r\n\r\n%v\r\n\r\nPara analizar otra VMU, arrastrá un archivo .bin sobre VMU_Manager.exe.",path,err)}
    if len(b)!=131072{return fmt.Sprintf("La VMU no tiene el tamaño estándar de 131.072 bytes: %d bytes",len(b))}
    root:=255*512
    formatted:=true
    for _,v:=range b[root:root+16]{if v!=0x55{formatted=false;break}}
    capacity:=200
    dir:=253
    dirs:=13
    if formatted{
        capacity=read16(b,root+0x40+16)
        if capacity<1||capacity>200{capacity=200}
        dir=read16(b,root+0x40+10)
        dirs=read16(b,root+0x40+12)
        if dir>253||dir<200||dirs<1||dirs>13||dir-dirs+1<200 {dir=253;dirs=13}
    }
    fat:=func(i int)int{return read16(b,254*512+i*2)}
    free:=0
    for i:=0;i<capacity;i++{if fat(i)==0xfffc{free++}}
    var out strings.Builder
    fmt.Fprintf(&out,"DREAMCAST RECOMPILED v1.2  /  VMU INSPECTOR\r\n\r\nArchivo: %s\r\nCapacidad: %d bloques x 512 bytes (128 KiB total)\r\n",path,capacity)
    fmt.Fprintf(&out,"Libre: %d bloques / Ocupado: %d bloques\r\nFormato detectado: %v\r\n\r\nARCHIVOS EN EL DIRECTORIO\r\n──────────────────────────────\r\n",free,capacity-free,formatted)
    count:=0
    for i:=dir;i>dir-dirs;i--{
        for slot:=0;slot<16;slot++{
            e:=b[i*512+slot*32:i*512+slot*32+32]
            if e[0]!=0x33&&e[0]!=0xcc{continue}
            first:=read16(e,2);blocks:=read16(e,24)
            name:=strings.TrimSpace(strings.TrimRight(string(e[4:16]),"\x00 "))
            if name==""{name="(sin nombre)"}
            kind:="datos";if e[0]==0x33{kind="juego"}
            msg:="OK";next:=first;seen:=map[int]bool{}
            for n:=0;n<blocks && n<=capacity;n++{
                if next>=capacity||seen[next]{msg="CADENA INVALIDA";break}
                seen[next]=true
                val:=fat(next)
                if n==blocks-1 {if val!=0xfffa{msg="FIN FAT INVALIDO"}} else if val==0xfffa||val==0xfffc{msg="CADENA TRUNCADA";break}
                next=val
            }
            if blocks==0||blocks>capacity{msg="TAMAÑO INVALIDO"}
            fmt.Fprintf(&out,"%02d  %-12s  %-5s  %3d bloques  FAT: %s\r\n",count+1,name,kind,blocks,msg)
            count++
        }
    }
    if count==0{out.WriteString("(No hay archivos de guardado reconocidos)\r\n")}
    out.WriteString("\r\n──────────────────────────────\r\nModo de solo lectura. No se modifica, formatea ni sincroniza la VMU.\r\nLa pantalla LCD en vivo todavía no está conectada al runtime.\r\nPara ver otra imagen, arrastrala a VMU_Manager.exe.\r\nPodés seleccionar y copiar texto con Ctrl+C.")
    return out.String()
}
func main(){
    local:=os.Getenv("LOCALAPPDATA")
    if local==""{local="."}
    f:=filepath.Join(local,"DreamcastRecompiled","system","dreamcast_vmu_a1.bin")
    if len(os.Args)>1{f=os.Args[1]}
    const WS_OVERLAPPEDWINDOW=0x00cf0000
    const WS_VISIBLE=0x10000000
    const WS_CHILD=0x40000000
    const WS_VSCROLL=0x00200000
    const WS_HSCROLL=0x00100000
    const ES_MULTILINE=0x0004
    const ES_READONLY=0x0800
    const ES_AUTOVSCROLL=0x0040
    const WS_EX_CLIENTEDGE=0x00000200
    const SW_SHOW=5
    hinst,_,_:=procModule.Call(0)
    title:=ptr("Dreamcast Recompiled v1.2 | Administrador VMU A1 (solo lectura)")
    classname:=ptr("DCR_VMUMANAGER_WIN")
    wc:=wndClass{Size:uint32(unsafe.Sizeof(wndClass{})),WndProc:syscall.NewCallback(wndProc),Instance:hinst,
      Background:6,ClassName:classname}
    if ret,_,_:=procRegisterClass.Call(uintptr(unsafe.Pointer(&wc)));ret==0{
        procMsgBox.Call(0,uintptr(unsafe.Pointer(ptr("No se pudo registrar la ventana VMU."))),uintptr(unsafe.Pointer(title)),0x10);return
    }
    parent,_,_:=procCreate.Call(0,uintptr(unsafe.Pointer(classname)),uintptr(unsafe.Pointer(title)),WS_OVERLAPPEDWINDOW|WS_VISIBLE,
       130,110,865,610,0,0,hinst,0)
    if parent==0{procMsgBox.Call(0,uintptr(unsafe.Pointer(ptr("No se pudo abrir la ventana VMU."))),uintptr(unsafe.Pointer(title)),0x10);return}
    report:=ptr(vmureport(f))
    procCreate.Call(WS_EX_CLIENTEDGE,uintptr(unsafe.Pointer(ptr("EDIT"))),uintptr(unsafe.Pointer(report)),
      WS_VISIBLE|WS_CHILD|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
      14,15,825,550,parent,0,hinst,0)
    procShow.Call(parent,SW_SHOW)
    procUpdate.Call(parent)
    var msg [8]uintptr
    for {r,_,_:=procGetMsg.Call(uintptr(unsafe.Pointer(&msg[0])),0,0,0);if int32(r)<=0{break};procTranslate.Call(uintptr(unsafe.Pointer(&msg[0])));procDispatch.Call(uintptr(unsafe.Pointer(&msg[0])))}
}
