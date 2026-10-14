//go:build windows

package main

import (
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"runtime/debug"
	"strings"
	"sync"
	"syscall"
	"time"
	"unsafe"
)

const launcherVersion = "v1.2 AQUA Testers RC1 + ABI1 Incremental"

const (
	WS_FIXED           = 0x00CA0000 // caption + sysmenu + minimize; fixed layout
	WS_VISIBLE         = 0x10000000
	WS_CHILD           = 0x40000000
	WS_TABSTOP         = 0x00010000
	ES_AUTOHSCROLL     = 0x0080
	BS_PUSHBUTTON      = 0x00000000
	BS_AUTOCHECKBOX    = 0x00000003
	CBS_DROPDOWNLIST   = 0x0003
	SW_SHOW            = 5
	WM_DESTROY         = 0x0002
	WM_CLOSE           = 0x0010
	WM_COMMAND         = 0x0111
	WM_TIMER           = 0x0113
	WM_PAINT           = 0x000F
	WM_ERASEBKGND      = 0x0014
	BM_GETCHECK        = 0x00F0
	BM_SETCHECK        = 0x00F1
	BST_CHECKED        = 1
	CB_ADDSTRING       = 0x0143
	CB_GETCURSEL       = 0x0147
	CB_SETCURSEL       = 0x014E
	CB_RESETCONTENT    = 0x014B
	CBN_SELCHANGE      = 1
	OFN_FILEMUSTEXIST  = 0x00001000
	OFN_PATHMUSTEXIST  = 0x00000800
	MB_OK              = 0x00000000
	MB_ICONERROR       = 0x00000010
	MB_ICONINFORMATION = 0x00000040
	WM_APP_STATUS      = 0x8001
	WM_APP_RESULT      = 0x8002
	JOY_RETURNALL      = 0x00FF
	DT_CENTER          = 0x0001
	DT_VCENTER         = 0x0004
	DT_SINGLELINE      = 0x0020
	TRANSPARENT        = 1
	DEFAULT_GUI_FONT   = 17
	PS_SOLID           = 0
	SRCCOPY            = 0x00CC0020
)

const (
	idDiscEdit      = 101
	idBrowse        = 102
	idRender        = 103
	idAudio         = 104
	idPerf          = 105
	idDebug         = 106
	idProfile       = 107
	idClock         = 108
	idPad           = 109
	idPlay          = 110
	idLogs          = 111
	idStatus        = 112
	idMapleTrace    = 113
	idGDROMTrace    = 114
	idNetwork       = 115
	idBuildMode     = 116
	idAdvanced      = 117
	idOpenGameCache = 118
	idNetStatus     = 119
	idHome          = 120

	idCtlPort    = 201
	idCtlDevice  = 202
	idCtlSlot1   = 203
	idCtlSlot2   = 204
	idCtlStatus  = 205
	idCtlRefresh = 206
	idCtlRumble  = 207
	idCtlSave    = 208
	idCtlVisual  = 209
)

var (
	user32   = syscall.NewLazyDLL("user32.dll")
	kernel32 = syscall.NewLazyDLL("kernel32.dll")
	comdlg32 = syscall.NewLazyDLL("comdlg32.dll")
	shell32  = syscall.NewLazyDLL("shell32.dll")
	winmm    = syscall.NewLazyDLL("winmm.dll")
	gdi32    = syscall.NewLazyDLL("gdi32.dll")

	pRegisterClassExW       = user32.NewProc("RegisterClassExW")
	pLoadIconW              = user32.NewProc("LoadIconW")
	pCreateWindowExW        = user32.NewProc("CreateWindowExW")
	pDefWindowProcW         = user32.NewProc("DefWindowProcW")
	pShowWindow             = user32.NewProc("ShowWindow")
	pUpdateWindow           = user32.NewProc("UpdateWindow")
	pGetMessageW            = user32.NewProc("GetMessageW")
	pTranslateMessage       = user32.NewProc("TranslateMessage")
	pDispatchMessageW       = user32.NewProc("DispatchMessageW")
	pPostQuitMessage        = user32.NewProc("PostQuitMessage")
	pPostMessageW           = user32.NewProc("PostMessageW")
	pSendMessageW           = user32.NewProc("SendMessageW")
	pGetWindowTextW         = user32.NewProc("GetWindowTextW")
	pSetWindowTextW         = user32.NewProc("SetWindowTextW")
	pMessageBoxW            = user32.NewProc("MessageBoxW")
	pEnableWindow           = user32.NewProc("EnableWindow")
	pDestroyWindow          = user32.NewProc("DestroyWindow")
	pSetTimer               = user32.NewProc("SetTimer")
	pKillTimer              = user32.NewProc("KillTimer")
	pGetAsyncKeyState       = user32.NewProc("GetAsyncKeyState")
	pGetCursorPos           = user32.NewProc("GetCursorPos")
	pInvalidateRect         = user32.NewProc("InvalidateRect")
	pBeginPaint             = user32.NewProc("BeginPaint")
	pEndPaint               = user32.NewProc("EndPaint")
	pGetClientRect          = user32.NewProc("GetClientRect")
	pFillRect               = user32.NewProc("FillRect")
	pDrawTextW              = user32.NewProc("DrawTextW")
	pGetModuleHandleW       = kernel32.NewProc("GetModuleHandleW")
	pGetOpenFileNameW       = comdlg32.NewProc("GetOpenFileNameW")
	pShellExecuteW          = shell32.NewProc("ShellExecuteW")
	pJoyGetNumDevs          = winmm.NewProc("joyGetNumDevs")
	pJoyGetPosExW           = winmm.NewProc("joyGetPosEx")
	pJoyGetDevCapsW         = winmm.NewProc("joyGetDevCapsW")
	pCreateSolidBrush       = gdi32.NewProc("CreateSolidBrush")
	pCreatePen              = gdi32.NewProc("CreatePen")
	pSelectObject           = gdi32.NewProc("SelectObject")
	pDeleteObject           = gdi32.NewProc("DeleteObject")
	pEllipse                = gdi32.NewProc("Ellipse")
	pRoundRect              = gdi32.NewProc("RoundRect")
	pRectangle              = gdi32.NewProc("Rectangle")
	pMoveToEx               = gdi32.NewProc("MoveToEx")
	pLineTo                 = gdi32.NewProc("LineTo")
	pSetBkMode              = gdi32.NewProc("SetBkMode")
	pSetTextColor           = gdi32.NewProc("SetTextColor")
	pGetStockObject         = gdi32.NewProc("GetStockObject")
	pCreateCompatibleDC     = gdi32.NewProc("CreateCompatibleDC")
	pCreateCompatibleBitmap = gdi32.NewProc("CreateCompatibleBitmap")
	pDeleteDC               = gdi32.NewProc("DeleteDC")
	pBitBlt                 = gdi32.NewProc("BitBlt")
	pBeginPath              = gdi32.NewProc("BeginPath")
	pEndPath                = gdi32.NewProc("EndPath")
	pCloseFigure            = gdi32.NewProc("CloseFigure")
	pFillPath               = gdi32.NewProc("FillPath")
	pStrokePath             = gdi32.NewProc("StrokePath")
	pPolyBezierTo           = gdi32.NewProc("PolyBezierTo")
)

type WNDCLASSEX struct {
	CbSize, Style       uint32
	WndProc             uintptr
	ClsExtra, WndExtra  int32
	Instance, Icon      uintptr
	Cursor, Background  uintptr
	MenuName, ClassName *uint16
	IconSm              uintptr
}

type MSG struct {
	Hwnd           uintptr
	Message        uint32
	WParam, LParam uintptr
	Time           uint32
	PtX, PtY       int32
	LPrivate       uint32
}

type OPENFILENAME struct {
	LStructSize       uint32
	HwndOwner         uintptr
	HInstance         uintptr
	LpstrFilter       *uint16
	LpstrCustomFilter *uint16
	NMaxCustFilter    uint32
	NFilterIndex      uint32
	LpstrFile         *uint16
	NMaxFile          uint32
	LpstrFileTitle    *uint16
	NMaxFileTitle     uint32
	LpstrInitialDir   *uint16
	LpstrTitle        *uint16
	Flags             uint32
	NFileOffset       uint16
	NFileExtension    uint16
	LpstrDefExt       *uint16
	LCustData         uintptr
	LpfnHook          uintptr
	LpTemplateName    *uint16
	PvReserved        uintptr
	DwReserved        uint32
	FlagsEx           uint32
}

type POINT struct{ X, Y int32 }

type RECT struct{ Left, Top, Right, Bottom int32 }

type PAINTSTRUCT struct {
	Hdc         uintptr
	Erase       int32
	RcPaint     RECT
	Restore     int32
	IncUpdate   int32
	RgbReserved [32]byte
}

type JOYINFOEX struct {
	Size, Flags                                      uint32
	X, Y, Z, R, U, V                                 uint32
	Buttons, ButtonNumber, POV, Reserved1, Reserved2 uint32
}

type JOYCAPSW struct {
	Mid, Pid                           uint16
	Pname                              [32]uint16
	Xmin, Xmax, Ymin, Ymax, Zmin, Zmax uint32
	NumButtons, PeriodMin, PeriodMax   uint32
	Rmin, Rmax, Umin, Umax, Vmin, Vmax uint32
	Caps, MaxAxes, NumAxes, MaxButtons uint32
	RegKey                             [32]uint16
	OEMVxD                             [260]uint16
}

type XINPUT_GAMEPAD struct {
	Buttons                            uint16
	LeftTrigger, RightTrigger          uint8
	ThumbLX, ThumbLY, ThumbRX, ThumbRY int16
}
type XINPUT_STATE struct {
	PacketNumber uint32
	Gamepad      XINPUT_GAMEPAD
}
type XINPUT_VIBRATION struct{ LeftMotorSpeed, RightMotorSpeed uint16 }

type MaplePortConfig struct {
	Device string `json:"device"`
	Slot1  string `json:"slot1"`
	Slot2  string `json:"slot2"`
}

type InputDevice struct{ Spec, Label string }

type Settings struct {
	Disc             string             `json:"disc"`
	Library          []string           `json:"library"`
	Renderer         int                `json:"renderer"`
	AudioMode        int                `json:"audio_mode"` // 0=full, 1=basic, 2=disabled
	Perf             bool               `json:"perf"`
	Debug            bool               `json:"debug"`
	PVRProfile       bool               `json:"pvr_profile"`
	MapleTrace       bool               `json:"maple_trace"`
	GDROMTrace       bool               `json:"gdrom_trace"`
	Clock            int                `json:"clock"`
	NetworkMode      int                `json:"network_mode"` // 0 Auto, 1 DCNet PPP, 2 offline
	BuildMode        int                `json:"build_mode"`   // 0 incremental, 1 reuse, 2 recompile all
	FPU              int                `json:"fpu"`
	VideoResolution  int                `json:"video_resolution"` // native, 960p, 1080p, 1440p, desktop
	VideoAspect      int                `json:"video_aspect"`     // 4:3 or stretched 16:9
	VideoBackend     int                `json:"video_backend"`    // 0 native DXGI; 1 compatibility GDI
	Heartbeats       bool               `json:"heartbeats"`
	ShowConsole      bool               `json:"show_console"`
	NativeSeeds      bool               `json:"native_seeds"`
	NativePatches    bool               `json:"native_patches"`
	Interpreter      bool               `json:"interpreter"`
	InterpTrace      bool               `json:"interp_trace"`
	CodeRelocTrace   bool               `json:"code_reloc_trace"`
	SPGFastForward   bool               `json:"spg_fast_forward"`
	HotRecovery      bool               `json:"hot_recovery"`
	AutoPortableSync bool               `json:"auto_portable_sync"`
	RebuildRuntime   bool               `json:"rebuild_runtime"`
	RebuildNetwork   bool               `json:"rebuild_network"`
	RebuildMaple     bool               `json:"rebuild_maple"`
	RebuildGDROM     bool               `json:"rebuild_gdrom"`
	HotPC            bool               `json:"hot_pc"`
	MaplePorts       [4]MaplePortConfig `json:"maple_ports"`
}

