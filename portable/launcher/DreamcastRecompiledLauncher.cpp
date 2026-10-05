#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "bcrypt.lib")

namespace fs = std::filesystem;

static constexpr wchar_t kTitle[] = L"Dreamcast Recompiled - ChuChu Rocket! Online Preview";
static constexpr wchar_t kIpSha[] = L"9925c18f0857ccd363cb8d641122f410083b133102ac99f06bae6bb83f9aad4d";
static constexpr wchar_t kBootSha[] = L"b43cb7977871e0c1ce7971da3da7f5a7ea4eb6903a61fbefc5a50de4230d2927";

enum : int {
    IDC_GAME = 1001, IDC_BROWSE, IDC_STATUS, IDC_PLAY, IDC_LOGS,
    IDC_RENDER, IDC_AUDIO, IDC_PERF, IDC_DEBUG, IDC_PVRPROFILE, IDC_CLOCK,
    IDC_PAD_BACKEND, IDC_PAD_DEVICE, IDC_PAD_DEADZONE, IDC_RESET_PAD
};

static HWND g_hwnd{};
static HWND g_game{}, g_status{}, g_render{}, g_audio{}, g_perf{}, g_debug{}, g_pvrProfile{}, g_clock{};
static HWND g_padBackend{}, g_padDevice{}, g_padDeadzone{};
static fs::path g_root, g_local, g_settings, g_gameCache, g_controllerProfile;

static std::wstring q(const fs::path& p) { return L"\"" + p.wstring() + L"\""; }
static std::wstring qstr(const std::wstring& s) { return L"\"" + s + L"\""; }

static fs::path exe_dir() {
    std::vector<wchar_t> buf(32768);
    DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    return fs::path(std::wstring(buf.data(), n)).parent_path();
}

static fs::path local_appdata() {
    PWSTR value = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &value))) return {};
    fs::path out(value);
    CoTaskMemFree(value);
    return out;
}

static std::wstring ctl_text(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(static_cast<size_t>(n), L'\0');
    if (n) GetWindowTextW(h, s.data(), n + 1);
    return s;
}

static void set_status(const std::wstring& s) {
    SetWindowTextW(g_status, s.c_str());
    UpdateWindow(g_status);
}

static std::wstring ini_get(const wchar_t* section, const wchar_t* key, const wchar_t* def = L"") {
    wchar_t buf[32768]{};
    GetPrivateProfileStringW(section, key, def, buf, static_cast<DWORD>(std::size(buf)), g_settings.c_str());
    return buf;
}

static void ini_set(const wchar_t* section, const wchar_t* key, const std::wstring& value) {
    WritePrivateProfileStringW(section, key, value.c_str(), g_settings.c_str());
}

static bool file_sha256(const fs::path& path, std::wstring& out) {
    BCRYPT_ALG_HANDLE alg{};
    BCRYPT_HASH_HANDLE hash{};
    DWORD objLen = 0, cb = 0, hashLen = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    auto closeAlg = [&] { if (hash) BCryptDestroyHash(hash); if (alg) BCryptCloseAlgorithmProvider(alg, 0); };

    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objLen), sizeof(objLen), &cb, 0) != 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLen), sizeof(hashLen), &cb, 0) != 0) {
        closeAlg(); return false;
    }
    std::vector<UCHAR> obj(objLen), digest(hashLen);
    if (BCryptCreateHash(alg, &hash, obj.data(), objLen, nullptr, 0, 0) != 0) { closeAlg(); return false; }

    std::ifstream in(path, std::ios::binary);
    if (!in) { closeAlg(); return false; }
    std::vector<char> buffer(1 << 20);
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto got = in.gcount();
        if (got > 0 && BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(got), 0) != 0) {
            closeAlg(); return false;
        }
    }
    if (BCryptFinishHash(hash, digest.data(), hashLen, 0) != 0) { closeAlg(); return false; }
    closeAlg();

    std::wostringstream ss;
    ss << std::hex << std::setfill(L'0');
    for (auto b : digest) ss << std::setw(2) << static_cast<unsigned>(b);
    out = ss.str();
    return true;
}

