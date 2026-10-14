//go:build windows

package main

// Aqua is rendered by stock Win32 + GDI+ (gdiplus.dll). The launcher never
// executes Python or embeds a WebView. Existing PNGs stay external to the EXE
// so themes can be replaced without rebuilding the binary.
import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"syscall"
	"time"
	"unsafe"
)

const (
	wmMouseMove = 0x0200
	wmLButtonUp = 0x0202
	wmKeyDown   = 0x0100
	swHide      = 0
	swpNoZOrder = 0x0004
	swpNoMove   = 0x0002
)

var (
	aquaHome          = false
	aquaNight         = false
	aquaHovered       = -1
	aquaFocused       = 0
	aquaReady         = false
	aquaToken         uintptr
	aquaImages        = map[string]uintptr{}
	aquaGdi           = syscall.NewLazyDLL("gdiplus.dll")
	gdStartup         = aquaGdi.NewProc("GdiplusStartup")
	gdShutdown        = aquaGdi.NewProc("GdiplusShutdown")
	gdLoadImage       = aquaGdi.NewProc("GdipLoadImageFromFile")
	gdDisposeImage    = aquaGdi.NewProc("GdipDisposeImage")
	gdCreateGraphics  = aquaGdi.NewProc("GdipCreateFromHDC")
	gdDeleteGraphics  = aquaGdi.NewProc("GdipDeleteGraphics")
	gdDrawImage       = aquaGdi.NewProc("GdipDrawImageRectI")
	pAdjustWindowRect = user32.NewProc("AdjustWindowRect")
	pSetWindowPos     = user32.NewProc("SetWindowPos")
	pShowWindowCtl    = user32.NewProc("ShowWindow")
	pGetSystemMetrics = user32.NewProc("GetSystemMetrics")
	pSetFocus         = user32.NewProc("SetFocus")
	pCreateFontW      = gdi32.NewProc("CreateFontW")
)

type aquaItem struct {
	key    string
	label  string
	cx, cy int32
}

var aquaMain = []aquaItem{
	{"play", "Jugar", 190, 225}, {"games", "Juegos", 453, 225}, {"vmu", "VMU", 716, 225}, {"music", "Música", 979, 225},
	{"settings", "Configuración", 190, 495}, {"controllers", "Mandos", 453, 495}, {"display", "Pantalla", 716, 495}, {"network", "Red", 979, 495},
}
var aquaAdvanced = []aquaItem{
	{"audio", "Audio", 99, 764}, {"video", "Vídeo", 218, 764}, {"performance", "Rendimiento", 337, 764},
	{"saves", "Guardados", 456, 764}, {"system", "BIOS/Sistema", 575, 764}, {"profiles", "Perfiles", 694, 764},
	{"debug", "Diagnóstico", 813, 764}, {"language", "Idioma", 932, 764}, {"themes", "Temas", 1051, 764},
}