type LaunchSnapshot struct {
	Disc            string
	Renderer        int
	AudioMode       int
	Perf            bool
	Debug           bool
	PVRProfile      bool
	MapleTrace      bool
	GDROMTrace      bool
	Clock           int
	NetworkMode     int
	BuildMode       int
	VideoResolution int
	VideoAspect     int
	VideoBackend    int
	Options         LaunchOptions
	MaplePorts      [4]MaplePortConfig
}

var (
	hwndMain             uintptr
	controls             = map[int]uintptr{}
	settings             = Settings{Renderer: 0, AudioMode: 0, Clock: 0, NativeSeeds: true, NativePatches: true, Interpreter: true, SPGFastForward: true, HotRecovery: true}
	wndProcPtr           uintptr
	ctlWndProcPtr        uintptr
	ctlVisualWndProcPtr  uintptr
	hwndAdvanced         uintptr
	advWndProcPtr        uintptr
	advControls          = map[int]uintptr{}
	hwndController       uintptr
	hwndControllerVisual uintptr
	ctlControls          = map[int]uintptr{}
	ctlPort              int
	ctlDevices           []InputDevice
	ctlVisualState       DCVisualState
	ctlVisualHaveState   bool
	ctlVisualPort        = -1
	ctlLastStatusText    string
	ctlHapticsStatus     string
	ctlLastHapticSpec    string

	xinputOnce         sync.Once
	xinputDLL          *syscall.DLL
	xinputGetStateProc *syscall.Proc
	xinputSetStateProc *syscall.Proc

	asyncMu     sync.Mutex
	asyncStatus string
	asyncErr    string
	asyncOK     bool
	playBusy    bool
)

func w(s string) *uint16 { p, _ := syscall.UTF16PtrFromString(s); return p }

func appDataDir() string {
	d := os.Getenv("LOCALAPPDATA")
	if d == "" {
		if x, err := os.UserConfigDir(); err == nil {
			d = x
		}
	}
	if d == "" {
		d = exeDir()
	}
	d = filepath.Join(d, "DreamcastRecompiled")
	_ = os.MkdirAll(d, 0755)
	return d
}

func launcherLogPath() string { return filepath.Join(appDataDir(), "launcher-v0.7.log") }

func appendLog(format string, a ...any) {
	f, err := os.OpenFile(launcherLogPath(), os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
	if err != nil {
		return
	}
	defer f.Close()
	_, _ = fmt.Fprintf(f, "%s "+format+"\n", append([]any{time.Now().Format("2006-01-02 15:04:05.000")}, a...)...)
}

func logRecovered(where string, v any) {
	appendLog("PANIC %s: %v\n%s", where, v, string(debug.Stack()))
}

func msgbox(text, title string, flags uintptr) {
	pMessageBoxW.Call(hwndMain, uintptr(unsafe.Pointer(w(text))), uintptr(unsafe.Pointer(w(title))), flags)
}
func setText(h uintptr, s string) {
	if h != 0 {
		pSetWindowTextW.Call(h, uintptr(unsafe.Pointer(w(s))))
	}
}
func getText(h uintptr) string {
	if h == 0 {
		return ""
	}
	buf := make([]uint16, 32768)
	n, _, _ := pGetWindowTextW.Call(h, uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)))
	return syscall.UTF16ToString(buf[:n])
}
func send(h uintptr, m uint32, wp, lp uintptr) uintptr {
	if h == 0 {
		return 0
	}
	r, _, _ := pSendMessageW.Call(h, uintptr(m), wp, lp)
	return r
}
func checked(id int) bool { return send(controls[id], BM_GETCHECK, 0, 0) == BST_CHECKED }
func setChecked(id int, v bool) {
	var x uintptr
	if v {
		x = BST_CHECKED
	}
	send(controls[id], BM_SETCHECK, x, 0)
}
func comboIndex(id int) int {
	i := int(send(controls[id], CB_GETCURSEL, 0, 0))
	if i < 0 {
		return 0
	}
	return i
}
func setCombo(id, idx int) { send(controls[id], CB_SETCURSEL, uintptr(idx), 0) }
func enable(id int, v bool) {
	if controls[id] != 0 {
		var x uintptr
		if v {
			x = 1
		}
		pEnableWindow.Call(controls[id], x)
	}
}

func exeDir() string {
	e, err := os.Executable()
	if err != nil {
		return "."
	}
	return filepath.Dir(e)
}
func configPath() string { return filepath.Join(appDataDir(), "launcher.json") }

func normalizePortConfig(port int, c MaplePortConfig) MaplePortConfig {
	if c.Device == "" {
		if port == 0 {
			c.Device = "auto"
		} else {
			c.Device = "none"
		}
	}
	validDevice := c.Device == "auto" || c.Device == "none" || c.Device == "keyboard_mouse" || strings.HasPrefix(c.Device, "xinput:") || strings.HasPrefix(c.Device, "winmm:")
	if !validDevice {
		c.Device = "none"
	}
	validSlot := func(v string) bool { return v == "empty" || v == "vmu" || v == "purupuru" }
	if c.Slot1 == "" {
		if port == 0 {
			c.Slot1 = "vmu"
		} else {
			c.Slot1 = "empty"
		}
	}
	if c.Slot2 == "" {
		if port == 0 {
			c.Slot2 = "purupuru"
		} else {
			c.Slot2 = "empty"
		}
	}
	if !validSlot(c.Slot1) {
		c.Slot1 = "empty"
	}
	if !validSlot(c.Slot2) {
		c.Slot2 = "empty"
	}
	// cpp36b has persistent VMU storage for A1; keep invalid VMU placements out of the saved config.
	if !(port == 0 && c.Slot1 == "vmu") && c.Slot1 == "vmu" {
		c.Slot1 = "empty"
	}
	if c.Slot2 == "vmu" {
		c.Slot2 = "empty"
	}
	return c
}

func normalizeSettings() {
	if settings.Renderer < 0 || settings.Renderer > 2 {
		settings.Renderer = 0
	}
	if settings.AudioMode < 0 || settings.AudioMode > 2 {
		settings.AudioMode = 0
	}
	if settings.Clock < 0 || settings.Clock > 1 {
		settings.Clock = 0
	}
	if settings.NetworkMode < 0 || settings.NetworkMode > 2 {
		settings.NetworkMode = 0
	}
	if settings.BuildMode < 0 || settings.BuildMode > 2 {
		settings.BuildMode = 0
	}
	if settings.FPU < 0 || settings.FPU > 1 {
		settings.FPU = 0
	}
	if settings.VideoResolution < 0 || settings.VideoResolution > 4 {
		settings.VideoResolution = 0
	}
	if settings.VideoAspect < 0 || settings.VideoAspect > 1 {
		settings.VideoAspect = 0
	}
	if settings.VideoBackend < 0 || settings.VideoBackend > 1 {
		settings.VideoBackend = 0
	}
	for i := 0; i < 4; i++ {
		settings.MaplePorts[i] = normalizePortConfig(i, settings.MaplePorts[i])
	}
}

func loadSettings() {
	b, err := os.ReadFile(configPath())
	if err != nil {
		migrateLegacyAquaSettings()
		normalizeSettings()
		return
	}
	_ = json.Unmarshal(b, &settings)
	// v0.1 compatibility: old config stored only a boolean "audio".
	var raw map[string]json.RawMessage
	if json.Unmarshal(b, &raw) == nil {
		if _, ok := raw["audio_mode"]; !ok {
			var old struct {
				Audio *bool `json:"audio"`
			}
			if json.Unmarshal(b, &old) == nil && old.Audio != nil && !*old.Audio {
				settings.AudioMode = 2
			}
		}
	}
	normalizeSettings()
}

func readSettingsFromUI() Settings {
	result := settings
	result.Disc = strings.TrimSpace(getText(controls[idDiscEdit]))
	result.Renderer = comboIndex(idRender)
	result.AudioMode = comboIndex(idAudio)
	result.Perf = checked(idPerf)
	result.Debug = checked(idDebug)
	result.PVRProfile = checked(idProfile)
	result.MapleTrace = checked(idMapleTrace)
	result.GDROMTrace = checked(idGDROMTrace)
	result.Clock = comboIndex(idClock)
	result.NetworkMode = comboIndex(idNetwork)
	result.BuildMode = comboIndex(idBuildMode)
	return result
}

func saveSettings() {
	if hwndMain == 0 || controls[idDiscEdit] == 0 {
		return
	}
	settings = readSettingsFromUI()
	normalizeSettings()
	b, err := json.MarshalIndent(settings, "", "  ")
	if err != nil {
		return
	}
	p := configPath()
	tmp := p + ".tmp"
	if os.WriteFile(tmp, b, 0644) == nil {
		_ = os.Rename(tmp, p)
	}
}

func create(cls, text string, style uint32, x, y, cx, cy int32, id int) uintptr {
	h, _, callErr := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(w(cls))), uintptr(unsafe.Pointer(w(text))), uintptr(style), uintptr(x), uintptr(y), uintptr(cx), uintptr(cy), hwndMain, uintptr(id), 0, 0)
	if h == 0 {
		appendLog("CreateWindowEx failed class=%s id=%d err=%v", cls, id, callErr)
	}
	if id != 0 {
		controls[id] = h
	}
	return h
}

func createFor(parent uintptr, store map[int]uintptr, cls, text string, style uint32, x, y, cx, cy int32, id int) uintptr {
	h, _, callErr := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(w(cls))), uintptr(unsafe.Pointer(w(text))), uintptr(style), uintptr(x), uintptr(y), uintptr(cx), uintptr(cy), parent, uintptr(id), 0, 0)
	if h == 0 {
		appendLog("CreateWindowEx controller failed class=%s id=%d err=%v", cls, id, callErr)
	}
	if id != 0 {
		store[id] = h
	}
	return h
}

func comboIndexHandle(h uintptr) int {
	i := int(send(h, CB_GETCURSEL, 0, 0))
	if i < 0 {
		return 0
	}
	return i
}
func comboReset(h uintptr)             { send(h, CB_RESETCONTENT, 0, 0) }
func comboAdd(h uintptr, label string) { send(h, CB_ADDSTRING, 0, uintptr(unsafe.Pointer(w(label)))) }

func initXInput() bool {
	xinputOnce.Do(func() {
		for _, name := range []string{"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"} {
			d, err := syscall.LoadDLL(name)
			if err != nil {
				continue
			}
			g, eg := d.FindProc("XInputGetState")
			st, es := d.FindProc("XInputSetState")
			if eg == nil && es == nil {
				xinputDLL, xinputGetStateProc, xinputSetStateProc = d, g, st
				return
			}
			_ = d.Release()
		}
	})
	return xinputDLL != nil && xinputGetStateProc != nil
}

func xinputState(index int) (XINPUT_STATE, bool) {
	var st XINPUT_STATE
	if index < 0 || index > 3 || !initXInput() {
		return st, false
	}
	r, _, _ := xinputGetStateProc.Call(uintptr(index), uintptr(unsafe.Pointer(&st)))
	return st, r == 0
}
func xinputRumble(index int, low, high uint16) bool {
	if index < 0 || index > 3 || !initXInput() || xinputSetStateProc == nil {
		return false
	}
	v := XINPUT_VIBRATION{LeftMotorSpeed: low, RightMotorSpeed: high}
	r, _, _ := xinputSetStateProc.Call(uintptr(index), uintptr(unsafe.Pointer(&v)))
	return r == 0
}