static bool run_hidden(const fs::path& exe, const std::wstring& args, const fs::path& cwd, DWORD* exitCode = nullptr) {
    std::wstring cmd = q(exe) + L" " + args;
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.c_str(), mutableCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, cwd.c_str(), &si, &pi)) return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 0;
    GetExitCodeProcess(pi.hProcess, &rc);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (exitCode) *exitCode = rc;
    return rc == 0;
}

static bool parse_map_title(const fs::path& map, std::string& title) {
    std::ifstream in(map);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("title=", 0) == 0) {
            title = line.substr(6);
            while (!title.empty() && (title.back() == '\r' || title.back() == '\n' || title.back() == ' ')) title.pop_back();
            return true;
        }
    }
    return false;
}

static bool prepare_game(const fs::path& disc) {
    const fs::path probe = g_root / L"tools" / L"dc_disc_probe.exe";
    const fs::path prep  = g_root / L"tools" / L"dc_boot_prepare.exe";
    if (!fs::exists(probe) || !fs::exists(prep)) {
        MessageBoxW(g_hwnd, L"Faltan tools\\dc_disc_probe.exe o tools\\dc_boot_prepare.exe.", kTitle, MB_ICONERROR);
        return false;
    }

    fs::create_directories(g_gameCache);
    const fs::path ip = g_gameCache / L"IP.BIN";
    const fs::path rawBoot = g_gameCache / L"BOOT.DISC.BIN";
    const fs::path boot = g_gameCache / L"BOOT.BIN";
    const fs::path bootstrap = g_gameCache / L"BOOTSTRAP.BIN";
    const fs::path map = g_gameCache / L"disc.map";

    bool cached = fs::exists(ip) && fs::exists(boot) && fs::exists(bootstrap) && fs::exists(map)
               && ini_get(L"Game", L"source") == disc.wstring();

    if (!cached) {
        set_status(L"Analizando tu copia original de ChuChu Rocket...");
        std::wstring args = q(disc)
            + L" --extract-ip=" + q(ip)
            + L" --extract-boot=" + q(rawBoot)
            + L" --extract-disc-map=" + q(map);
        DWORD rc{};
        if (!run_hidden(probe, args, g_root, &rc)) {
            MessageBoxW(g_hwnd, (L"No se pudo analizar el CDI/GDI. RC=" + std::to_wstring(rc)).c_str(), kTitle, MB_ICONERROR);
            return false;
        }

        std::string mapTitle;
        if (!parse_map_title(map, mapTitle) || mapTitle != "CHUCHU ROCKET") {
            MessageBoxW(g_hwnd, L"La imagen seleccionada no fue identificada como CHUCHU ROCKET.", kTitle, MB_ICONERROR);
            return false;
        }

        set_status(L"Preparando el bootstrap local...");
        args = q(ip) + L" " + q(rawBoot)
             + L" --mode=auto --boot-out=" + q(boot)
             + L" --combined-out=" + q(bootstrap);
        if (!run_hidden(prep, args, g_root, &rc)) {
            MessageBoxW(g_hwnd, (L"No se pudo preparar el bootstrap. RC=" + std::to_wstring(rc)).c_str(), kTitle, MB_ICONERROR);
            return false;
        }
    }

    set_status(L"Verificando revisión del juego...");
    std::wstring ipSha, bootSha;
    if (!file_sha256(ip, ipSha) || !file_sha256(boot, bootSha)) {
        MessageBoxW(g_hwnd, L"No se pudieron calcular los hashes de la copia preparada.", kTitle, MB_ICONERROR);
        return false;
    }
    if (_wcsicmp(ipSha.c_str(), kIpSha) != 0 || _wcsicmp(bootSha.c_str(), kBootSha) != 0) {
        std::wstring msg =
            L"Esta revisión de ChuChu Rocket! todavía no está soportada por este guest precompilado.\n\n"
            L"IP.BIN: " + ipSha + L"\nBOOT.BIN: " + bootSha;
        MessageBoxW(g_hwnd, msg.c_str(), kTitle, MB_ICONWARNING);
        return false;
    }

    ini_set(L"Game", L"source", disc.wstring());
    ini_set(L"Game", L"ip_sha256", ipSha);
    ini_set(L"Game", L"boot_sha256", bootSha);
    set_status(L"ChuChu Rocket! V1.007 compatible - listo.");
    return true;
}

