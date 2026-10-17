//go:build windows

// SGGG portable frontend for DCR v1.2 Rev 1.
// This is a minimal standalone Windows shell around the verified direct-run
// flow; no Python, SH-4 rebuild, embedded game image or external dependencies.
package main

import (
    "fmt"
    "io"
    "os"
    "os/exec"
    "path/filepath"
    "runtime"
    "strings"
    "syscall"
    "time"
    "unicode/utf16"
    "unsafe"
)

const (
    ofnFileMustExist = 0x00001000
    ofnPathMustExist = 0x00000800
    ofnExplorer = 0x00080000
    mbOK = 0x00000000
    mbIconError = 0x00000010
)

var (
    user32 = syscall.NewLazyDLL("user32.dll")
    messageBox = user32.NewProc("MessageBoxW")
    comdlg32 = syscall.NewLazyDLL("comdlg32.dll")
    getOpenFileName = comdlg32.NewProc("GetOpenFileNameW")
)

// OPENFILENAMEW for Windows x64. uintptr preserves native pointer alignment.
type openFilenameW struct {
    structSize uint32
    owner uintptr
    instance uintptr
    filter uintptr
    customFilter uintptr
    maxCustomFilter uint32
    filterIndex uint32
    file uintptr
    maxFile uint32
    fileTitle uintptr
    maxFileTitle uint32
    initialDir uintptr
    title uintptr
    flags uint32
    fileOffset uint16
    fileExtension uint16
    defExt uintptr
    custData uintptr
    hook uintptr
    templateName uintptr
    reserved uintptr
    reservedDword uint32
    flagsEx uint32
}

func showMessage(title, body string) {
    t, _ := syscall.UTF16PtrFromString(title)
    b, _ := syscall.UTF16PtrFromString(body)
    messageBox.Call(0, uintptr(unsafe.Pointer(b)), uintptr(unsafe.Pointer(t)), mbOK|mbIconError)
}

func selectDisc() string {
    filter := append(utf16.Encode([]rune("Dreamcast CDI/ISO/GDI (*.cdi;*.iso;*.gdi)\x00*.cdi;*.iso;*.gdi\x00Todos los archivos (*.*)\x00*.*\x00")), 0)
    title, _ := syscall.UTF16PtrFromString("DreamcastRecompiled v1.2 Rev 1 - Seleccionar SGGG")
    buffer := make([]uint16, 32768)
    ofn := openFilenameW{
        structSize: uint32(unsafe.Sizeof(openFilenameW{})),
        filter: uintptr(unsafe.Pointer(&filter[0])),
        file: uintptr(unsafe.Pointer(&buffer[0])),
        maxFile: uint32(len(buffer)),
        title: uintptr(unsafe.Pointer(title)),
        flags: ofnFileMustExist | ofnPathMustExist | ofnExplorer,
    }
    result, _, _ := getOpenFileName.Call(uintptr(unsafe.Pointer(&ofn)))
    runtime.KeepAlive(filter)
    runtime.KeepAlive(title)
    runtime.KeepAlive(buffer)
    if result == 0 { return "" }
    return syscall.UTF16ToString(buffer)
}

func runStage(log io.Writer, executable, dir string, env []string, args ...string) error {
    fmt.Fprintf(log, "\n>>> %s %s\n", executable, strings.Join(args, " "))
    cmd := exec.Command(executable, args...)
    cmd.Dir = dir
    cmd.Env = env
    cmd.Stdout = log
    cmd.Stderr = log
    if err := cmd.Run(); err != nil { return fmt.Errorf("%s: %w", filepath.Base(executable), err) }
    return nil
}

func checkFiles(root string) error {
    for _, rel := range []string{"bin/DreamcastRuntime.dll", "bin/dcr_game_module.dll", "bin/dreamcast_program.exe", "bin/dc_boot_prepare.exe", "bin/dc_disc_probe.exe", "portable_manifest.txt"} {
        if _, err := os.Stat(filepath.Join(root, filepath.FromSlash(rel))); err != nil { return fmt.Errorf("ZIP incompleto: %s", rel) }
    }
    return nil
}