func maplePluginCandidates() []string {
	out := []string{}
	if d := strings.TrimSpace(os.Getenv("DCR_PLUGIN_DIR")); d != "" {
		out = append(out, filepath.Join(d, "dcr_maple.dll"))
	}
	base := exeDir()
	out = append(out,
		filepath.Join(base, "bin", "plugins", "dcr_maple.dll"),
		filepath.Join(base, "plugins", "dcr_maple.dll"),
		filepath.Join(base, "dcr_maple.dll"),
	)
	return out
}

func maplePluginRumble(spec string, low, high uint16) (bool, string) {
	for _, path := range maplePluginCandidates() {
		if _, err := os.Stat(path); err != nil {
			continue
		}
		dll, err := syscall.LoadDLL(path)
		if err != nil {
			continue
		}
		proc, err := dll.FindProc("dcr_maple_test_host_rumble_v1")
		if err != nil {
			_ = dll.Release()
			continue
		}
		sp, _ := syscall.BytePtrFromString(spec)
		buf := make([]byte, 256)
		r, _, _ := proc.Call(
			uintptr(unsafe.Pointer(sp)), uintptr(low), uintptr(high),
			uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)),
		)
		_ = dll.Release()
		msg := strings.TrimSpace(string(buf[:bytesIndexZero(buf)]))
		if msg == "" {
			if r != 0 {
				msg = "Rumble sent through dcr_maple.dll"
			} else {
				msg = "Haptics backend unavailable"
			}
		}
		return r != 0, msg
	}
	// Development fallback: XInput can still be tested if the plugin DLL is not beside the launcher.
	if i, ok := parseSpecIndex(spec, "xinput:"); ok {
		if xinputRumble(i, low, high) {
			return true, "XInput rumble active (launcher fallback)"
		}
	}
	return false, "dcr_maple.dll with Sony haptics was not found beside this launcher"
}

func maplePluginProbeHaptics(spec string) (bool, string) {
	for _, path := range maplePluginCandidates() {
		if _, err := os.Stat(path); err != nil {
			continue
		}
		dll, err := syscall.LoadDLL(path)
		if err != nil {
			continue
		}
		proc, err := dll.FindProc("dcr_maple_probe_host_haptics_v1")
		if err != nil {
			_ = dll.Release()
			continue
		}
		sp, _ := syscall.BytePtrFromString(spec)
		buf := make([]byte, 256)
		r, _, _ := proc.Call(uintptr(unsafe.Pointer(sp)), uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)))
		_ = dll.Release()
		msg := strings.TrimSpace(string(buf[:bytesIndexZero(buf)]))
		return r != 0, msg
	}
	return false, "dcr_maple.dll haptics probe unavailable"
}

func bytesIndexZero(b []byte) int {
	for i, v := range b {
		if v == 0 {
			return i
		}
	}
	return len(b)
}

func winmmState(index int) (JOYINFOEX, bool) {
	var j JOYINFOEX
	j.Size = uint32(unsafe.Sizeof(j))
	j.Flags = JOY_RETURNALL
	r, _, _ := pJoyGetPosExW.Call(uintptr(index), uintptr(unsafe.Pointer(&j)))
	return j, r == 0
}
func winmmName(index int) string {
	var c JOYCAPSW
	r, _, _ := pJoyGetDevCapsW.Call(uintptr(index), uintptr(unsafe.Pointer(&c)), uintptr(unsafe.Sizeof(c)))
	if r != 0 {
		return fmt.Sprintf("WinMM Controller %d", index+1)
	}
	n := syscall.UTF16ToString(c.Pname[:])
	if strings.TrimSpace(n) == "" {
		return fmt.Sprintf("WinMM Controller %d", index+1)
	}
	return n
}

func enumerateInputDevices() []InputDevice {
	out := []InputDevice{
		{Spec: "auto", Label: "Automatic / compatibility"},
		{Spec: "none", Label: "Disconnected"},
		{Spec: "keyboard_mouse", Label: "Keyboard + Mouse (combined)"},
	}
	for i := 0; i < 4; i++ {
		if _, ok := xinputState(i); ok {
			out = append(out, InputDevice{Spec: fmt.Sprintf("xinput:%d", i), Label: fmt.Sprintf("XInput #%d — independent gamepad", i+1)})
		}
	}
	n, _, _ := pJoyGetNumDevs.Call()
	for i := 0; i < int(n); i++ {
		if _, ok := winmmState(i); ok {
			out = append(out, InputDevice{Spec: fmt.Sprintf("winmm:%d", i), Label: fmt.Sprintf("DirectInput/WinMM #%d — %s", i+1, winmmName(i))})
		}
	}
	return out
}

func parseSpecIndex(spec, prefix string) (int, bool) {
	if !strings.HasPrefix(spec, prefix) {
		return 0, false
	}
	var n int
	if _, err := fmt.Sscanf(strings.TrimPrefix(spec, prefix), "%d", &n); err != nil {
		return 0, false
	}
	return n, true
}
func keyDown(vk uintptr) bool {
	r, _, _ := pGetAsyncKeyState.Call(vk)
	return (uint16(r) & 0x8000) != 0
}

type DCVisualState struct {
	Connected bool
	Buttons   uint16
	LTrig     uint8
	RTrig     uint8
	JoyX      int8
	JoyY      int8
	Backend   string
}

func absInt(v int) int {
	if v < 0 {
		return -v
	}
	return v
}
func clampInt(v, lo, hi int) int {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}
func dcAxisXInput(v int16, invert bool) int8 {
	x := int(v)
	if invert {
		x = -x
	}
	if absInt(x) < 5898 {
		return 0
	}
	return int8(clampInt(x/256, -128, 127))
}
func dcAxisWinMM(v uint32) int8 {
	x := int(v) - 32767
	if absInt(x) < 5898 {
		return 0
	}
	return int8(clampInt(x/256, -128, 127))
}

func dreamcastVisualState(spec string, port int) DCVisualState {
	var out DCVisualState
	if spec == "none" {
		out.Backend = "Disconnected"
		return out
	}
	if spec == "keyboard_mouse" {
		out.Connected = true
		out.Backend = "Keyboard + Mouse"
		if keyDown('J') || keyDown(0x20) || keyDown('Z') || keyDown(0x01) {
			out.Buttons |= 1 << 2
		}
		if keyDown('K') || keyDown('X') || keyDown(0x02) {
			out.Buttons |= 1 << 1
		}
		if keyDown('U') || keyDown('C') || keyDown(0x04) {
			out.Buttons |= 1 << 10
		}
		if keyDown('I') || keyDown('V') || keyDown(0x05) {
			out.Buttons |= 1 << 9
		}
		if keyDown(0x0D) || keyDown(0x06) {
			out.Buttons |= 1 << 3
		}
		if keyDown(0x26) {
			out.Buttons |= 1 << 4
		}
		if keyDown(0x28) {
			out.Buttons |= 1 << 5
		}
		if keyDown(0x25) {
			out.Buttons |= 1 << 6
		}
		if keyDown(0x27) {
			out.Buttons |= 1 << 7
		}
		if keyDown('Q') {
			out.LTrig = 255
		}
		if keyDown('E') {
			out.RTrig = 255
		}
		if keyDown('A') {
			out.JoyX = -127
		} else if keyDown('D') {
			out.JoyX = 127
		}
		if keyDown('W') {
			out.JoyY = -127
		} else if keyDown('S') {
			out.JoyY = 127
		}
		return out
	}
	device := port
	kind := spec
	if i, ok := parseSpecIndex(spec, "xinput:"); ok {
		kind = "xinput"
		device = i
	}
	if i, ok := parseSpecIndex(spec, "winmm:"); ok {
		kind = "winmm"
		device = i
	}
	if kind == "auto" || kind == "xinput" {
		if st, ok := xinputState(device); ok {
			g := st.Gamepad
			out.Connected = true
			out.Backend = fmt.Sprintf("XInput #%d", device+1)
			const (
				xUp        = 0x0001
				xDown      = 0x0002
				xLeft      = 0x0004
				xRight     = 0x0008
				xStart     = 0x0010
				xLShoulder = 0x0100
				xRShoulder = 0x0200
				xA         = 0x1000
				xB         = 0x2000
				xX         = 0x4000
				xY         = 0x8000
			)
			b := g.Buttons
			if b&xA != 0 {
				out.Buttons |= 1 << 2
			}
			if b&xB != 0 {
				out.Buttons |= 1 << 1
			}
			if b&xX != 0 {
				out.Buttons |= 1 << 10
			}
			if b&xY != 0 {
				out.Buttons |= 1 << 9
			}
			if b&xStart != 0 {
				out.Buttons |= 1 << 3
			}
			if b&xUp != 0 {
				out.Buttons |= 1 << 4
			}
			if b&xDown != 0 {
				out.Buttons |= 1 << 5
			}
			if b&xLeft != 0 {
				out.Buttons |= 1 << 6
			}
			if b&xRight != 0 {
				out.Buttons |= 1 << 7
			}
			// The current Maple mapping exposes host shoulder buttons as the optional C/Z bits.
			if b&xLShoulder != 0 {
				out.Buttons |= 1 << 0
			}
			if b&xRShoulder != 0 {
				out.Buttons |= 1 << 8
			}
			out.LTrig = g.LeftTrigger
			out.RTrig = g.RightTrigger
			out.JoyX = dcAxisXInput(g.ThumbLX, false)
			out.JoyY = dcAxisXInput(g.ThumbLY, true)
			return out
		}
		if kind == "xinput" {
			out.Backend = fmt.Sprintf("XInput #%d disconnected", device+1)
			return out
		}
	}
	if kind == "auto" || kind == "winmm" {
		if j, ok := winmmState(device); ok {
			out.Connected = true
			out.Backend = fmt.Sprintf("%s", winmmName(device))
			bd := func(i int) bool { return i >= 0 && i < 32 && (j.Buttons&(1<<uint(i))) != 0 }
			if bd(1) {
				out.Buttons |= 1 << 2
			}
			if bd(2) {
				out.Buttons |= 1 << 1
			}
			if bd(0) {
				out.Buttons |= 1 << 10
			}
			if bd(3) {
				out.Buttons |= 1 << 9
			}
			if bd(9) {
				out.Buttons |= 1 << 3
			}
			if bd(4) {
				out.Buttons |= 1 << 0
			}
			if bd(5) {
				out.Buttons |= 1 << 8
			}
			if j.POV != 0xFFFF {
				pov := j.POV % 36000
				if pov >= 31500 || pov <= 4500 {
					out.Buttons |= 1 << 4
				}
				if pov >= 4500 && pov <= 13500 {
					out.Buttons |= 1 << 7
				}
				if pov >= 13500 && pov <= 22500 {
					out.Buttons |= 1 << 5
				}
				if pov >= 22500 && pov <= 31500 {
					out.Buttons |= 1 << 6
				}
			}
			if bd(6) {
				out.LTrig = 255
			}
			if bd(7) {
				out.RTrig = 255
			}
			out.JoyX = dcAxisWinMM(j.X)
			out.JoyY = dcAxisWinMM(j.Y)
			return out
		}
		out.Backend = fmt.Sprintf("DirectInput/WinMM #%d disconnected", device+1)
		return out
	}
	out.Backend = spec
	return out
}

func rgb(r, g, b byte) uintptr { return uintptr(uint32(r) | uint32(g)<<8 | uint32(b)<<16) }