func aquaImagePath(name string) string { return filepath.Join(exeDir(), "launcher", "assets", name) }
func aquaLoad(name string) uintptr {
	if v := aquaImages[name]; v != 0 {
		return v
	}
	path := aquaImagePath(name)
	if _, err := os.Stat(path); err != nil {
		return 0
	}
	var img uintptr
	p := w(path)
	result, _, _ := gdLoadImage.Call(uintptr(unsafe.Pointer(p)), uintptr(unsafe.Pointer(&img)))
	runtime.KeepAlive(p)
	if result != 0 || img == 0 {
		appendLog("Aqua: could not load %s: gdiplus=%d", name, result)
		return 0
	}
	aquaImages[name] = img
	return img
}
func aquaInit() {
	if b,e := os.ReadFile(filepath.Join(appDataDir(),"launcher_aqua_theme.txt")); e==nil { aquaNight = strings.TrimSpace(string(b))=="dark" }
	// GdiplusStartupInput { GdiplusVersion(1), DebugEventCallback, SuppressBackgroundThread, SuppressExternalCodecs }
	var input struct {
		Version    uint32
		Callback   uintptr
		NoThread   int32
		NoExternal int32
	}
	input.Version = 1
	status, _, _ := gdStartup.Call(uintptr(unsafe.Pointer(&aquaToken)), uintptr(unsafe.Pointer(&input)), 0)
	if status != 0 {
		appendLog("GDI+ initialization failed: %d", status)
		return
	}
	aquaReady = true
	if aquaLoad("dreamcast_launcher_1152.png") == 0 {
		appendLog("Aqua assets missing: launcher/assets/dreamcast_launcher_1152.png")
	}
	for _, item := range aquaMain {
		aquaLoad("button_" + item.key + "_active_1152.png")
	}
}
func aquaStop() {
	for k, im := range aquaImages {
		gdDisposeImage.Call(im)
		delete(aquaImages, k)
	}
	if aquaToken != 0 {
		gdShutdown.Call(aquaToken)
		aquaToken = 0
	}
	aquaReady = false
}
func aquaSize(wantedW, wantedH int32) {
	var rect = RECT{Right: wantedW, Bottom: wantedH}
	pAdjustWindowRect.Call(uintptr(unsafe.Pointer(&rect)), WS_FIXED, 0)
	width, height := rect.Right-rect.Left, rect.Bottom-rect.Top
	pSetWindowPos.Call(hwndMain, 0, 0, 0, uintptr(width), uintptr(height), swpNoMove|swpNoZOrder)
}
func showAquaHome() {
	saveSettings()
	for _, h := range controls {
		if h != 0 {
			pShowWindowCtl.Call(h, swHide)
		}
	}
	aquaHome = true
	aquaHovered = -1
	// Match the original Aqua layout on 1366x768 / 1080p monitors.
	screenW, _, _ := pGetSystemMetrics.Call(0)
	screenH, _, _ := pGetSystemMetrics.Call(1)
	width := int32(1152)
	if screenW < 1250 || screenH < 960 {
		width = 960
	}
	if screenW < 1050 || screenH < 820 {
		width = 800
	}
	aquaSize(width, width*3/4)
	pSetWindowTextW.Call(hwndMain, uintptr(unsafe.Pointer(w("Dreamcast Recompiled v1.2 | Aqua Nativo"))))
	pSetFocus.Call(hwndMain)
	pSetTimer.Call(hwndMain, 1, 1000, 0)
	pInvalidateRect.Call(hwndMain, 0, 1)
}
func showNativeSettings() {
	aquaHome = false
	pKillTimer.Call(hwndMain, 1)
	aquaSize(820, 690)
	for _, h := range controls {
		if h != 0 {
			pShowWindowCtl.Call(h, SW_SHOW)
		}
	}
	pSetWindowTextW.Call(hwndMain, uintptr(unsafe.Pointer(w("Dreamcast Recompiled | Configuración Nativa"))))
	pInvalidateRect.Call(hwndMain, 0, 1)
}
func aquaPaint(hwnd uintptr) uintptr {
	var ps PAINTSTRUCT
	hdc, _, _ := pBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
	if hdc == 0 {
		return 0
	}
	defer pEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
	var rc RECT
	pGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
	cw, ch := rc.Right, rc.Bottom
	// Native offscreen buffer; compositing prevents flicker when hover changes.
	mdc, _, _ := pCreateCompatibleDC.Call(hdc)
	if mdc == 0 {
		return 0
	}
	defer pDeleteDC.Call(mdc)
	bmp, _, _ := pCreateCompatibleBitmap.Call(hdc, uintptr(cw), uintptr(ch))
	if bmp == 0 {
		return 0
	}
	old, _, _ := pSelectObject.Call(mdc, bmp)
	defer func() { pSelectObject.Call(mdc, old); pDeleteObject.Call(bmp) }()
	var graphics uintptr
	if aquaReady {
		result, _, _ := gdCreateGraphics.Call(mdc, uintptr(unsafe.Pointer(&graphics)))
		if result == 0 && graphics != 0 {
			background := "dreamcast_launcher_1152.png"
            if aquaNight { background = "dreamcast_launcher_dark_1152.png" }
            if bg := aquaLoad(background); bg != 0 {
				gdDrawImage.Call(graphics, bg, 0, 0, uintptr(cw), uintptr(ch))
			}
			if aquaHovered >= 0 && aquaHovered < len(aquaMain) {
				item := aquaMain[aquaHovered]
				if overlay := aquaLoad("button_" + item.key + "_active_1152.png"); overlay != 0 {
					// Centered over the composite's original normal capsule.
					x := int32(float64(item.cx-109) * float64(cw) / 1152.)
					y := int32(float64(item.cy+127-31) * float64(ch) / 864.)
					w := int32(float64(218) * float64(cw) / 1152.)
					h := int32(float64(63) * float64(ch) / 864.)
					gdDrawImage.Call(graphics, overlay, uintptr(x), uintptr(y), uintptr(w), uintptr(h))
				}
			}
			gdDeleteGraphics.Call(graphics)
			if aquaHovered >= 0 && aquaHovered < len(aquaMain) {
				// Capsules are bitmap-only; redraw the label after the active PNG
				// so the original button text never disappears on hover.
				item := aquaMain[aquaHovered]
				x := int32(float64(item.cx-109) * float64(cw) / 1152.)
				y := int32(float64(item.cy+127-31) * float64(ch) / 864.)
				drawAquaText(mdc, item.label, x, y, int32(float64(218)*float64(cw)/1152.), int32(float64(63)*float64(ch)/864.), 0x00ffffff)
			}
		}
	}
	if !aquaReady || aquaImages["dreamcast_launcher_1152.png"] == 0 {
		b, _, _ := pCreateSolidBrush.Call(0x00edb65f)
		pFillRect.Call(mdc, uintptr(unsafe.Pointer(&rc)), b)
		pDeleteObject.Call(b)
		drawAquaText(mdc, "No se encontraron los recursos Aqua. Extrae el ZIP completo.", 30, 130, 1050, 35, 0x00ffffff)
	}
	// Replace the artwork's sample clock with the live local clock.
	// Gray matches the screenshot header; display text is native GDI text.
	scalex := float64(cw) / 1152.
	scaley := float64(ch) / 864.
	x1, y1 := int32(772*scalex), int32(17*scaley)
	x2, y2 := int32(1130*scalex), int32(83*scaley)
	rr := RECT{x1, y1, x2, y2}
	clockBg := uintptr(0x00b9b7bc)
    clockColor := uint32(0x00242530)
    if aquaNight { clockBg=0x00262a33; clockColor=0x00f5e9df }
    brush, _, _ := pCreateSolidBrush.Call(clockBg)
	pFillRect.Call(mdc, uintptr(unsafe.Pointer(&rr)), brush)
	pDeleteObject.Call(brush)
	clock := time.Now().Format("02/01/2006 15:04:05")
	drawAquaText(mdc, clock, x1, y1, x2-x1, y2-y1, clockColor)
	selected := strings.TrimSpace(getText(controls[idDiscEdit]))
	if selected != "" {
		gameName := filepath.Base(selected)
		drawAquaText(mdc, gameName, int32(220*scalex), int32(95*scaley), int32(735*scalex), int32(35*scaley), 0x00ffffff)
	}
	pBitBlt.Call(hdc, 0, 0, uintptr(cw), uintptr(ch), mdc, 0, 0, SRCCOPY)
	return 0
}
func drawAquaText(dc uintptr, s string, x, y, cw, ch int32, color uint32) {
	rect := RECT{Left: x, Top: y, Right: x + cw, Bottom: y + ch}
	pSetBkMode.Call(dc, TRANSPARENT)
	pSetTextColor.Call(dc, uintptr(color))
	// Segoe UI from Windows; no external font dependency.
	fontHeight := int32(-23)
	font, _, _ := pCreateFontW.Call(uintptr(uint32(fontHeight)), 0, 0, 0, 700, 0, 0, 0, 1, 0, 0, 0, 0, uintptr(unsafe.Pointer(w("Segoe UI"))))
	ownFont := font != 0
	if font == 0 {
		font, _, _ = pGetStockObject.Call(DEFAULT_GUI_FONT)
	}
	old, _, _ := pSelectObject.Call(dc, font)
	u := w(s)
	pDrawTextW.Call(dc, uintptr(unsafe.Pointer(u)), uintptr(^uint32(0)), uintptr(unsafe.Pointer(&rect)), DT_CENTER|DT_VCENTER|DT_SINGLELINE)
	pSelectObject.Call(dc, old)
	if ownFont {
		pDeleteObject.Call(font)
	}
	runtime.KeepAlive(u)
}
func aquaHit(lp uintptr) int {
	var rc RECT
	pGetClientRect.Call(hwndMain, uintptr(unsafe.Pointer(&rc)))
	if rc.Right <= 0 || rc.Bottom <= 0 {
		return -1
	}
	x := float64(int16(lp&0xffff)) * 1152. / float64(rc.Right)
	y := float64(int16((lp>>16)&0xffff)) * 864. / float64(rc.Bottom)
	for i, it := range aquaMain {
		if x >= float64(it.cx-115) && x <= float64(it.cx+115) && y >= float64(it.cy-105) && y <= float64(it.cy+155) {
			return i
		}
	}
	for i, it := range aquaAdvanced {
		if x >= float64(it.cx-54) && x <= float64(it.cx+54) && y >= 723 && y <= 830 {
			return len(aquaMain) + i
		}
	}
	return -1
}
func aquaHover(lp uintptr) {
	i := aquaHit(lp)
	if i != aquaHovered {
		aquaHovered = i
		pInvalidateRect.Call(hwndMain, 0, 0)
	}
}
func aquaClick(lp uintptr) {
	i := aquaHit(lp)
	if i >= 0 {
		aquaActivate(i)
	}
}
func aquaKeyboard(wp uintptr) {
	switch wp {
	case 0x25:
		aquaFocused = (aquaFocused + len(aquaMain) - 1) % len(aquaMain)
	case 0x27:
		aquaFocused = (aquaFocused + 1) % len(aquaMain)
	case 0x26:
		aquaFocused = (aquaFocused + len(aquaMain) - 4) % len(aquaMain)
	case 0x28:
		aquaFocused = (aquaFocused + 4) % len(aquaMain)
	case 0x0d, 0x20:
		aquaActivate(aquaFocused)
		return
	case 0x4f:
		browseDisc()
		return
	default:
		return
	}
	aquaHovered = aquaFocused
	pInvalidateRect.Call(hwndMain, 0, 0)
}
func aquaActivate(i int) {
	if i < 0 {
		return
	}
	var key string
	if i < len(aquaMain) {
		key = aquaMain[i].key
	} else if i-len(aquaMain) < len(aquaAdvanced) {
		key = aquaAdvanced[i-len(aquaMain)].key
	} else {
		return
	}
	switch key {
	case "play":
		startPlay()
	case "games":
		openGameLibrary()
	case "controllers":
		openController()
	case "vmu", "saves":
        openVMUWindow()
	case "music":
		msgbox("La biblioteca de música está disponible en el launcher Python legado, todavía no migrada a Win32.", "Música", MB_OK|MB_ICONINFORMATION)
	case "system":
		openNativeFolder(filepath.Join(appDataDir(), "system"))
	case "profiles":
		openNativeFolder(filepath.Join(appDataDir(), "games"))
	case "debug":
		openLogs()
	case "language":
		msgbox("El launcher nativo está disponible en español. La traducción completa se agregará en una revisión.", "Idioma", MB_OK|MB_ICONINFORMATION)
	case "themes":
        aquaNight = !aquaNight
        name := "classic"
        if aquaNight { name = "dark" }
        _ = os.MkdirAll(appDataDir(),0755)
        _ = os.WriteFile(filepath.Join(appDataDir(),"launcher_aqua_theme.txt"), []byte(name+"\n"),0644)
        pInvalidateRect.Call(hwndMain,0,1)
	case "settings", "network", "display", "audio", "video", "performance":
		showNativeSettings()
	default:
		showNativeSettings()
	}
}
func openNativeFolder(path string) {
	_ = os.MkdirAll(path, 0755)
	pShellExecuteW.Call(hwndMain, uintptr(unsafe.Pointer(w("open"))), uintptr(unsafe.Pointer(w(path))), 0, 0, SW_SHOW)
}
func aquaPreflight() error {
	p := aquaImagePath("dreamcast_launcher_1152.png")
	if _, err := os.Stat(p); err != nil {
		return fmt.Errorf("missing Aqua artwork: %s", p)
	}
	return nil
}