func launch(disc string) error {
    disc, err := filepath.Abs(disc)
    if err != nil { return err }
    if _, err := os.Stat(disc); err != nil { return fmt.Errorf("imagen no disponible: %w", err) }
    switch strings.ToLower(filepath.Ext(disc)) {
    case ".cdi", ".iso", ".gdi":
    default: return fmt.Errorf("formato no admitido: %s", filepath.Ext(disc))
    }
    self, err := os.Executable()
    if err != nil { return err }
    root := filepath.Dir(self)
    if err := checkFiles(root); err != nil { return err }
    bin := filepath.Join(root, "bin")
    probe := filepath.Join(bin, "dc_disc_probe.exe")
    prepare := filepath.Join(bin, "dc_boot_prepare.exe")
    game := filepath.Join(bin, "dreamcast_program.exe")
    stamp := time.Now().Format("20060102-150405")
    work := filepath.Join(root, "SGGG_LOGS", stamp)
    if err := os.MkdirAll(work, 0755); err != nil { return err }
    logFile := filepath.Join(work, "session.log")
    f, err := os.Create(logFile)
    if err != nil { return err }
    defer f.Close()
    var log io.Writer = f
    fmt.Fprintln(log, "DCR v1.2 Rev 1 SGGG - validated AUTO/AUTO bootstrap sequence")
    fmt.Fprintf(log, "Disc: %s\nSession directory: %s\n", disc, work)
    ip := filepath.Join(work, "IP.BIN")
    boot := filepath.Join(work, "BOOT.DISC.BIN")
    discMap := filepath.Join(work, "disc.map")
    combined := filepath.Join(work, "BOOTSTRAP.BIN")
    childEnv := append([]string{}, os.Environ()...)
    if err = runStage(log, probe, bin, childEnv, disc,
        "--extract-ip="+ip, "--extract-boot="+boot, "--extract-disc-map="+discMap); err != nil {
        return fmt.Errorf("probe fallo: %w\nLog: %s", err, logFile)
    }
    // The 32 KiB IP.BIN must be left alone. The runner's stage mode below is
    // a separate decision based on real physical track geometry. This exact
    // pair of auto modes booted SGGG for the tester while forced plain did not.
    if err = runStage(log, prepare, bin, childEnv, ip, boot,
        "--mode=auto", "--boot-out="+filepath.Join(work, "BOOT.BIN"), "--combined-out="+combined); err != nil {
        return fmt.Errorf("boot prepare fallo: %w\nLog: %s", err, logFile)
    }
    env := append(childEnv,
        "DCR_PLUGIN_DIR="+filepath.Join(bin, "plugins"),
        "DCR_EXTERNAL_RAW_IMAGE="+combined,
        "DCR_EXTERNAL_RAW_BASE=0x8C008000",
        "DCR_GAME_TITLE=SGGG",
        "DCR_WINDOW_TITLE=DreamcastRecompiled v1.2 Rev 1 - SGGG",
        "DCR_STATUS_WINDOW_TITLE=DCR v1.2 Rev 1 - SGGG Diagnostics",
        "DCR_NETWORK_MODE=off",
        "DCR_FPU_METRICS=0",
        "DCR_HOST_SYNC_QUANTUM=262144",
        "DCR_SPG_WAIT_FF=1",
        "DCR_CPP38E_HOT_RECOVERY=1",
    )
    err = runStage(log, game, bin, env,
        "--commercial-boot", "--commercial-boot-stage=auto", "--disc-map="+discMap,
        "--pvr-window", "--pvr-frame-sync", "--pvr-gpu", "--pvr-render-done-scheduled", "--pvr-mt",
        "--fast-dispatch", "--direct-dispatch", "--sh4-tick-batch=256", "--maple-host-input",
        "--aica-arm7", "--aica-arm7-slice=128", "--aica-arm7-boot=32768", "--aica-play",
        "--device-clock-host", "--device-clock-host-max-catchup=4096", "--diag-heartbeat-ms=1000",
        "--perf-profile", "--gdrom-trace")
    if err != nil { return fmt.Errorf("juego finalizo con error: %w\nLog: %s", err, logFile) }
    return nil
}

func main() {
    self, err := os.Executable()
    if err != nil { showMessage("SGGG", err.Error()); return }
    if len(os.Args) > 1 && os.Args[1] == "--self-check" {
        if err := checkFiles(filepath.Dir(self)); err != nil { os.Exit(3) }
        return
    }
    var disc string
    if len(os.Args) > 1 { disc = os.Args[1] } else { disc = selectDisc() }
    if disc == "" { return }
    if err := launch(disc); err != nil { showMessage("SGGG - Error al iniciar", err.Error()) }
}