func drawTextCentered(hdc uintptr, text string, rc RECT, color uintptr) {
	u, _ := syscall.UTF16FromString(text)
	if len(u) == 0 {
		return
	}
	pSetBkMode.Call(hdc, TRANSPARENT)
	pSetTextColor.Call(hdc, color)
	font, _, _ := pGetStockObject.Call(DEFAULT_GUI_FONT)
	old, _, _ := pSelectObject.Call(hdc, font)
	pDrawTextW.Call(hdc, uintptr(unsafe.Pointer(&u[0])), uintptr(len(u)-1), uintptr(unsafe.Pointer(&rc)), DT_CENTER|DT_VCENTER|DT_SINGLELINE)
	if old != 0 {
		pSelectObject.Call(hdc, old)
	}
	runtime.KeepAlive(u)
}

func gdiShape(hdc uintptr, fill, stroke uintptr, width int, fn func()) {
	br, _, _ := pCreateSolidBrush.Call(fill)
	pen, _, _ := pCreatePen.Call(PS_SOLID, uintptr(width), stroke)
	oldB, _, _ := pSelectObject.Call(hdc, br)
	oldP, _, _ := pSelectObject.Call(hdc, pen)
	fn()
	if oldP != 0 {
		pSelectObject.Call(hdc, oldP)
	}
	if oldB != 0 {
		pSelectObject.Call(hdc, oldB)
	}
	if pen != 0 {
		pDeleteObject.Call(pen)
	}
	if br != 0 {
		pDeleteObject.Call(br)
	}
}

func drawRound(hdc uintptr, rc RECT, radius int32, fill, stroke uintptr, width int) {
	gdiShape(hdc, fill, stroke, width, func() {
		pRoundRect.Call(hdc, uintptr(rc.Left), uintptr(rc.Top), uintptr(rc.Right), uintptr(rc.Bottom), uintptr(radius), uintptr(radius))
	})
}
func drawCircle(hdc uintptr, cx, cy, r int32, fill, stroke uintptr, width int) {
	gdiShape(hdc, fill, stroke, width, func() { pEllipse.Call(hdc, uintptr(cx-r), uintptr(cy-r), uintptr(cx+r), uintptr(cy+r)) })
}
func drawRect(hdc uintptr, rc RECT, fill, stroke uintptr, width int) {
	gdiShape(hdc, fill, stroke, width, func() { pRectangle.Call(hdc, uintptr(rc.Left), uintptr(rc.Top), uintptr(rc.Right), uintptr(rc.Bottom)) })
}
func drawLine(hdc uintptr, x1, y1, x2, y2 int32, color uintptr, width int) {
	pen, _, _ := pCreatePen.Call(PS_SOLID, uintptr(width), color)
	old, _, _ := pSelectObject.Call(hdc, pen)
	pMoveToEx.Call(hdc, uintptr(x1), uintptr(y1), 0)
	pLineTo.Call(hdc, uintptr(x2), uintptr(y2))
	if old != 0 {
		pSelectObject.Call(hdc, old)
	}
	if pen != 0 {
		pDeleteObject.Call(pen)
	}
}

func drawDreamcastBody(hdc uintptr, w, h int32, fill, stroke uintptr) {
	br, _, _ := pCreateSolidBrush.Call(fill)
	pen, _, _ := pCreatePen.Call(PS_SOLID, 3, stroke)
	oldB, _, _ := pSelectObject.Call(hdc, br)
	oldP, _, _ := pSelectObject.Call(hdc, pen)
	pBeginPath.Call(hdc)
	pMoveToEx.Call(hdc, uintptr(w/2), 24, 0)
	pts := []POINT{
		{w * 35 / 100, 22}, {w * 27 / 100, 43}, {w * 20 / 100, 75},
		{w * 10 / 100, 82}, {w * 7 / 100, 125}, {w * 8 / 100, 180},
		{w * 9 / 100, 235}, {w * 10 / 100, h - 28}, {w * 14 / 100, h - 18},
		{w * 18 / 100, h - 10}, {w * 19 / 100, h - 78}, {w * 27 / 100, h - 96},
		{w * 35 / 100, h - 78}, {w * 42 / 100, h - 58}, {w / 2, h - 54},
		{w * 58 / 100, h - 58}, {w * 65 / 100, h - 78}, {w * 73 / 100, h - 96},
		{w * 81 / 100, h - 78}, {w * 82 / 100, h - 10}, {w * 86 / 100, h - 18},
		{w * 90 / 100, h - 28}, {w * 91 / 100, 235}, {w * 92 / 100, 180},
		{w * 93 / 100, 125}, {w * 90 / 100, 82}, {w * 80 / 100, 75},
		{w * 73 / 100, 43}, {w * 65 / 100, 22}, {w / 2, 24},
	}
	if len(pts) >= 3 {
		pPolyBezierTo.Call(hdc, uintptr(unsafe.Pointer(&pts[0])), uintptr(len(pts)))
	}
	pCloseFigure.Call(hdc)
	pEndPath.Call(hdc)
	pFillPath.Call(hdc)
	// Rebuild the path for stroke because FillPath consumes it.
	pBeginPath.Call(hdc)
	pMoveToEx.Call(hdc, uintptr(w/2), 24, 0)
	if len(pts) >= 3 {
		pPolyBezierTo.Call(hdc, uintptr(unsafe.Pointer(&pts[0])), uintptr(len(pts)))
	}
	pCloseFigure.Call(hdc)
	pEndPath.Call(hdc)
	pStrokePath.Call(hdc)
	if oldP != 0 {
		pSelectObject.Call(hdc, oldP)
	}
	if oldB != 0 {
		pSelectObject.Call(hdc, oldB)
	}
	if pen != 0 {
		pDeleteObject.Call(pen)
	}
	if br != 0 {
		pDeleteObject.Call(br)
	}
}

func drawDreamcastController(hdc uintptr, rc RECT, st DCVisualState, port int) {
	bg := rgb(247, 249, 252)
	body := rgb(232, 232, 229)
	edge := rgb(52, 56, 61)
	active := rgb(49, 180, 132)
	inactive := rgb(225, 228, 230)
	dark := rgb(31, 41, 55)
	muted := rgb(107, 114, 128)
	bgBrush, _, _ := pCreateSolidBrush.Call(bg)
	pFillRect.Call(hdc, uintptr(unsafe.Pointer(&rc)), bgBrush)
	if bgBrush != 0 {
		pDeleteObject.Call(bgBrush)
	}

	w := rc.Right - rc.Left
	h := rc.Bottom - rc.Top
	if w < 700 || h < 320 {
		drawTextCentered(hdc, "Controller preview needs more space", rc, dark)
		return
	}

	// Front silhouette modeled after the original Dreamcast controller: domed top,
	// wide shoulders and the two lower hand grips with the central cable notch.
	drawDreamcastBody(hdc, w, h, body, edge)

	// Dreamcast badge and VMU bay, centered like the real controller.
	drawTextCentered(hdc, "Dreamcast", RECT{w/2 - 70, 37, w/2 + 70, 61}, dark)
	vmu := RECT{w/2 - 82, 73, w/2 + 82, 166}
	drawRound(hdc, vmu, 18, rgb(214, 215, 211), edge, 2)
	screen := RECT{w/2 - 60, 91, w/2 + 60, 146}
	drawRound(hdc, screen, 8, rgb(189, 224, 205), rgb(88, 112, 101), 1)
	portName := string(rune('A' + port))
	drawTextCentered(hdc, "VMU  PORT "+portName, RECT{screen.Left + 3, screen.Top + 4, screen.Right - 3, screen.Top + 27}, dark)
	vmuState := "NO INPUT"
	if st.Connected {
		vmuState = "INPUT OK"
	}
	drawTextCentered(hdc, vmuState, RECT{screen.Left + 3, screen.Top + 26, screen.Right - 3, screen.Bottom - 3}, muted)

	// Trigger pressure sits near the top edge rather than floating above the pad.
	bar := func(x int32, val uint8, label string) {
		base := RECT{x, 52, x + 136, 67}
		drawRound(hdc, base, 7, rgb(210, 213, 215), edge, 1)
		fillW := int32(float64(132) * float64(val) / 255.0)
		if fillW > 0 {
			drawRound(hdc, RECT{x + 2, 54, x + 2 + fillW, 65}, 5, active, active, 1)
		}
		drawTextCentered(hdc, fmt.Sprintf("%s %3d", label, val), RECT{x, 67, x + 136, 87}, dark)
	}
	bar(88, st.LTrig, "L")
	bar(w-224, st.RTrig, "R")

	// Analog stick in the original upper-left location.
	sx, sy := int32(204), int32(130)
	drawCircle(hdc, sx, sy, 45, rgb(213, 213, 207), edge, 2)
	for r := int32(36); r >= 24; r -= 6 {
		drawCircle(hdc, sx, sy, r, rgb(213, 213, 207), rgb(133, 133, 126), 1)
	}
	kx := sx + int32(float64(st.JoyX)/127.0*25.0)
	ky := sy + int32(float64(st.JoyY)/127.0*25.0)
	knob := rgb(150, 150, 143)
	if st.JoyX != 0 || st.JoyY != 0 {
		knob = active
	}
	drawCircle(hdc, kx, ky, 17, knob, edge, 2)

	// D-pad lower-left.
	dx, dy := int32(250), int32(230)
	dOn := func(bit uint) uintptr {
		if st.Buttons&(1<<bit) != 0 {
			return active
		}
		return rgb(207, 208, 204)
	}
	drawRound(hdc, RECT{dx - 17, dy - 58, dx + 17, dy - 16}, 4, dOn(4), edge, 2)
	drawRound(hdc, RECT{dx - 17, dy + 16, dx + 17, dy + 58}, 4, dOn(5), edge, 2)
	drawRound(hdc, RECT{dx - 58, dy - 17, dx - 16, dy + 17}, 4, dOn(6), edge, 2)
	drawRound(hdc, RECT{dx + 16, dy - 17, dx + 58, dy + 17}, 4, dOn(7), edge, 2)
	drawCircle(hdc, dx, dy, 17, rgb(190, 190, 185), edge, 1)

	// Start in the lower middle, triangular-ish placement from the original pad.
	startFill := inactive
	if st.Buttons&(1<<3) != 0 {
		startFill = active
	}
	drawRound(hdc, RECT{w/2 - 44, 218, w/2 + 44, 247}, 10, startFill, edge, 2)
	drawTextCentered(hdc, "START", RECT{w/2 - 44, 218, w/2 + 44, 247}, dark)

	// Authentic Dreamcast face colors and positions: Y green, B blue, A red, X yellow.
	fx, fy := w-230, int32(177)
	face := func(label string, cx, cy int32, bit uint, baseColor uintptr) {
		fill := rgb(232, 233, 231)
		if st.Buttons&(1<<bit) != 0 {
			fill = baseColor
		}
		drawCircle(hdc, cx, cy, 25, fill, edge, 2)
		// Colored inner ring remains visible even when not pressed.
		drawCircle(hdc, cx, cy, 20, fill, baseColor, 2)
		drawTextCentered(hdc, label, RECT{cx - 22, cy - 21, cx + 22, cy + 21}, dark)
	}
	face("Y", fx, fy-59, 9, rgb(91, 191, 139))
	face("B", fx+61, fy, 1, rgb(55, 133, 214))
	face("A", fx, fy+59, 2, rgb(218, 72, 70))
	face("X", fx-61, fy, 10, rgb(241, 194, 37))

	// C/Z are Maple auxiliary bits, not physical standard-Dreamcast buttons.
	cz := []string{}
	if st.Buttons&(1<<0) != 0 {
		cz = append(cz, "C")
	}
	if st.Buttons&(1<<8) != 0 {
		cz = append(cz, "Z")
	}
	aux := ""
	if len(cz) != 0 {
		aux = "  AUX:" + strings.Join(cz, "/")
	}
	conn := "DISCONNECTED"
	connColor := rgb(190, 64, 64)
	if st.Connected {
		conn = "CONNECTED"
		connColor = active
	}
	drawTextCentered(hdc, fmt.Sprintf("PORT %s  •  %s  •  %s%s", portName, conn, st.Backend, aux), RECT{70, h - 28, w - 70, h - 5}, connColor)
}