static void write_controller_profile() {
    fs::create_directories(g_controllerProfile.parent_path());
    int backendIndex = static_cast<int>(SendMessageW(g_padBackend, CB_GETCURSEL, 0, 0));
    const char* backend = backendIndex == 1 ? "ps4" : backendIndex == 2 ? "xinput" : backendIndex == 3 ? "winmm" : "auto";
    std::wstring device = ctl_text(g_padDevice);
    std::wstring deadzone = ctl_text(g_padDeadzone);
    if (device.empty()) device = L"0";
    if (deadzone.empty()) deadzone = L"0.18";

    std::ofstream out(g_controllerProfile, std::ios::binary | std::ios::trunc);
    out << "[DreamcastRecompController]\n"
        << "version=2\nbackend=" << backend << "\ndevice=" << std::string(device.begin(), device.end())
        << "\ndeadzone=" << std::string(deadzone.begin(), deadzone.end()) << "\n"
        << "a=logical:a\nb=logical:b\nx=logical:x\ny=logical:y\nstart=logical:start\n"
        << "up=logical:up\ndown=logical:down\nleft=logical:left\nright=logical:right\n"
        << "c=logical:c\nz=logical:z\njoy_x=logical:joy_x\njoy_y=logical:joy_y\n"
        << "joy2_x=logical:joy2_x\njoy2_y=logical:joy2_y\n"
        << "ltrig=logical:ltrig\nrtrig=logical:rtrig\n";
}

static void save_settings() {
    ini_set(L"Game", L"source", ctl_text(g_game));
    ini_set(L"Video", L"renderer", std::to_wstring(SendMessageW(g_render, CB_GETCURSEL, 0, 0)));
    ini_set(L"Audio", L"enabled", Button_GetCheck(g_audio) == BST_CHECKED ? L"1" : L"0");
    ini_set(L"Diagnostics", L"perf", Button_GetCheck(g_perf) == BST_CHECKED ? L"1" : L"0");
    ini_set(L"Diagnostics", L"debug", Button_GetCheck(g_debug) == BST_CHECKED ? L"1" : L"0");
    ini_set(L"Diagnostics", L"pvr_profile", Button_GetCheck(g_pvrProfile) == BST_CHECKED ? L"1" : L"0");
    ini_set(L"Timing", L"clock", std::to_wstring(SendMessageW(g_clock, CB_GETCURSEL, 0, 0)));
    ini_set(L"Controller", L"backend", std::to_wstring(SendMessageW(g_padBackend, CB_GETCURSEL, 0, 0)));
    ini_set(L"Controller", L"device", ctl_text(g_padDevice));
    ini_set(L"Controller", L"deadzone", ctl_text(g_padDeadzone));
}

