//go:build windows

package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"unsafe"
)

const (
	libraryAdd     = 411
	libraryUse     = 412
	libraryRemove  = 413
	libraryPlay    = 414
	libraryClose   = 415
	libraryList    = 416
	lbAddString    = 0x0180
	lbResetContent = 0x0184
	lbGetCurSel    = 0x0188
	lbSetCurSel    = 0x0186
	lbErr          = 0xffff
	lbsNotify      = 0x0001
	lbnDblClick    = 2
)

var (
	hwndLibrary       uintptr
	hwndLibraryList   uintptr
	libraryWndProcPtr uintptr
	libraryControls   = map[int]uintptr{}
)

// Copy the previous Tkinter user's most important preferences once. Native
// settings subsequently live in launcher.json; the legacy file is untouched.
func migrateLegacyAquaSettings() {
	path := filepath.Join(appDataDir(), "launcher_v1_2", "settings.json")
	b, e := os.ReadFile(path)
	if e != nil {
		return
	}
	var v struct {
		Library       []string `json:"library"`
		LastGame      string   `json:"last_game"`
		Renderer      string   `json:"renderer"`
		Network       string   `json:"network_mode"`
		Build         string   `json:"build_mode"`
		Audio         bool     `json:"audio"`
		AudioMode     string   `json:"audio_mode"`
		Heartbeats    bool     `json:"heartbeats"`
		ShowConsole   bool     `json:"show_console"`
		NativeSeeds   bool     `json:"native_seeds"`
		NativePatches bool     `json:"native_patches"`
	}
	if e = json.Unmarshal(b, &v); e != nil {
		return
	}
	settings.Library = append(settings.Library, v.Library...)
	settings.Disc = v.LastGame
	switch v.Renderer {
	case "dx11":
		settings.Renderer = 1
	case "software":
		settings.Renderer = 2
	}
	switch v.Network {
	case "dcnet":
		settings.NetworkMode = 1
	case "off":
		settings.NetworkMode = 2
	}
	switch v.Build {
	case "reuse":
		settings.BuildMode = 1
	case "full":
		settings.BuildMode = 2
	}
	switch v.AudioMode {
	case "basic":
		settings.AudioMode = 1
	case "off":
		settings.AudioMode = 2
	}
	settings.Heartbeats = v.Heartbeats
	settings.ShowConsole = v.ShowConsole
	settings.NativeSeeds = v.NativeSeeds
	settings.NativePatches = v.NativePatches
	appendLog("Aqua preferences migrated from legacy Tkinter; games=%d", len(settings.Library))
}
func gameValid(path string) bool {
	ext := strings.ToLower(filepath.Ext(path))
	if ext != ".cdi" && ext != ".iso" && ext != ".gdi" {
		return false
	}
	s, e := os.Stat(path)
	return e == nil && !s.IsDir()
}
func addLibraryGame(path string) {
	if !gameValid(path) {
		return
	}
	for _, g := range settings.Library {
		if strings.EqualFold(g, path) {
			return
		}
	}
	settings.Library = append(settings.Library, path)
}
func syncLibrary() {
	if hwndLibraryList == 0 {
		return
	}
	send(hwndLibraryList, lbResetContent, 0, 0)
	current := getText(controls[idDiscEdit])
	target := -1
	for i, path := range settings.Library {
		label := filepath.Base(path)
		if !gameValid(path) {
			label += "   [imagen no encontrada]"
		}
		send(hwndLibraryList, lbAddString, 0, uintptr(unsafe.Pointer(w(label))))
		if strings.EqualFold(path, current) {
			target = i
		}
	}
	if target >= 0 {
		send(hwndLibraryList, lbSetCurSel, uintptr(target), 0)
	}
}
func selectedLibraryIndex() int {
	id := send(hwndLibraryList, lbGetCurSel, 0, 0)
	if id == lbErr {
		return -1
	}
	i := int(id)
	if i >= len(settings.Library) || i < 0 {
		return -1
	}
	return i
}
func chooseLibraryGame() bool {
	i := selectedLibraryIndex()
	if i < 0 {
		return false
	}
	path := settings.Library[i]
	if !gameValid(path) {
		msgbox("No se encontró la imagen: "+path, "Biblioteca", MB_OK|MB_ICONERROR)
		return false
	}
	setText(controls[idDiscEdit], path)
	saveSettings()
	return true
}
func libraryWndProc(hwnd uintptr, msg uint32, wp, lp uintptr) uintptr {
	switch msg {
	case WM_COMMAND:
		id := int(wp & 0xffff)
		if id == libraryList && int((wp>>16)&0xffff) == lbnDblClick {
			if chooseLibraryGame() {
				pDestroyWindow.Call(hwnd)
				startPlay()
			}
			return 0
		}
		switch id {
		case libraryAdd:
			old := getText(controls[idDiscEdit])
			browseDisc()
			got := getText(controls[idDiscEdit])
			if got != old || gameValid(got) {
				addLibraryGame(got)
			}
			syncLibrary()
			saveSettings()
		case libraryUse:
			if chooseLibraryGame() {
				pDestroyWindow.Call(hwnd)
			}
		case libraryPlay:
			if chooseLibraryGame() {
				pDestroyWindow.Call(hwnd)
				startPlay()
			}
		case libraryRemove:
			i := selectedLibraryIndex()
			if i >= 0 {
				settings.Library = append(settings.Library[:i], settings.Library[i+1:]...)
				syncLibrary()
				saveSettings()
			}
		case libraryClose:
			pDestroyWindow.Call(hwnd)
		}
		return 0
	case WM_CLOSE:
		pDestroyWindow.Call(hwnd)
		return 0
	case WM_DESTROY:
		hwndLibrary = 0
		hwndLibraryList = 0
		libraryControls = map[int]uintptr{}
		return 0
	}
	r, _, _ := pDefWindowProcW.Call(hwnd, uintptr(msg), wp, lp)
	return r
}
func openGameLibrary() {
	if hwndLibrary != 0 {
		pShowWindow.Call(hwndLibrary, SW_SHOW)
		return
	}
	inst, _, _ := pGetModuleHandleW.Call(0)
	cls := w("DCRAquaGameLibraryV16")
	libraryWndProcPtr = syscall.NewCallback(libraryWndProc)
	icon, _, _ := pLoadIconW.Call(inst, 1)
	wc := WNDCLASSEX{CbSize: uint32(unsafe.Sizeof(WNDCLASSEX{})), WndProc: libraryWndProcPtr, Instance: inst, Icon: icon, IconSm: icon, Background: 6, ClassName: cls}
	pRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))
	h, _, _ := pCreateWindowExW.Call(0, uintptr(unsafe.Pointer(cls)), uintptr(unsafe.Pointer(w("Dreamcast Recompiled | Biblioteca de juegos"))), WS_FIXED, 250, 120, 760, 485, hwndMain, 0, inst, 0)
	if h == 0 {
		return
	}
	hwndLibrary = h
	hwndLibraryList = createFor(h, libraryControls, "LISTBOX", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|0x00200000|lbsNotify, 20, 18, 702, 328, libraryList)
	createFor(h, libraryControls, "BUTTON", "Agregar CDI/GDI/ISO", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 20, 367, 170, 37, libraryAdd)
	createFor(h, libraryControls, "BUTTON", "Seleccionar", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 202, 367, 113, 37, libraryUse)
	createFor(h, libraryControls, "BUTTON", "Jugar", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 326, 367, 102, 37, libraryPlay)
	createFor(h, libraryControls, "BUTTON", "Quitar", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 440, 367, 102, 37, libraryRemove)
	createFor(h, libraryControls, "BUTTON", "Cerrar", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, 557, 367, 160, 37, libraryClose)
	syncLibrary()
	pShowWindow.Call(h, SW_SHOW)
	pUpdateWindow.Call(h)
}