func controllerVisualWndProc(hwnd uintptr, msg uint32, wp, lp uintptr) (ret uintptr) {
	defer func() {
		if v := recover(); v != nil {
			logRecovered("controllerVisualWndProc", v)
			ret, _, _ = pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
		}
	}()
	switch msg {
	case WM_PAINT:
		var ps PAINTSTRUCT
		hdc, _, _ := pBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		var rc RECT
		pGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
		memDC, _, _ := pCreateCompatibleDC.Call(hdc)
		bmp, _, _ := pCreateCompatibleBitmap.Call(hdc, uintptr(rc.Right-rc.Left), uintptr(rc.Bottom-rc.Top))
		if memDC != 0 && bmp != 0 {
			oldBmp, _, _ := pSelectObject.Call(memDC, bmp)
			st := ctlVisualState
			if !ctlVisualHaveState {
				st = dreamcastVisualState(currentCtlDeviceSpec(), ctlPort)
			}
			drawDreamcastController(memDC, rc, st, ctlPort)
			pBitBlt.Call(hdc, 0, 0, uintptr(rc.Right-rc.Left), uintptr(rc.Bottom-rc.Top), memDC, 0, 0, SRCCOPY)
			if oldBmp != 0 {
				pSelectObject.Call(memDC, oldBmp)
			}
			pDeleteObject.Call(bmp)
			pDeleteDC.Call(memDC)
		} else {
			drawDreamcastController(hdc, rc, dreamcastVisualState(currentCtlDeviceSpec(), ctlPort), ctlPort)
		}
		pEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0
	case WM_ERASEBKGND:
		return 1
	}
	r, _, _ := pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
	return r
}

func liveDeviceStatus(spec string) string {
	if spec == "none" {
		return "Disconnected — no controller will be exposed on this Dreamcast port."
	}
	if spec == "auto" {
		return "Automatic compatibility mode. Port A keeps the proven legacy mapping; other ports select the matching host index."
	}
	if spec == "keyboard_mouse" {
		keys := []string{}
		for _, k := range []struct {
			vk uintptr
			n  string
		}{{0x57, "W"}, {0x41, "A"}, {0x53, "S"}, {0x44, "D"}, {0x26, "Up"}, {0x28, "Down"}, {0x25, "Left"}, {0x27, "Right"}, {0x20, "Space"}, {0x0D, "Enter"}} {
			if keyDown(k.vk) {
				keys = append(keys, k.n)
			}
		}
		buttons := []string{}
		if keyDown(0x01) {
			buttons = append(buttons, "Mouse L")
		}
		if keyDown(0x02) {
			buttons = append(buttons, "Mouse R")
		}
		if keyDown(0x04) {
			buttons = append(buttons, "Mouse M")
		}
		var pt POINT
		pGetCursorPos.Call(uintptr(unsafe.Pointer(&pt)))
		if len(keys) == 0 {
			keys = []string{"—"}
		}
		if len(buttons) == 0 {
			buttons = []string{"—"}
		}
		return fmt.Sprintf("Keyboard + Mouse\r\nKeys: %s\r\nMouse: %s   Cursor: %d,%d", strings.Join(keys, ", "), strings.Join(buttons, ", "), pt.X, pt.Y)
	}
	if i, ok := parseSpecIndex(spec, "xinput:"); ok {
		st, connected := xinputState(i)
		if !connected {
			return fmt.Sprintf("XInput #%d: disconnected", i+1)
		}
		g := st.Gamepad
		return fmt.Sprintf("XInput #%d connected\r\nButtons: 0x%04X  LT:%3d RT:%3d\r\nLeft: %6d,%6d   Right: %6d,%6d", i+1, g.Buttons, g.LeftTrigger, g.RightTrigger, g.ThumbLX, g.ThumbLY, g.ThumbRX, g.ThumbRY)
	}
	if i, ok := parseSpecIndex(spec, "winmm:"); ok {
		j, connected := winmmState(i)
		if !connected {
			return fmt.Sprintf("DirectInput/WinMM #%d: disconnected", i+1)
		}
		return fmt.Sprintf("%s\r\nButtons: 0x%08X  POV:%d\r\nAxes: X=%d Y=%d Z=%d R=%d", winmmName(i), j.Buttons, j.POV, j.X, j.Y, j.Z, j.R)
	}
	return "Unknown input selection."
}

var ctlSlot1Specs []string
var ctlSlot2Specs []string

func findDeviceIndex(spec string) int {
	for i, d := range ctlDevices {
		if d.Spec == spec {
			return i
		}
	}
	return -1
}
func currentCtlDeviceSpec() string {
	i := comboIndexHandle(ctlControls[idCtlDevice])
	if i >= 0 && i < len(ctlDevices) {
		return ctlDevices[i].Spec
	}
	return "none"
}
func slotSpec(h uintptr, specs []string) string {
	i := comboIndexHandle(h)
	if i >= 0 && i < len(specs) {
		return specs[i]
	}
	return "empty"
}

func saveCtlSelections() {
	if ctlPort < 0 || ctlPort > 3 || hwndController == 0 {
		return
	}
	settings.MaplePorts[ctlPort].Device = currentCtlDeviceSpec()
	settings.MaplePorts[ctlPort].Slot1 = slotSpec(ctlControls[idCtlSlot1], ctlSlot1Specs)
	settings.MaplePorts[ctlPort].Slot2 = slotSpec(ctlControls[idCtlSlot2], ctlSlot2Specs)
	settings.MaplePorts[ctlPort] = normalizePortConfig(ctlPort, settings.MaplePorts[ctlPort])
}

func fillSlotCombo(h uintptr, port, slot int, selected string) []string {
	comboReset(h)
	specs := []string{"empty", "purupuru"}
	labels := []string{"Empty", "PuruPuru / Rumble Pack"}
	if port == 0 && slot == 1 {
		specs = []string{"empty", "vmu", "purupuru"}
		labels = []string{"Empty", "VMU A1 (persistent)", "PuruPuru / Rumble Pack"}
	}
	idx := 0
	for i, label := range labels {
		comboAdd(h, label)
		if specs[i] == selected {
			idx = i
		}
	}
	send(h, CB_SETCURSEL, uintptr(idx), 0)
	return specs
}

func refreshCtlDevices() {
	selected := settings.MaplePorts[ctlPort].Device
	ctlDevices = enumerateInputDevices()
	if findDeviceIndex(selected) < 0 && selected != "" {
		ctlDevices = append(ctlDevices, InputDevice{Spec: selected, Label: "Saved device (currently disconnected): " + selected})
	}
	h := ctlControls[idCtlDevice]
	comboReset(h)
	idx := 0
	for i, d := range ctlDevices {
		comboAdd(h, d.Label)
		if d.Spec == selected {
			idx = i
		}
	}
	send(h, CB_SETCURSEL, uintptr(idx), 0)
}

func loadCtlPort(port int) {
	if port < 0 || port > 3 {
		return
	}
	ctlPort = port
	settings.MaplePorts[port] = normalizePortConfig(port, settings.MaplePorts[port])
	refreshCtlDevices()
	ctlSlot1Specs = fillSlotCombo(ctlControls[idCtlSlot1], port, 1, settings.MaplePorts[port].Slot1)
	ctlSlot2Specs = fillSlotCombo(ctlControls[idCtlSlot2], port, 2, settings.MaplePorts[port].Slot2)
	updateCtlStatus()
}

func updateCtlStatus() {
	if hwndController == 0 {
		return
	}
	spec := currentCtlDeviceSpec()
	if spec != ctlLastHapticSpec {
		_, ctlHapticsStatus = maplePluginProbeHaptics(spec)
		ctlLastHapticSpec = spec
	}
	portName := string(rune('A' + ctlPort))
	c := settings.MaplePorts[ctlPort]
	status := fmt.Sprintf("Dreamcast Port %s\r\n%s\r\nSlot 1: %s   Slot 2: %s", portName, liveDeviceStatus(spec), c.Slot1, c.Slot2)
	if ctlHapticsStatus != "" {
		status += "\r\nHaptics: " + ctlHapticsStatus
	}
	if status != ctlLastStatusText {
		setText(ctlControls[idCtlStatus], status)
		ctlLastStatusText = status
	}
	canRumble := spec != "none" && spec != "keyboard_mouse"
	var en uintptr
	if canRumble {
		en = 1
	}
	pEnableWindow.Call(ctlControls[idCtlRumble], en)

	st := dreamcastVisualState(spec, ctlPort)
	if !ctlVisualHaveState || ctlVisualPort != ctlPort || st != ctlVisualState {
		ctlVisualState = st
		ctlVisualHaveState = true
		ctlVisualPort = ctlPort
		if hwndControllerVisual != 0 {
			pInvalidateRect.Call(hwndControllerVisual, 0, 0)
		}
	}
}

func controllerWndProc(hwnd uintptr, msg uint32, wp, lp uintptr) (ret uintptr) {
	defer func() {
		if v := recover(); v != nil {
			logRecovered("controllerWndProc", v)
			ret, _, _ = pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
		}
	}()
	switch msg {
	case WM_COMMAND:
		id := int(wp & 0xffff)
		notify := int((wp >> 16) & 0xffff)
		if notify == CBN_SELCHANGE {
			switch id {
			case idCtlPort:
				ctlHapticsStatus = ""
				saveCtlSelections()
				loadCtlPort(comboIndexHandle(ctlControls[idCtlPort]))
				return 0
			case idCtlDevice, idCtlSlot1, idCtlSlot2:
				if id == idCtlDevice {
					ctlHapticsStatus = ""
					ctlLastHapticSpec = ""
				}
				saveCtlSelections()
				updateCtlStatus()
				return 0
			}
		}
		switch id {
		case idCtlRefresh:
			saveCtlSelections()
			refreshCtlDevices()
			updateCtlStatus()
			return 0
		case idCtlRumble:
			spec := currentCtlDeviceSpec()
			ok, msg := maplePluginRumble(spec, 48000, 56000)
			ctlHapticsStatus = msg
			updateCtlStatus()
			if ok {
				time.AfterFunc(650*time.Millisecond, func() {
					maplePluginRumble(spec, 0, 0)
				})
			}
			return 0
		case idCtlSave:
			saveCtlSelections()
			saveSettings()
			pDestroyWindow.Call(hwnd)
			return 0
		}
	case WM_TIMER:
		updateCtlStatus()
		return 0
	case WM_CLOSE:
		saveCtlSelections()
		saveSettings()
		pDestroyWindow.Call(hwnd)
		return 0
	case WM_DESTROY:
		pKillTimer.Call(hwnd, 1)
		hwndController = 0
		hwndControllerVisual = 0
		ctlVisualHaveState = false
		ctlVisualPort = -1
		ctlLastStatusText = ""
		ctlHapticsStatus = ""
		ctlLastHapticSpec = ""
		ctlControls = map[int]uintptr{}
		return 0
	}
	r, _, _ := pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
	return r
}