static void load_settings() {
    SetWindowTextW(g_game, ini_get(L"Game", L"source").c_str());

    int renderer = _wtoi(ini_get(L"Video", L"renderer", L"0").c_str());
    SendMessageW(g_render, CB_SETCURSEL, renderer >= 0 && renderer <= 2 ? renderer : 0, 0);
    Button_SetCheck(g_audio, ini_get(L"Audio", L"enabled", L"1") == L"1" ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_perf, ini_get(L"Diagnostics", L"perf", L"0") == L"1" ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_debug, ini_get(L"Diagnostics", L"debug", L"0") == L"1" ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(g_pvrProfile, ini_get(L"Diagnostics", L"pvr_profile", L"0") == L"1" ? BST_CHECKED : BST_UNCHECKED);

    int clock = _wtoi(ini_get(L"Timing", L"clock", L"0").c_str());
    SendMessageW(g_clock, CB_SETCURSEL, clock == 1 ? 1 : 0, 0);
    int backend = _wtoi(ini_get(L"Controller", L"backend", L"0").c_str());
    SendMessageW(g_padBackend, CB_SETCURSEL, backend >= 0 && backend <= 3 ? backend : 0, 0);
    SetWindowTextW(g_padDevice, ini_get(L"Controller", L"device", L"0").c_str());
    SetWindowTextW(g_padDeadzone, ini_get(L"Controller", L"deadzone", L"0.18").c_str());
}

static void browse_game() {
    wchar_t file[32768]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"Dreamcast images (*.cdi;*.gdi)\0*.cdi;*.gdi\0CDI (*.cdi)\0*.cdi\0GDI (*.gdi)\0*.gdi\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = static_cast<DWORD>(std::size(file));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(g_game, file);
        set_status(L"Copia seleccionada. Pulsa PLAY para verificarla.");
    }
}

static bool launch_game() {
    fs::path disc = ctl_text(g_game);
    if (disc.empty() || !fs::exists(disc)) {
        MessageBoxW(g_hwnd, L"Selecciona tu CDI/GDI original de ChuChu Rocket!.", kTitle, MB_ICONINFORMATION);
        return false;
    }
    auto ext = disc.extension().wstring();
    if (_wcsicmp(ext.c_str(), L".cdi") != 0 && _wcsicmp(ext.c_str(), L".gdi") != 0) {
        MessageBoxW(g_hwnd, L"El archivo debe ser .cdi o .gdi.", kTitle, MB_ICONWARNING);
        return false;
    }

    save_settings();
    write_controller_profile();
    if (!prepare_game(disc)) return false;

    const fs::path runner = g_root / L"dreamcast_program.exe";
    const fs::path runtime = g_root / L"DreamcastRuntime.dll";
    const fs::path guest = g_root / L"dcr_game_module.dll";
    const fs::path network = g_root / L"plugins" / L"dcr_network.dll";
    if (!fs::exists(runner) || !fs::exists(runtime) || !fs::exists(guest) || !fs::exists(network)) {
        MessageBoxW(g_hwnd,
            L"El paquete portable está incompleto. Deben existir dreamcast_program.exe, DreamcastRuntime.dll, dcr_game_module.dll y plugins\\dcr_network.dll.",
            kTitle, MB_ICONERROR);
        return false;
    }

    fs::create_directories(g_local / L"system");
    fs::create_directories(g_root / L"logs");
    const fs::path bootstrap = g_gameCache / L"BOOTSTRAP.BIN";
    const fs::path discMap = g_gameCache / L"disc.map";

    SetEnvironmentVariableW(L"DCR_EXTERNAL_GAME_IMAGE", bootstrap.c_str());
    SetEnvironmentVariableW(L"DCR_SYSTEM_DATA_ROOT", (g_local / L"system").c_str());
    SetEnvironmentVariableW(L"DCR_PLUGIN_DIR", (g_root / L"plugins").c_str());
    SetEnvironmentVariableW(L"DCR_TRACE_HISTORY", Button_GetCheck(g_debug) == BST_CHECKED ? L"1" : L"0");

    std::wstring args =
        L"--commercial-boot --disc-map=" + q(discMap)
        + L" --pvr-window --pvr-frame-sync --fast-dispatch --direct-dispatch --sh4-tick-batch=256"
        + L" --maple-host-input --controller-profile=" + q(g_controllerProfile);

    int renderer = static_cast<int>(SendMessageW(g_render, CB_GETCURSEL, 0, 0));
    if (renderer == 2) args += L" --pvr-mt";
    else args += L" --pvr-gpu --pvr-render-done-scheduled --pvr-mt";

    if (Button_GetCheck(g_audio) == BST_CHECKED)
        args += L" --aica-arm7 --aica-arm7-slice=128 --aica-arm7-boot=32768 --aica-play";
    else
        args += L" --aica-arm7 --aica-arm7-slice=128 --aica-arm7-boot=32768";

    int clock = static_cast<int>(SendMessageW(g_clock, CB_GETCURSEL, 0, 0));
    if (clock == 1) args += L" --device-clock";
    else args += L" --device-clock-host --device-clock-host-max-catchup=4096";

    int heartbeat = Button_GetCheck(g_debug) == BST_CHECKED ? 500 : 1000;
    args += L" --diag-heartbeat-ms=" + std::to_wstring(heartbeat);
    if (Button_GetCheck(g_perf) == BST_CHECKED) args += L" --perf-profile --perf-sample-stride=64";
    if (Button_GetCheck(g_debug) == BST_CHECKED) args += L" --host-window";
    if (Button_GetCheck(g_pvrProfile) == BST_CHECKED) args += L" --pvr-profile --host-window";

    std::wstring cmd = q(runner) + L" " + args;
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');
    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(runner.c_str(), mutableCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP, nullptr, g_root.c_str(), &si, &pi)) {
        MessageBoxW(g_hwnd, (L"No se pudo iniciar DreamcastRecompiled. Win32=" + std::to_wstring(GetLastError())).c_str(), kTitle, MB_ICONERROR);
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    set_status(L"Ejecutando ChuChu Rocket! Online Preview...");
    return true;
}

