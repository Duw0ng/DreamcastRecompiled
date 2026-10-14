package main

// Native equivalents of the older Python guest generators. These are kept as
// standalone, testable pure transformations; the actual SH-4 decode/recompile
// remains the project's existing C++ dc_raw_recomp executable.
import (
	"embed"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

//go:embed templates/*
var templates embed.FS

func tpl(name string) string {
	b, e := templates.ReadFile("templates/" + name)
	if e != nil {
		panic(e)
	}
	return string(b)
}
func readText(path string) (string, error) {
	b, e := os.ReadFile(path)
	if e != nil {
		return "", e
	}
	return string(b), nil
}
func writeText(path, stringValue string) error {
	stringValue = strings.ReplaceAll(stringValue, "\r\n", "\n")
	if prior, e := readText(path); e == nil && prior == stringValue {
		return nil
	}
	return os.WriteFile(path, []byte(stringValue), 0644)
}
func replaceFirst(s, old, new string) string {
	return strings.Replace(s, old, new, 1)
}
func globSorted(pattern string) []string {
	a, _ := filepath.Glob(pattern)
	sort.Strings(a)
	return a
}
func discoverRegistration(cpp string) (string, error) {
	candidates := append([]string{filepath.Join(cpp, "generated_program.hpp")}, globSorted(filepath.Join(cpp, "generated_*.hpp"))...)
	re := regexp.MustCompile(`\bvoid\s+(register_recompiled_program|register_generated_[A-Za-z0-9_]+)\s*\(DCRuntime&`)
	for _, file := range candidates {
		s, e := readText(file)
		if e != nil {
			continue
		}
		if m := re.FindStringSubmatch(s); len(m) > 1 {
			return m[1], nil
		}
	}
	return "", fmt.Errorf("[MODULAR] native: registration function not found in %s", cpp)
}
func discoverSources(cpp string) ([]string, error) {
	parts := globSorted(filepath.Join(cpp, "generated_program_part_*.cpp"))
	var files []string
	if len(parts) > 0 {
		if exists(filepath.Join(cpp, "generated_program.cpp")) {
			files = append(files, "generated_program.cpp")
		}
		for _, p := range parts {
			files = append(files, filepath.Base(p))
		}
		return files, nil
	}
	if exists(filepath.Join(cpp, "generated_program.cpp")) {
		return []string{"generated_program.cpp"}, nil
	}
	for _, p := range globSorted(filepath.Join(cpp, "generated_*.cpp")) {
		name := filepath.Base(p)
		if name != "generated_runner.cpp" && name != "generated_compile_test.cpp" && name != "generated_main.cpp" {
			files = append(files, name)
		}
	}
	if exists(filepath.Join(cpp, "generated_main.cpp")) {
		files = append(files, "generated_main.cpp")
	}
	if len(files) == 0 {
		return nil, fmt.Errorf("[MODULAR] generated sources not found")
	}
	return files, nil
}
func patchRunner(cpp string) error {
	p := filepath.Join(cpp, "generated_runner.cpp")
	if !exists(p) {
		return nil
	}
	s, e := readText(p)
	if e != nil {
		return e
	}
	if !strings.Contains(s, `#include "dcr_game_api.hpp"`) {
		i := strings.Index(s, "#include")
		if i >= 0 {
			j := strings.Index(s[i:], "\n")
			if j >= 0 {
				s = s[:i+j+1] + "#include \"dcr_game_api.hpp\"\n" + s[i+j+1:]
			}
		}
	}
	if !strings.Contains(s, "#include <cstdlib>") {
		s = replaceFirst(s, "#include <chrono>\n", "#include <cstdlib>\n#include <chrono>\n")
	}
	re := regexp.MustCompile(`\s*load_embedded_elf_image\(runtime\);\s*\n\s*register_(?:recompiled_program|generated_[A-Za-z0-9_]+)\(runtime\);`)
	s = re.ReplaceAllStringFunc(s, func(m string) string {
		return "\n        if (!dcr_game_attach_v1(&runtime)) throw std::runtime_error(\"DCR guest module ABI attach failed\");"
	})
	attach := `        if (!dcr_game_attach_v1(&runtime)) throw std::runtime_error("DCR guest module ABI attach failed");`
	if !strings.Contains(s, "dc_plugins_load_default(runtime);") {
		s = replaceFirst(s, attach, attach+"\n        dc_plugins_load_default(runtime);")
	}
	if !strings.Contains(s, "DCRPluginShutdownGuard") {
		guard := "struct DCRPluginShutdownGuard {\n    dcrecomp_generated::DCRuntime* runtime{};\n    ~DCRPluginShutdownGuard() { if (runtime) dcrecomp_generated::dc_plugins_shutdown(*runtime); }\n};\n\n"
		s = replaceFirst(s, "int main(int argc, char** argv) {", guard+"int main(int argc, char** argv) {")
		s = replaceFirst(s, "    DCRuntime runtime;", "    DCRuntime runtime;\n    DCRPluginShutdownGuard dcr_plugin_guard{&runtime};")
	}
	if !strings.Contains(s, "DCR_SYSTEM_DATA_DIR") {
		setup := `    if (const char* dcr_system_dir = std::getenv("DCR_SYSTEM_DATA_DIR"); dcr_system_dir && *dcr_system_dir) {
        std::error_code dcr_save_ec;
        const std::filesystem::path dcr_save_root(dcr_system_dir);
        std::filesystem::create_directories(dcr_save_root, dcr_save_ec);
        runtime.bios_flash_path = (dcr_save_root / "dreamcast_flash.bin").string();
        runtime.maple_vmu_path = (dcr_save_root / "dreamcast_vmu_a1.bin").string();
        std::cout << "[DCR DATA] system=" << dcr_save_root.string() << "\n";
    }
`
		s = replaceFirst(s, "    DCRPluginShutdownGuard dcr_plugin_guard{&runtime};\n", "    DCRPluginShutdownGuard dcr_plugin_guard{&runtime};\n"+setup)
	}
	for _, banner := range []string{
		"cpp33c PLANETWEB_DUAL_ISP", "cpp34a MODULAR_RUNTIME_ABI1", "cpp34b MODULAR_FULL_ISP_ABI1",
		"cpp34c FLASH_BASE0_FIX_ABI1", "cpp34d IPBIN_REENTRY_SHADOW_GUARD_ABI1", "cpp34e BOOTSAFE_IPBIN_REENTRY_ABI1",
		"cpp34f ALIAS_SAFE_IPBIN_REENTRY_ABI1", "cpp34g PRESERVE_RELOC_IPBIN_ABI1", "cpp34h BOOTROM_RO_FLYCAST_MODEM_TRACE_ABI1",
		"cpp34i FLYCAST_MODEM_FULLSTACK_ABI1", "cpp34i1 FLYCAST_MODEM_FULLSTACK_MSVC_FIX_ABI1",
	} {
		s = strings.ReplaceAll(s, banner, "cpp34k STACKSAFE_IPBIN_REENTRY_ABI1")
	}
	for _, suffix := range []string{"cpp33c-network", "cpp34c-modular-full-isp", "cpp34d-ipbin-reentry-shadow-guard", "cpp34e-bootsafe-ipbin-reentry", "cpp34f-alias-safe-ipbin-reentry", "cpp34g-preserve-reloc-ipbin", "cpp34h-bootrom-ro-flycast-modem-trace", "cpp34i-flycast-modem-fullstack"} {
		s = strings.ReplaceAll(s, "v1.2-"+suffix, "v1.2-cpp34k-stacksafe-ipbin-reentry")
	}
	return writeText(p, s)
}
func patchCompileTest(cpp string) error {
	p := filepath.Join(cpp, "generated_compile_test.cpp")
	if !exists(p) {
		return nil
	}
	s, e := readText(p)
	if e != nil {
		return e
	}
	if !strings.Contains(s, `#include "dcr_game_api.hpp"`) {
		s = "#include \"dcr_game_api.hpp\"\n" + s
	}
	re := regexp.MustCompile(`\s*dcrecomp_generated::load_embedded_elf_image\(runtime\);\s*\n\s*dcrecomp_generated::register_(?:recompiled_program|generated_[A-Za-z0-9_]+)\(runtime\);`)
	s = re.ReplaceAllString(s, "\n    if (!dcrecomp_generated::dcr_game_attach_v1(&runtime)) return 90;")
	return writeText(p, s)
}
func modularize(cpp, runtime string) error {
	st, e := os.Stat(cpp)
	if e != nil || !st.IsDir() {
		return fmt.Errorf("[MODULAR] generated directory missing: %s", cpp)
	}
	guest := filepath.Join(runtime, "guest_abi", "v1")
	for _, f := range []string{"dc_runtime.hpp", "dc_arm7.hpp", "dc_native_overrides.hpp"} {
		src := filepath.Join(guest, f)
		if !exists(src) {
			return fmt.Errorf("[MODULAR] frozen Guest ABI missing: %s", src)
		}
		if e := copyFile(src, filepath.Join(cpp, f)); e != nil {
			return e
		}
	}
	if e := copyFile(filepath.Join(runtime, "include", "dcr_plugin_api.hpp"), filepath.Join(cpp, "dcr_plugin_api.hpp")); e != nil {
		return e
	}
	reg, e := discoverRegistration(cpp)
	if e != nil {
		return e
	}
	sources, e := discoverSources(cpp)
	if e != nil {
		return e
	}
	if e := writeText(filepath.Join(cpp, "dcr_game_api.hpp"), tpl("guest_api.hpp.txt")); e != nil {
		return e
	}
	src := strings.ReplaceAll(tpl("guest_exports.cpp.txt"), "__DCR_REGISTER__", reg)
	if e := writeText(filepath.Join(cpp, "dcr_game_exports.cpp"), src); e != nil {
		return e
	}
	if e := patchRunner(cpp); e != nil {
		return e
	}
	if e := patchCompileTest(cpp); e != nil {
		return e
	}
	list := []string{}
	for _, f := range sources {
		list = append(list, "    "+f)
	}
	cm := strings.ReplaceAll(tpl("modular_cmake.txt"), "    __DCR_GUEST_SOURCES__", strings.Join(list, "\n"))
	if e := writeText(filepath.Join(cpp, "CMakeLists.txt"), cm); e != nil {
		return e
	}
	if e := writeText(filepath.Join(cpp, ".dcr_guest_abi"), "DCR_GUEST_ABI=1\nDCR_GUEST_HEADER=FROZEN_V1\n"); e != nil {
		return e
	}
	fmt.Printf("[MODULAR] native guest ABI v1 ready | registration=%s | sources=%d\n", reg, len(sources))
	return nil
}
func metricfree(cpp string) error {
	header, cmake := filepath.Join(cpp, "dc_runtime.hpp"), filepath.Join(cpp, "CMakeLists.txt")
	if !exists(header) || !exists(cmake) {
		return fmt.Errorf("[cpp38b2] generated source or CMake missing: %s", cpp)
	}
	s, e := readText(header)
	if e != nil {
		return e
	}
	old, newMacro := tpl("old_fpu_macro.txt"), tpl("new_fpu_macro.txt")
	if strings.Contains(s, old) {
		s = replaceFirst(s, old, newMacro)
	} else if !strings.Contains(s, "DCR_FPU_METRIC_INC") {
		return fmt.Errorf("[cpp38b2] header macro layout unknown")
	}
	if e = writeText(header, s); e != nil {
		return e
	}
	s, e = readText(cmake)
	if e != nil {
		return e
	}
	needle := "target_compile_definitions(dcr_game_module PRIVATE DCR_GAME_MODULE_BUILD=1 DCR_GUEST_ABI_VERSION=1)"
	if !strings.Contains(s, "DCR_FIXED_FPU_REGION_METRICS=0") {
		if !strings.Contains(s, needle) {
			return fmt.Errorf("[cpp38b2] modular guest target not found")
		}
		s = replaceFirst(s, needle, needle+"\ntarget_compile_definitions(dcr_game_module PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)")
		if e = writeText(cmake, s); e != nil {
			return e
		}
	}
	paths := []string{}
	e = filepath.WalkDir(cpp, func(path string, d os.DirEntry, er error) error {
		if er != nil {
			return er
		}
		if !d.IsDir() && filepath.Ext(path) == ".cpp" {
			paths = append(paths, path)
		}
		return nil
	})
	if e != nil {
		return e
	}
	fields := strings.Fields(tpl("fpu_fields.txt"))
	localFields := strings.Fields(tpl("fpu_local_fields.txt"))
	reMode := regexp.MustCompile(`if \(runtime\.fpu_aot_mode != 3u \|\| runtime\.fpu_region_metrics\)`)
	transformed, changes := 0, 0
	for _, path := range paths {
		content, e := readText(path)
		if e != nil {
			return e
		}
		original := content
		content = reMode.ReplaceAllString(content, `if (DCR_FPU_REGION_METRICS_ENABLED(runtime))`)
		for _, field := range fields {
			content = strings.ReplaceAll(content, "++runtime."+field+";", "DCR_FPU_METRIC_INC(runtime."+field+");")
			re := regexp.MustCompile(`runtime\.` + regexp.QuoteMeta(field) + `\s*\+=\s*([^;\n]+);`)
			content = re.ReplaceAllStringFunc(content, func(m string) string {
				capture := re.FindStringSubmatch(m)
				return "DCR_FPU_METRIC_ADD(runtime." + field + ", " + strings.TrimSpace(capture[1]) + ");"
			})
		}
		for _, field := range localFields {
			content = strings.ReplaceAll(content, "++"+field+";", "DCR_FPU_METRIC_INC("+field+");")
			re := regexp.MustCompile(`\b` + regexp.QuoteMeta(field) + `\s*\+=\s*([^;\n]+);`)
			content = re.ReplaceAllStringFunc(content, func(m string) string {
				capture := re.FindStringSubmatch(m)
				return "DCR_FPU_METRIC_ADD(" + field + ", " + strings.TrimSpace(capture[1]) + ");"
			})
		}
		if content != original {
			if e = writeText(path, content); e != nil {
				return e
			}
			transformed++
			changes++
		}
	}
	if e = writeText(filepath.Join(cpp, ".dcr_guest_impl_source"), "DCR_GUEST_IMPL=cpp38b2-metricfree-v1\n"); e != nil {
		return e
	}
	fmt.Printf("[cpp38b2] native metric-free ready | files=%d changed=%d\n", transformed, changes)
	return nil
}
func hotRecovery(cpp string) error {
	cmake := filepath.Join(cpp, "CMakeLists.txt")
	s, e := readText(cmake)
	if e != nil {
		return e
	}
	if !strings.Contains(s, "# BEGIN DCR_CPP38E_HOT_BLOCK_RECOVERY") {
		found := false
		for _, anchor := range []string{
			"target_compile_definitions(dcr_game_module PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)\n",
			"target_compile_definitions(dcr_game_module PRIVATE DCR_GAME_MODULE_BUILD=1 DCR_GUEST_ABI_VERSION=1)\n",
		} {
			if strings.Contains(s, anchor) {
				s = replaceFirst(s, anchor, anchor+tpl("hot_cpp38e.txt"))
				found = true
				break
			}
		}
		if !found {
			return fmt.Errorf("[cpp38e] missing guest CMake target")
		}
		if e = writeText(cmake, s); e != nil {
			return e
		}
	}
	total := 0
	for _, path := range globSorted(filepath.Join(cpp, "generated_program*.cpp")) {
		source, e := readText(path)
		if e != nil {
			return e
		}
		next := strings.ReplaceAll(source, "++runtime.dynamic_direct_hint_hits;", "DCR_HOT_METRIC_INC(runtime.dynamic_direct_hint_hits);")
		if next != source {
			if e = writeText(path, next); e != nil {
				return e
			}
			total++
		}
	}
	marker := "DCR_CPP38E_HOT_BLOCK_RECOVERY=1\nHOT_PC_1=0x8C0FE5A0\nHOT_PC_2=0x8C1088DA\nMMU_ASSUMPTION=OFF_VALIDATED\nTICK_BATCH=256\n"
	if e = writeText(filepath.Join(cpp, ".dcr_cpp38e_hot_block_recovery"), marker); e != nil {
		return e
	}
	fmt.Printf("[cpp38e] native hot recovery ready | files=%d\n", total)
	return nil
}