func openController() {
	if hwndController != 0 {
		pShowWindow.Call(hwndController, SW_SHOW)
		return
	}
	inst, _, _ := pGetModuleHandleW.Call(0)
	cls := w("DCRModernMapleInputV07")
	if ctlWndProcPtr == 0 {
		ctlWndProcPtr = syscall.NewCallback(controllerWndProc)
		wc := WNDCLASSEX{CbSize: uint32(unsafe.Sizeof(WNDCLASSEX{})), WndProc: ctlWndProcPtr, Instance: inst, Background: 6, ClassName: cls}
		pRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))
	}
	visualCls := w("DCRDreamcastPadVisualV07")
	if ctlVisualWndProcPtr == 0 {
		ctlVisualWndProcPtr = syscall.NewCallback(controllerVisualWndProc)
		wc := WNDCLASSEX{CbSize: uint32(unsafe.Sizeof(WNDCLASSEX{})), WndProc: ctlVisualWndProcPtr, Instance: inst, Background: 0, ClassName: visualCls}
		pRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))
	}
	h, _, err := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(cls)), uintptr(unsafe.Pointer(w("Maple Controller Setup — Dreamcast Visual Test + Sony Haptics"))), WS_FIXED, 160, 40, 940, 820, hwndMain, 0, inst, 0)
	if h == 0 {
		msgbox(fmt.Sprintf("Could not open modern controller setup: %v", err), "Dreamcast Recompiled", MB_OK|MB_ICONERROR)
		return
	}
	hwndController = h
	createFor(h, ctlControls, "STATIC", "Dreamcast port", WS_CHILD|WS_VISIBLE, 24, 22, 130, 22, 0)
	cp := createFor(h, ctlControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 155, 18, 180, 160, idCtlPort)
	for _, x := range []string{"Port A", "Port B", "Port C", "Port D"} {
		comboAdd(cp, x)
	}
	send(cp, CB_SETCURSEL, 0, 0)
	createFor(h, ctlControls, "STATIC", "Host device", WS_CHILD|WS_VISIBLE, 365, 22, 105, 22, 0)
	createFor(h, ctlControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 470, 18, 405, 260, idCtlDevice)
	createFor(h, ctlControls, "STATIC", "Expansion slot 1", WS_CHILD|WS_VISIBLE, 24, 67, 130, 22, 0)
	createFor(h, ctlControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 155, 63, 260, 160, idCtlSlot1)
	createFor(h, ctlControls, "STATIC", "Expansion slot 2", WS_CHILD|WS_VISIBLE, 455, 67, 130, 22, 0)
	createFor(h, ctlControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 585, 63, 290, 160, idCtlSlot2)

	createFor(h, ctlControls, "STATIC", "Dreamcast mapped input — live visual test", WS_CHILD|WS_VISIBLE, 24, 111, 360, 22, 0)
	hwndControllerVisual = createFor(h, ctlControls, "DCRDreamcastPadVisualV07", "", WS_CHILD|WS_VISIBLE, 24, 136, 871, 370, idCtlVisual)

	createFor(h, ctlControls, "STATIC", "Raw host diagnostics", WS_CHILD|WS_VISIBLE, 24, 520, 180, 22, 0)
	createFor(h, ctlControls, "STATIC", "", WS_CHILD|WS_VISIBLE, 24, 544, 871, 96, idCtlStatus)
	createFor(h, ctlControls, "BUTTON", "Refresh devices", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 24, 656, 150, 34, idCtlRefresh)
	createFor(h, ctlControls, "BUTTON", "Test rumble (PS4 / PS5)", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 190, 656, 170, 34, idCtlRumble)
	createFor(h, ctlControls, "BUTTON", "Save & Close", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 730, 656, 165, 34, idCtlSave)
	createFor(h, ctlControls, "STATIC", "Dreamcast-shaped live view: A/B/X/Y, D-pad, Start, analog stick and L/R show the mapped Maple state. Test rumble uses dcr_maple.dll itself and supports XInput, DualShock 4 and DualSense/PS5 HID.", WS_CHILD|WS_VISIBLE, 24, 708, 871, 52, 0)
	loadCtlPort(0)
	pSetTimer.Call(h, 1, 33, 0)
	pShowWindow.Call(h, SW_SHOW)
	pUpdateWindow.Call(h)
}

const (
	aHeartbeats = 301 + iota
	aConsole
	aNativeSeeds
	aNativePatches
	aInterp
	aInterpTrace
	aRelocTrace
	aSPG
	aHotRecovery
	aPortable
	aRuntime
	aNetwork
	aMaple
	aGDROM
	aHotPC
	aFPU
	aDone
	aResolution
	aAspect
	aVideoBackend
)

func advChecked(id int) bool { return send(advControls[id], BM_GETCHECK, 0, 0) == BST_CHECKED }
func advSet(id int, v bool) {
	if v {
		send(advControls[id], BM_SETCHECK, BST_CHECKED, 0)
	} else {
		send(advControls[id], BM_SETCHECK, 0, 0)
	}
}
func closeAdvanced() {
	if hwndAdvanced == 0 {
		return
	}
	settings.Heartbeats = advChecked(aHeartbeats)
	settings.ShowConsole = advChecked(aConsole)
	settings.NativeSeeds = advChecked(aNativeSeeds)
	settings.NativePatches = advChecked(aNativePatches)
	settings.Interpreter = advChecked(aInterp)
	settings.InterpTrace = advChecked(aInterpTrace)
	settings.CodeRelocTrace = advChecked(aRelocTrace)
	settings.SPGFastForward = advChecked(aSPG)
	settings.HotRecovery = advChecked(aHotRecovery)
	settings.AutoPortableSync = advChecked(aPortable)
	settings.RebuildRuntime = advChecked(aRuntime)
	settings.RebuildNetwork = advChecked(aNetwork)
	settings.RebuildMaple = advChecked(aMaple)
	settings.RebuildGDROM = advChecked(aGDROM)
	settings.HotPC = advChecked(aHotPC)
	settings.FPU = comboIndexHandle(advControls[aFPU])
	settings.VideoResolution = comboIndexHandle(advControls[aResolution])
	settings.VideoAspect = comboIndexHandle(advControls[aAspect])
	settings.VideoBackend = comboIndexHandle(advControls[aVideoBackend])
	saveSettings()
	pDestroyWindow.Call(hwndAdvanced)
	hwndAdvanced = 0
}
func advancedWndProc(hwnd uintptr, msg uint32, wp, lp uintptr) (ret uintptr) {
	defer func() {
		if v := recover(); v != nil {
			logRecovered("advancedWndProc", v)
			ret, _, _ = pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
		}
	}()
	switch msg {
	case WM_COMMAND:
		if int(wp&0xffff) == aDone {
			closeAdvanced()
			return 0
		}
	case WM_CLOSE:
		closeAdvanced()
		return 0
	case WM_DESTROY:
		hwndAdvanced = 0
		return 0
	}
	r, _, _ := pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
	return r
}
func openAdvanced() {
	if hwndAdvanced != 0 {
		return
	}
	// Persist anything selected on the main form, but do not touch old cache.
	saveSettings()
	inst, _, _ := pGetModuleHandleW.Call(0)
	cls := w("DCRAdvancedSettingsV15")
	advWndProcPtr = syscall.NewCallback(advancedWndProc)
	wc := WNDCLASSEX{CbSize: uint32(unsafe.Sizeof(WNDCLASSEX{})), WndProc: advWndProcPtr, Instance: inst, Background: 6, ClassName: cls}
	pRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))
	h, _, _ := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(cls)), uintptr(unsafe.Pointer(w("Dreamcast Recompiled - Opciones avanzadas"))), WS_FIXED, 260, 60, 865, 710, hwndMain, 0, inst, 0)
	if h == 0 {
		msgbox("No pude abrir las opciones avanzadas.", "Launcher", MB_OK|MB_ICONERROR)
		return
	}
	hwndAdvanced = h
	advControls = map[int]uintptr{}
	label := func(x, y int32, s string) {
		createFor(h, advControls, "STATIC", s, WS_CHILD|WS_VISIBLE, x, y, 790, 22, 0)
	}
	check := func(id int, x, y int32, name string, value bool) {
		c := createFor(h, advControls, "BUTTON", name, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX, x, y, 400, 28, id)
		_ = c
		advSet(id, value)
	}
	label(18, 12, "SH-4 / Caché nativa y rendimiento")
	check(aNativeSeeds, 20, 40, "Compilar automáticamente nuevas seeds SH-4", settings.NativeSeeds)
	check(aNativePatches, 435, 40, "Cargar DLLs nativas incrementales", settings.NativePatches)
	check(aInterp, 20, 75, "Intérprete temporal cuando falte target", settings.Interpreter)
	check(aInterpTrace, 435, 75, "Registrar seeds interpretadas", settings.InterpTrace)
	check(aSPG, 20, 110, "SPG Fast Forward", settings.SPGFastForward)
	check(aHotRecovery, 435, 110, "Hot Block Recovery", settings.HotRecovery)
	check(aPortable, 20, 145, "Sincronizar build portable automáticamente", settings.AutoPortableSync)
	check(aHotPC, 435, 145, "HOTPC profiler (mayor overhead)", settings.HotPC)
	label(20, 188, "Opciones de depuración (desactivadas de forma predeterminada)")
	check(aHeartbeats, 20, 218, "Heartbeats del runtime (1 segundo)", settings.Heartbeats)
	check(aConsole, 435, 218, "Mostrar consola externa del juego", settings.ShowConsole)
	check(aRelocTrace, 20, 253, "Trace de reubicaciones SH-4", settings.CodeRelocTrace)
	label(20, 300, "Recompilar plugins (conserva el guest SH-4 existente)")
	check(aRuntime, 20, 330, "Runtime y plugins", settings.RebuildRuntime)
	check(aNetwork, 435, 330, "Solo dcr_network.dll", settings.RebuildNetwork)
	check(aMaple, 20, 365, "Solo dcr_maple.dll", settings.RebuildMaple)
	check(aGDROM, 435, 365, "Solo dcr_gdrom.dll", settings.RebuildGDROM)
	label(20, 414, "FPU")
	combo := createFor(h, advControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 92, 411, 240, 120, aFPU)
	comboAdd(combo, "Superblock (estable)")
	comboAdd(combo, "RegionPlus (A/B)")
	send(combo, CB_SETCURSEL, uintptr(settings.FPU), 0)
	label(20, 455, "Vídeo (solo ajustes implementados en el runtime)")
	label(20, 483, "Resolución")
	resolutions := createFor(h, advControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 118, 480, 188, 160, aResolution)
	for _, v := range []string{"Nativa", "960p", "1080p", "1440p", "Escritorio"} {
		comboAdd(resolutions, v)
	}
	send(resolutions, CB_SETCURSEL, uintptr(settings.VideoResolution), 0)
	label(330, 483, "Aspecto")
	aspects := createFor(h, advControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 410, 480, 155, 120, aAspect)
	comboAdd(aspects, "4:3 nativo")
	comboAdd(aspects, "16:9 estirado")
	send(aspects, CB_SETCURSEL, uintptr(settings.VideoAspect), 0)
	label(580, 483, "Salida")
	backend := createFor(h, advControls, "COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 642, 480, 174, 120, aVideoBackend)
	comboAdd(backend, "DXGI (nativo)")
	comboAdd(backend, "GDI (compat)")
	send(backend, CB_SETCURSEL, uintptr(settings.VideoBackend), 0)
	label(20, 545, "La recompilación incremental no modifica la DLL base si la caché ABI1 sigue válida.")
	createFor(h, advControls, "BUTTON", "Guardar opciones", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 325, 578, 220, 44, aDone)
	pShowWindow.Call(h, SW_SHOW)
	pUpdateWindow.Call(h)
}