static HWND label(HWND p, const wchar_t* text, int x, int y, int w, int h) {
    return CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, p, nullptr, nullptr, nullptr);
}
static HWND combo(HWND p, int id, int x, int y, int w) {
    return CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                         x, y, w, 200, p, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
}
static HWND edit(HWND p, int id, int x, int y, int w, const wchar_t* value) {
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", value, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                             x, y, w, 24, p, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    return h;
}
static HWND check(HWND p, int id, const wchar_t* text, int x, int y, int w) {
    return CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                         x, y, w, 24, p, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
}

static void create_ui(HWND hwnd) {
    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    auto apply = [&](HWND h){ SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); };

    HWND title = label(hwnd, L"Dreamcast Recompiled", 22, 18, 420, 26); apply(title);
    HWND sub = label(hwnd, L"ChuChu Rocket! Online Preview - guest precompilado", 22, 45, 520, 22); apply(sub);

    label(hwnd, L"Juego original (.cdi / .gdi)", 22, 82, 260, 20);
    g_game = edit(hwnd, IDC_GAME, 22, 104, 500, L""); apply(g_game);
    HWND browse = CreateWindowW(L"BUTTON", L"Seleccionar...", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 532,103,100,26,hwnd,(HMENU)IDC_BROWSE,nullptr,nullptr); apply(browse);

    g_status = label(hwnd, L"Selecciona tu copia original para comenzar.", 22, 137, 610, 24); apply(g_status);

    label(hwnd, L"Video", 22, 180, 120, 20);
    label(hwnd, L"Render", 22, 207, 80, 20);
    g_render = combo(hwnd, IDC_RENDER, 105, 203, 210); apply(g_render);
    SendMessageW(g_render, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Auto (GPU recomendado)"));
    SendMessageW(g_render, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"GPU - Direct3D 11"));
    SendMessageW(g_render, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Software - CPU"));

    label(hwnd, L"Timing", 340, 207, 80, 20);
    g_clock = combo(hwnd, IDC_CLOCK, 410, 203, 222); apply(g_clock);
    SendMessageW(g_clock, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Host sincronizado"));
    SendMessageW(g_clock, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Device determinista"));

    label(hwnd, L"Audio y diagnóstico", 22, 248, 180, 20);
    g_audio = check(hwnd, IDC_AUDIO, L"Audio", 22, 274, 100); apply(g_audio);
    g_perf = check(hwnd, IDC_PERF, L"Modo rendimiento", 135, 274, 145); apply(g_perf);
    g_debug = check(hwnd, IDC_DEBUG, L"Debug", 295, 274, 90); apply(g_debug);
    g_pvrProfile = check(hwnd, IDC_PVRPROFILE, L"PVR profile", 400, 274, 120); apply(g_pvrProfile);

    label(hwnd, L"Mando", 22, 318, 120, 20);
    label(hwnd, L"Backend", 22, 345, 80, 20);
    g_padBackend = combo(hwnd, IDC_PAD_BACKEND, 105, 341, 210); apply(g_padBackend);
    SendMessageW(g_padBackend, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Auto"));
    SendMessageW(g_padBackend, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"PS4 nativo / DirectInput"));
    SendMessageW(g_padBackend, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"XInput / DS4Windows / Steam"));
    SendMessageW(g_padBackend, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"WinMM genérico"));

    label(hwnd, L"Dispositivo", 340, 345, 78, 20);
    g_padDevice = edit(hwnd, IDC_PAD_DEVICE, 420, 341, 52, L"0"); apply(g_padDevice);
    label(hwnd, L"Deadzone", 488, 345, 70, 20);
    g_padDeadzone = edit(hwnd, IDC_PAD_DEADZONE, 558, 341, 74, L"0.18"); apply(g_padDeadzone);

    HWND reset = CreateWindowW(L"BUTTON", L"Restablecer mando", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 105,377,145,26,hwnd,(HMENU)IDC_RESET_PAD,nullptr,nullptr); apply(reset);
    HWND note = label(hwnd, L"Mapeo Dreamcast lógico: A/B/X/Y, Start, D-pad, sticks, L/R. Se guarda por usuario.", 270, 380, 360, 35); apply(note);

    HWND play = CreateWindowW(L"BUTTON", L"PLAY", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 22,438,180,42,hwnd,(HMENU)IDC_PLAY,nullptr,nullptr); apply(play);
    HWND logs = CreateWindowW(L"BUTTON", L"Abrir logs", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 218,438,110,42,hwnd,(HMENU)IDC_LOGS,nullptr,nullptr); apply(logs);
    HWND foot = label(hwnd, L"No incluye datos del juego. El bootstrap se extrae localmente de tu CDI/GDI.", 345, 443, 290, 34); apply(foot);

    load_settings();
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_BROWSE: browse_game(); return 0;
                case IDC_PLAY: launch_game(); return 0;
                case IDC_LOGS: {
                    fs::create_directories(g_root / L"logs");
                    ShellExecuteW(hwnd, L"open", (g_root / L"logs").c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    return 0;
                }
                case IDC_RESET_PAD:
                    SendMessageW(g_padBackend, CB_SETCURSEL, 0, 0);
                    SetWindowTextW(g_padDevice, L"0");
                    SetWindowTextW(g_padDeadzone, L"0.18");
                    return 0;
            }
            break;
        case WM_CLOSE:
            save_settings();
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int show) {
    g_root = exe_dir();
    g_local = local_appdata() / L"DreamcastRecompiled";
    g_settings = g_local / L"launcher" / L"settings.ini";
    g_gameCache = g_local / L"games" / L"CHUCHU_ROCKET" / L"portable";
    g_controllerProfile = g_local / L"profiles" / L"controller_profile.ini";
    fs::create_directories(g_settings.parent_path());

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInst;
    wc.lpfnWndProc = wndproc;
    wc.lpszClassName = L"DCRPortableLauncher";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    g_hwnd = CreateWindowExW(0, wc.lpszClassName, kTitle,
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             CW_USEDEFAULT, CW_USEDEFAULT, 675, 540,
                             nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) return 1;
    create_ui(g_hwnd);
    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);

    MSG m{};
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return static_cast<int>(m.wParam);
}