func browseDisc() {
	buf := make([]uint16, 32768)
	if cur := getText(controls[idDiscEdit]); cur != "" {
		u, _ := syscall.UTF16FromString(cur)
		copy(buf, u)
	}
	filter := "Dreamcast images (*.cdi;*.gdi;*.iso)\x00*.cdi;*.gdi;*.iso\x00CDI (*.cdi)\x00*.cdi\x00GDI (*.gdi)\x00*.gdi\x00ISO (*.iso)\x00*.iso\x00All files\x00*.*\x00\x00"
	fu := make([]uint16, 0, len(filter))
	for _, r := range filter {
		fu = append(fu, uint16(r))
	}
	ofn := OPENFILENAME{
		LStructSize: uint32(unsafe.Sizeof(OPENFILENAME{})), HwndOwner: hwndMain,
		LpstrFilter: &fu[0], NFilterIndex: 1, LpstrFile: &buf[0], NMaxFile: uint32(len(buf)),
		Flags: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, LpstrTitle: w("Seleccionar imagen original Dreamcast CDI/GDI/ISO"),
	}
	r, _, _ := pGetOpenFileNameW.Call(uintptr(unsafe.Pointer(&ofn)))
	runtime.KeepAlive(fu)
	runtime.KeepAlive(buf)
	if r != 0 {
		setText(controls[idDiscEdit], syscall.UTF16ToString(buf))
		saveSettings()
	}
}

func runHidden(name string, args ...string) error {
	c := exec.Command(name, args...)
	c.Dir = exeDir()
	c.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	out, err := c.CombinedOutput()
	if err != nil {
		return fmt.Errorf("%v\n%s", err, string(out))
	}
	return nil
}

func makeSnapshot() LaunchSnapshot {
	s := readSettingsFromUI()
	return LaunchSnapshot{Disc: s.Disc, Renderer: s.Renderer, AudioMode: s.AudioMode, Perf: s.Perf, Debug: s.Debug, PVRProfile: s.PVRProfile, MapleTrace: s.MapleTrace, GDROMTrace: s.GDROMTrace, Clock: s.Clock, NetworkMode: s.NetworkMode, BuildMode: s.BuildMode, VideoResolution: s.VideoResolution, VideoAspect: s.VideoAspect, VideoBackend: s.VideoBackend,
		Options: LaunchOptions{Renderer: s.Renderer, AudioMode: s.AudioMode, BuildMode: s.BuildMode, FPU: s.FPU,
			Perf: s.Perf, Debug: s.Debug, PVRProfile: s.PVRProfile, MapleTrace: s.MapleTrace, GDROMTrace: s.GDROMTrace,
			Heartbeats: s.Heartbeats, ShowConsole: s.ShowConsole, NativeSeeds: s.NativeSeeds, NativePatches: s.NativePatches, Interpreter: s.Interpreter,
			InterpTrace: s.InterpTrace, CodeRelocTrace: s.CodeRelocTrace, SPGFastForward: s.SPGFastForward, HotRecovery: s.HotRecovery,
			AutoPortableSync: s.AutoPortableSync, RebuildRuntime: s.RebuildRuntime, RebuildNetwork: s.RebuildNetwork, RebuildMaple: s.RebuildMaple, RebuildGDROM: s.RebuildGDROM, HotPC: s.HotPC}, MaplePorts: s.MaplePorts}
}

func postStatus(s string) {
	asyncMu.Lock()
	asyncStatus = s
	asyncMu.Unlock()
	pPostMessageW.Call(hwndMain, WM_APP_STATUS, 0, 0)
}
func postResult(ok bool, text string) {
	asyncMu.Lock()
	asyncOK = ok
	if ok {
		asyncStatus = text
		asyncErr = ""
	} else {
		asyncErr = text
		asyncStatus = "Launch failed"
	}
	asyncMu.Unlock()
	pPostMessageW.Call(hwndMain, WM_APP_RESULT, 0, 0)
}

func discMapTitle(path string) string {
	b, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	for _, line := range strings.Split(string(b), "\n") {
		line = strings.TrimSpace(line)
		if len(line) >= 6 && strings.EqualFold(line[:6], "title=") {
			return strings.TrimSpace(line[6:])
		}
	}
	return ""
}

// The original portable GUI hardcoded CHUCHU_ROCKET and only booted a precompiled
// runner. The real launcher must use run_game.bat, which owns cache validation,
// per-game profiles, ABI1 incremental patch builds and runtime/plugin rebuilds.
func projectRoot() string {
	root := exeDir()
	for _, dir := range []string{root, filepath.Dir(root)} {
		p := filepath.Join(dir, "run_game.bat")
		if st, err := os.Stat(p); err == nil && !st.IsDir() {
			return dir
		}
	}
	return ""
}
func networkModeEnv(n int) string {
	switch n {
	case 1:
		return "dcnet"
	case 2:
		return "off"
	default:
		return "auto"
	}
}
func videoEnv(s LaunchSnapshot) []string {
	resolution := []string{"native", "960p", "1080p", "1440p", "desktop"}[s.VideoResolution]
	aspect := []string{"4:3", "16:9"}[s.VideoAspect]
	backend := []string{"dx11", "gdi"}[s.VideoBackend]
	return []string{"DCR_VIDEO_RESOLUTION=" + resolution, "DCR_VIDEO_ASPECT=" + aspect, "DCR_VIDEO_OUTPUT_BACKEND=" + backend}
}

func launchFromProject(s LaunchSnapshot, root string) {
	disc := strings.TrimSpace(s.Disc)
	if disc == "" {
		postResult(false, "Selecciona una imagen CDI/GDI/ISO.")
		return
	}
	ext := strings.ToLower(filepath.Ext(disc))
	if ext != ".cdi" && ext != ".gdi" && ext != ".iso" {
		postResult(false, "Formato de juego no admitido: "+ext)
		return
	}
	if st, err := os.Stat(disc); err != nil || st.IsDir() {
		postResult(false, "La imagen del juego no existe: "+disc)
		return
	}
	// cmd.exe expands percent/delayed-expansion characters even inside quotes.
	// Refuse those rare paths rather than executing a corrupted command line.
	if strings.ContainsAny(disc+root, "%!?\r\n\"") {
		postResult(false, "La ruta contiene caracteres que cmd.exe no puede procesar con seguridad (% ! ? o comillas). Cambia el nombre o la carpeta.")
		return
	}
	bat := filepath.Join(root, "run_game.bat")
	args := buildRunGameArgs(s.Options)
	// Standard cmd /S /C double-quote convention to preserve paths with spaces.
	line := `""` + bat + `" "` + disc + `" ` + strings.Join(args, " ") + `"`
	if s.Options.ShowConsole {
		// ShellExecute starts a real interactive console; os/exec in a GUI
		// subsystem application otherwise gives cmd.exe NUL standard handles.
		// Use a per-launch wrapper to carry the network and Maple environment.
		wrapperDir := filepath.Join(appDataDir(), "logs")
		_ = os.MkdirAll(wrapperDir, 0755)
		wrapper := filepath.Join(wrapperDir, "debug-console-"+time.Now().Format("20060102-150405")+".cmd")
		lines := []string{"@echo off", "setlocal DisableDelayedExpansion", "set \"DCR_NETWORK_MODE=" + networkModeEnv(s.NetworkMode) + "\""}
		for _, v := range videoEnv(s) {
			lines = append(lines, "set \""+v+"\"")
		}
		for i, port := range []string{"A", "B", "C", "D"} {
			c := normalizePortConfig(i, s.MaplePorts[i])
			lines = append(lines, "set \"DCR_MAPLE_PORT_"+port+"_DEVICE="+c.Device+"\"",
				"set \"DCR_MAPLE_PORT_"+port+"_SLOT1="+c.Slot1+"\"",
				"set \"DCR_MAPLE_PORT_"+port+"_SLOT2="+c.Slot2+"\"")
		}
		lines = append(lines, "cd /d \""+root+"\"", "call \""+bat+"\" \""+disc+"\" "+strings.Join(args, " "), "echo.", "echo [DCR] La sesion finalizo. Puedes revisar los mensajes anteriores.")
		if err := os.WriteFile(wrapper, []byte(strings.Join(lines, "\r\n")+"\r\n"), 0600); err != nil {
			postResult(false, "No se pudo crear wrapper de consola: "+err.Error())
			return
		}
		parameters := `/D /K "` + wrapper + `"`
		r, _, _ := pShellExecuteW.Call(hwndMain, uintptr(unsafe.Pointer(w("open"))), uintptr(unsafe.Pointer(w("cmd.exe"))), uintptr(unsafe.Pointer(w(parameters))), uintptr(unsafe.Pointer(w(root))), SW_SHOW)
		if r <= 32 {
			postResult(false, "Windows no pudo abrir la consola (ShellExecute). Revisa el registro del launcher.")
			return
		}
		appendLog("debug-console game=%q mode=%s wrapper=%q", disc, networkModeEnv(s.NetworkMode), wrapper)
		postResult(true, "Consola abierta: "+filepath.Base(disc))
		return
	}
	cmd := exec.Command("cmd.exe", "/D", "/S", "/C", line)
	cmd.Dir = root
	env := append([]string{}, os.Environ()...)
	env = append(env, "DCR_NETWORK_MODE="+networkModeEnv(s.NetworkMode))
	env = append(env, videoEnv(s)...)
	for i, port := range []string{"A", "B", "C", "D"} {
		c := normalizePortConfig(i, s.MaplePorts[i])
		env = append(env, "DCR_MAPLE_PORT_"+port+"_DEVICE="+c.Device,
			"DCR_MAPLE_PORT_"+port+"_SLOT1="+c.Slot1, "DCR_MAPLE_PORT_"+port+"_SLOT2="+c.Slot2)
	}
	cmd.Env = env
	logs := filepath.Join(appDataDir(), "logs")
	_ = os.MkdirAll(logs, 0755)
	logName := filepath.Join(logs, "launch-"+time.Now().Format("20060102-150405")+".log")
	f, err := os.OpenFile(logName, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, 0644)
	if err != nil {
		postResult(false, "No se puede escribir log: "+err.Error())
		return
	}
	// A separate console is optional; hidden by default for ordinary players.
	// Stdout/stderr always go to a log so failures are diagnosable.
	cmd.Stdout = f
	cmd.Stderr = f
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: !s.Options.ShowConsole}
	if s.Options.ShowConsole {
		cmd.SysProcAttr.CreationFlags = 0x00000010
	}
	postStatus("Preparando/cargando caché nativa incremental...")
	appendLog("native launch game=%q mode=%d network=%s argv=%v", disc, s.BuildMode, networkModeEnv(s.NetworkMode), args)
	if err = cmd.Start(); err != nil {
		_ = f.Close()
		postResult(false, "No pude iniciar run_game.bat: "+err.Error())
		return
	}
	_ = f.Close()
	// Reap the child without blocking the launcher UI; preserve command exit status.
	go func() {
		err := cmd.Wait()
		if err != nil {
			appendLog("game script failed game=%q: %v; log=%q", disc, err, logName)
			postStatus("El proceso terminó con un error. Revisa los logs.")
		} else {
			postStatus("Proceso finalizado. Registro: " + logName)
		}
	}()
	postResult(true, "Iniciado: "+filepath.Base(disc)+" | Red: "+networkModeEnv(s.NetworkMode))
}

func prepareAndLaunchTask(s LaunchSnapshot) {
	defer func() {
		if v := recover(); v != nil {
			logRecovered("prepareAndLaunchTask", v)
			postResult(false, fmt.Sprintf("Error del launcher: %v", v))
		}
	}()
	if root := projectRoot(); root != "" {
		launchFromProject(s, root)
		return
	}
	postResult(false, "No se encontró run_game.bat junto a DreamcastRecompiled.exe o en su carpeta superior. Extrae la versión completa para activar el guest ABI1 y la compilación incremental.")
}

func startPlay() {
	asyncMu.Lock()
	if playBusy {
		asyncMu.Unlock()
		return
	}
	playBusy = true
	asyncMu.Unlock()
	saveSettings()
	enable(idPlay, false)
	enable(idBrowse, false)
	setText(controls[idStatus], "Preparing...")
	snap := makeSnapshot()
	// Explicit destructive or rebuild choices are one-shot. Next launch always
	// returns to automatic incremental compilation unless selected again.
	if snap.BuildMode == 2 {
		setCombo(idBuildMode, 0)
	}
	settings.RebuildRuntime = false
	settings.RebuildNetwork = false
	settings.RebuildMaple = false
	settings.RebuildGDROM = false
	saveSettings()
	go prepareAndLaunchTask(snap)
}

func openGameCache() {
	p := filepath.Join(appDataDir(), "games")
	_ = os.MkdirAll(p, 0755)
	pShellExecuteW.Call(hwndMain, uintptr(unsafe.Pointer(w("open"))), uintptr(unsafe.Pointer(w(p))), 0, 0, SW_SHOW)
}

func openLogs() {
	p := filepath.Join(appDataDir(), "logs")
	_ = os.MkdirAll(p, 0755)
	pShellExecuteW.Call(hwndMain, uintptr(unsafe.Pointer(w("open"))), uintptr(unsafe.Pointer(w(p))), 0, 0, SW_SHOW)
}

func wndProc(hwnd uintptr, msg uint32, wp, lp uintptr) (ret uintptr) {
	defer func() {
		if v := recover(); v != nil {
			logRecovered("wndProc", v)
			ret, _, _ = pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
		}
	}()
	switch msg {
	case WM_PAINT:
		if aquaHome {
			return aquaPaint(hwnd)
		}
	case WM_ERASEBKGND:
		if aquaHome {
			return 1
		}
	case wmMouseMove:
		if aquaHome {
			aquaHover(lp)
			return 0
		}
	case wmLButtonUp:
		if aquaHome {
			aquaClick(lp)
			return 0
		}
	case wmKeyDown:
		if aquaHome {
			aquaKeyboard(wp)
			return 0
		}
	case WM_TIMER:
		if aquaHome {
			pInvalidateRect.Call(hwnd, 0, 0)
			return 0
		}
	case WM_COMMAND:
		id := int(wp & 0xffff)
		switch id {
		case idBrowse:
			browseDisc()
		case idHome:
			showAquaHome()
		case idPlay:
			startPlay()
		case idPad:
			openController()
		case idLogs:
			openLogs()
		case idAdvanced:
			openAdvanced()
		case idOpenGameCache:
			openGameCache()
		}
		return 0
	case WM_APP_STATUS:
		asyncMu.Lock()
		s := asyncStatus
		asyncMu.Unlock()
		setText(controls[idStatus], s)
		return 0
	case WM_APP_RESULT:
		asyncMu.Lock()
		ok := asyncOK
		s := asyncStatus
		e := asyncErr
		playBusy = false
		asyncMu.Unlock()
		enable(idPlay, true)
		enable(idBrowse, true)
		setText(controls[idStatus], s)
		if !ok && e != "" {
			msgbox(e+"\n\nLauncher diagnostics: "+launcherLogPath(), "Launch failed", MB_OK|MB_ICONERROR)
		}
		return 0
	case WM_CLOSE:
		aquaStop()
		if hwndAdvanced != 0 {
			closeAdvanced()
		}
		saveSettings()
		pDestroyWindow.Call(hwnd)
		return 0
	case WM_DESTROY:
		hwndMain = 0
		pPostQuitMessage.Call(0)
		return 0
	}
	r, _, _ := pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
	return r
}

func runMain() error {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()
	loadSettings()
	// Drag & drop a CDI/GDI/ISO onto DreamcastRecompiled.exe, or pass it as argv[1].
	if len(os.Args) > 1 && gameValid(os.Args[1]) {
		disc, err := filepath.Abs(os.Args[1])
		if err == nil {
			settings.Disc = disc
			addLibraryGame(disc)
		}
	}
	appendLog("startup version=%s exe=%q", launcherVersion, exeDir())

	inst, _, errGet := pGetModuleHandleW.Call(0)
	if inst == 0 {
		return fmt.Errorf("GetModuleHandleW failed: %v", errGet)
	}
	cls := w("DCRNativeAquaLauncherV16")
	wndProcPtr = syscall.NewCallback(wndProc)    // retain for the full process lifetime
	iconHandle, _, _ := pLoadIconW.Call(inst, 1) // icon group resource ID=1
	wc := WNDCLASSEX{CbSize: uint32(unsafe.Sizeof(WNDCLASSEX{})), WndProc: wndProcPtr, Instance: inst, Icon: iconHandle, IconSm: iconHandle, Background: 6, ClassName: cls}
	atom, _, errReg := pRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))
	if atom == 0 {
		return fmt.Errorf("RegisterClassExW failed: %v", errReg)
	}

	h, _, errCreate := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(cls)), uintptr(unsafe.Pointer(w("Dreamcast Recompiled v1.2"))), WS_FIXED, 200, 100, 820, 725, 0, 0, inst, 0)
	if h == 0 {
		return fmt.Errorf("CreateWindowExW(main) failed: %v", errCreate)
	}
	hwndMain = h

	create("STATIC", "IMAGEN DEL JUEGO (CDI / GDI / ISO)", WS_CHILD|WS_VISIBLE, 24, 20, 350, 22, 0)
	create("EDIT", settings.Disc, WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL, 24, 47, 640, 28, idDiscEdit)
	create("BUTTON", "Examinar...", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 674, 47, 112, 30, idBrowse)

	create("STATIC", "VIDEO", WS_CHILD|WS_VISIBLE, 24, 95, 130, 22, 0)
	cr := create("COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 145, 93, 220, 160, idRender)
	for _, v := range []string{"Automático / DX11", "Direct3D 11 (GPU)", "Software (CPU)"} {
		comboAdd(cr, v)
	}
	setCombo(idRender, settings.Renderer)
	create("STATIC", "AUDIO", WS_CHILD|WS_VISIBLE, 403, 95, 90, 22, 0)
	ca := create("COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 495, 93, 291, 160, idAudio)
	for _, v := range []string{"Completo (ARM7 y salida)", "Básico", "Sin salida de audio"} {
		comboAdd(ca, v)
	}
	setCombo(idAudio, settings.AudioMode)

	create("STATIC", "RED / ONLINE", WS_CHILD|WS_VISIBLE, 24, 146, 125, 22, 0)
	cn := create("COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 145, 142, 265, 160, idNetwork)
	for _, v := range []string{"Automático (módem/PPP)", "DCNet / Flycast compatible", "Sin conexión"} {
		comboAdd(cn, v)
	}
	setCombo(idNetwork, settings.NetworkMode)
	create("STATIC", "CLOCK", WS_CHILD|WS_VISIBLE, 432, 146, 65, 22, 0)
	cc := create("COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 495, 142, 291, 160, idClock)
	for _, v := range []string{"Sincronizado con host", "Reloj Dreamcast"} {
		comboAdd(cc, v)
	}
	setCombo(idClock, settings.Clock)

	create("STATIC", "COMPILACIÓN SH-4", WS_CHILD|WS_VISIBLE, 24, 197, 120, 22, 0)
	cb := create("COMBOBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST, 145, 193, 641, 170, idBuildMode)
	for _, v := range []string{"Incremental automática (recomendado)", "Reutilizar caché existente (sin recompilar)", "Recompilar juego completo (solo si se solicita)"} {
		comboAdd(cb, v)
	}
	setCombo(idBuildMode, settings.BuildMode)

	create("BUTTON", "Perfil de rendimiento", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX, 24, 248, 230, 28, idPerf)
	setChecked(idPerf, settings.Perf)
	create("BUTTON", "Depuración", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX, 278, 248, 185, 28, idDebug)
	setChecked(idDebug, settings.Debug)
	create("BUTTON", "Perfil PVR", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX, 495, 248, 230, 28, idProfile)
	setChecked(idProfile, settings.PVRProfile)
	create("BUTTON", "Trace de controles Maple", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX, 24, 284, 230, 28, idMapleTrace)
	setChecked(idMapleTrace, settings.MapleTrace)
	create("BUTTON", "Trace de GD-ROM", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX, 278, 284, 230, 28, idGDROMTrace)
	setChecked(idGDROMTrace, settings.GDROMTrace)

	create("BUTTON", "Configurar mandos / Rumble", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 24, 337, 235, 38, idPad)
	create("BUTTON", "Opciones avanzadas...", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 271, 337, 227, 38, idAdvanced)
	create("BUTTON", "Logs", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 510, 337, 118, 38, idLogs)
	create("BUTTON", "Caché", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 640, 337, 146, 38, idOpenGameCache)

	create("STATIC", "DCNet utiliza el puente de módem V.42/LAPM + PPP del plugin dcr_network.dll. La conexión requiere un servidor disponible.", WS_CHILD|WS_VISIBLE, 24, 408, 762, 45, idNetStatus)
	create("STATIC", "SH-4: guest ABI1 conservado, seeds nuevas → DLLs incrementales. Sin recompilación completa por defecto.", WS_CHILD|WS_VISIBLE, 24, 471, 762, 39, 0)
	create("STATIC", "Listo para iniciar", WS_CHILD|WS_VISIBLE, 24, 534, 762, 25, idStatus)
	create("BUTTON", "INICIAR JUEGO", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 258, 573, 305, 60, idPlay)
	create("STATIC", "Dreamcast Recompiled v1.2  |  Launcher nativo 1.5  |  SH-4 Native Incremental", WS_CHILD|WS_VISIBLE, 24, 651, 762, 25, 0)

	create("BUTTON", "← Volver al menú Aqua", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 575, 585, 211, 40, idHome)
	// The original Win32 controls remain available as the functional settings
	// page. The dashboard itself is a native image-backed Win32 surface.
	aquaInit()
	showAquaHome()
	pShowWindow.Call(hwndMain, SW_SHOW)
	pUpdateWindow.Call(hwndMain)
	if len(os.Args) > 1 && os.Args[1] == "--controllers" {
		openController()
	}
	var m MSG
	for {
		r, _, e := pGetMessageW.Call(uintptr(unsafe.Pointer(&m)), 0, 0, 0)
		if int32(r) == -1 {
			return fmt.Errorf("GetMessageW failed: %v", e)
		}
		if r == 0 {
			break
		}
		pTranslateMessage.Call(uintptr(unsafe.Pointer(&m)))
		pDispatchMessageW.Call(uintptr(unsafe.Pointer(&m)))
	}
	appendLog("normal exit")
	return nil
}

func main() {
	defer func() {
		if v := recover(); v != nil {
			logRecovered("main", v)
			msgbox(fmt.Sprintf("Launcher crashed unexpectedly: %v\n\nDiagnostics:\n%s", v, launcherLogPath()), "Dreamcast Recompiled", MB_OK|MB_ICONERROR)
		}
	}()
	if err := runMain(); err != nil {
		appendLog("fatal: %v", err)
		msgbox(err.Error()+"\n\nDiagnostics:\n"+launcherLogPath(), "Dreamcast Recompiled", MB_OK|MB_ICONERROR)
	}
}
