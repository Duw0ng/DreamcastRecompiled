package main

import (
	"bufio"
	"crypto/sha256"
	"encoding/csv"
	"encoding/hex"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"hash/fnv"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"strings"
)

func fileHash(p string) (string, error) {
	f, e := os.Open(p)
	if e != nil {
		return "", e
	}
	defer f.Close()
	h := sha256.New()
	if _, e = io.Copy(h, f); e != nil {
		return "", e
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}
func checksum(b []byte) string            { h := sha256.Sum256(b); return hex.EncodeToString(h[:]) }
func failf(format string, a ...any) error { return fmt.Errorf(format, a...) }
func exists(p string) bool                { s, e := os.Stat(p); return e == nil && !s.IsDir() }
func abiBody(file string) (string, error) {
	bs, e := os.ReadFile(file)
	if e != nil {
		return "", e
	}
	t := string(bs)
	start := strings.Index(t, "struct DCRuntime {")
	if start < 0 {
		return "", errors.New("struct DCRuntime { not found")
	}
	depth, end := 0, -1
	for i := start; i < len(t); i++ {
		switch t[i] {
		case '{':
			depth++
		case '}':
			depth--
			if depth == 0 {
				end = i + 1
			}
		}
		if end >= 0 {
			break
		}
	}
	if end < 0 {
		return "", errors.New("struct DCRuntime incomplete")
	}
	body := t[start:end]
	// Equivalent to original Python comment elimination + join whitespace.
	block := regexp.MustCompile(`(?s)/\*.*?\*/`)
	body = block.ReplaceAllString(body, "")
	line := regexp.MustCompile(`//[^\n]*`)
	body = line.ReplaceAllString(body, "")
	body = strings.Join(strings.Fields(body), "")
	return checksum([]byte(body)), nil
}
func abi(args []string) error {
	if len(args) < 1 || len(args) > 2 {
		return errors.New("usage: abi FILE [REFERENCE]")
	}
	a, e := abiBody(args[0])
	if e != nil {
		return e
	}
	if len(args) == 1 {
		fmt.Println(a)
		return nil
	}
	b, e := abiBody(args[1])
	if e != nil {
		return e
	}
	fmt.Printf("[ABI FINGERPRINT] cached=%s\n[ABI FINGERPRINT] frozen=%s\n", a, b)
	if a != b {
		fmt.Println("[ABI FINGERPRINT] MISMATCH")
		return errors.New("ABI mismatch")
	}
	fmt.Println("[ABI FINGERPRINT] MATCH")
	return nil
}
func canonPath(s string) string {
	s = strings.TrimSpace(s)
	a, _ := filepath.Abs(s)
	if runtime.GOOS == "windows" {
		return strings.ToLower(strings.ReplaceAll(filepath.Clean(a), "/", `\`))
	}
	return filepath.Clean(a)
}
func cmakeHome(args []string) error {
	if len(args) != 2 {
		return errors.New("usage: cmake-home CACHE SOURCE")
	}
	b, e := os.ReadFile(args[0])
	if e != nil {
		return e
	}
	src := ""
	for _, line := range strings.Split(string(b), "\n") {
		if strings.HasPrefix(line, "CMAKE_HOME_DIRECTORY:INTERNAL=") {
			src = strings.TrimSpace(strings.TrimPrefix(line, "CMAKE_HOME_DIRECTORY:INTERNAL="))
			break
		}
	}
	if src != "" && canonPath(src) == canonPath(args[1]) {
		return nil
	}
	return failf("[RUNTIME CACHE] source=%s CMake source=%s", args[1], src)
}

var requiredDLLs = []string{"Release/DreamcastRuntime.dll", "plugins/Release/dcr_network.dll", "plugins/Release/dcr_maple.dll", "plugins/Release/dcr_gdrom.dll"}
var sourceExt = map[string]bool{".cpp": true, ".c": true, ".hpp": true, ".h": true, ".hlsl": true, ".cmake": true}

func sourceHash(dir string) (string, error) {
	var paths []string
	e := filepath.WalkDir(dir, func(p string, d os.DirEntry, er error) error {
		if er != nil {
			return er
		}
		if d.IsDir() {
			return nil
		}
		if sourceExt[strings.ToLower(filepath.Ext(p))] || d.Name() == "CMakeLists.txt" {
			paths = append(paths, p)
		}
		return nil
	})
	if e != nil {
		return "", e
	}
	if len(paths) == 0 || !exists(filepath.Join(dir, "dc_runtime.cpp")) {
		return "", errors.New("runtime source incomplete")
	}
	sort.Strings(paths)
	h := sha256.New()
	for _, p := range paths {
		rel, e := filepath.Rel(dir, p)
		if e != nil {
			return "", e
		}
		sha, e := fileHash(p)
		if e != nil {
			return "", e
		}
		io.WriteString(h, filepath.ToSlash(rel))
		h.Write([]byte{0})
		io.WriteString(h, sha)
		h.Write([]byte{'\n'})
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

// Field names and JSON encoding match the original Python manifest exactly.
type manifest struct {
	Format int               `json:"format"`
	Source string            `json:"source_sha256"`
	Binary map[string]string `json:"binary_sha256"`
}

func runtimeVerify(args []string) error {
	if len(args) != 4 {
		return errors.New("usage: runtime-verify check|write SOURCE BUILD MANIFEST")
	}
	mode, src, build, file := args[0], args[1], args[2], args[3]
	if mode != "check" && mode != "write" {
		return errors.New("mode must be check or write")
	}
	sh, e := sourceHash(src)
	if e != nil {
		return e
	}
	got := manifest{Format: 1, Source: sh, Binary: map[string]string{}}
	for _, r := range requiredDLLs {
		got.Binary[r], e = fileHash(filepath.Join(build, filepath.FromSlash(r)))
		if e != nil {
			return failf("missing/invalid runtime DLL %s: %w", r, e)
		}
	}
	if mode == "write" {
		if e = os.MkdirAll(filepath.Dir(file), 0755); e != nil {
			return e
		}
		b, _ := json.MarshalIndent(got, "", "  ")
		b = append(b, '\n')
		temp := file + ".tmp"
		if e = os.WriteFile(temp, b, 0644); e != nil {
			return e
		}
		if e = os.Rename(temp, file); e != nil {
			return e
		}
		fmt.Println("[RUNTIME VERIFY] Manifest written, source=" + sh[:16])
		return nil
	}
	b, e := os.ReadFile(file)
	if e != nil {
		return errors.New("runtime manifest missing; rebuild runtime only")
	}
	var prior manifest
	if e = json.Unmarshal(b, &prior); e != nil {
		return e
	}
	if got.Format != prior.Format || got.Source != prior.Source || len(got.Binary) != len(prior.Binary) {
		return errors.New("runtime source or DLL checksum mismatch")
	}
	for k, v := range got.Binary {
		if prior.Binary[k] != v {
			return errors.New("runtime source or DLL checksum mismatch")
		}
	}
	fmt.Println("[RUNTIME VERIFY] runtime and four DLLs match manifest")
	return nil
}

func execCommand(cwd string, command ...string) error {
	fmt.Println("[NATIVE SEEDS]", strings.Join(command, " "))
	cmd := exec.Command(command[0], command[1:]...)
	cmd.Dir = cwd
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}
func copyFile(a, b string) error {
	in, e := os.Open(a)
	if e != nil {
		return e
	}
	defer in.Close()
	out, e := os.OpenFile(b, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, 0644)
	if e != nil {
		return e
	}
	_, e = io.Copy(out, in)
	c := out.Close()
	if e != nil {
		return e
	}
	return c
}
func writeFile(p string, b []byte) error {
	if e := os.MkdirAll(filepath.Dir(p), 0755); e != nil {
		return e
	}
	return os.WriteFile(p, b, 0644)
}
func knownBase(path string) map[uint64]bool {
	found := map[uint64]bool{}
	f, e := os.Open(path)
	if e != nil {
		return found
	}
	defer f.Close()
	cr := csv.NewReader(f)
	rows, e := cr.ReadAll()
	if e != nil || len(rows) == 0 {
		return found
	}
	col := -1
	for i, k := range rows[0] {
		if strings.TrimPrefix(k, "\ufeff") == "address" {
			col = i
		}
	}
	if col < 0 {
		return found
	}
	for _, r := range rows[1:] {
		if col < len(r) {
			v, e := strconv.ParseUint(strings.TrimSpace(r[col]), 0, 32)
			if e == nil {
				found[v] = true
			}
		}
	}
	return found
}

var seedRegexp = regexp.MustCompile(`^\s*(0x[0-9a-fA-F]{8})(?:\s|$)`)

func readSeeds(path string, base, size uint64) []uint64 {
	f, e := os.Open(path)
	if e != nil {
		return nil
	}
	defer f.Close()
	got := map[uint64]bool{}
	sc := bufio.NewScanner(f)
	for sc.Scan() {
		line := strings.TrimPrefix(sc.Text(), "\ufeff")
		match := seedRegexp.FindStringSubmatch(line)
		if len(match) < 2 {
			continue
		}
		n, e := strconv.ParseUint(match[1], 0, 32)
		if e != nil || n&1 != 0 || n < base || n-base+32 > size {
			continue
		}
		got[n] = true
	}
	var out []uint64
	for v := range got {
		out = append(out, v)
	}
	sort.Slice(out, func(i, j int) bool { return out[i] < out[j] })
	return out
}
func cmakeSource(id, lib, out string) string {
	return fmt.Sprintf(`cmake_minimum_required(VERSION 3.20)
project(dcr_sh4_patch LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(dcr_sh4_patch_%s SHARED generated_program.cpp incremental_exports.cpp)
target_include_directories(dcr_sh4_patch_%s PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(dcr_sh4_patch_%s PRIVATE "%s")
target_compile_definitions(dcr_sh4_patch_%s PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)
set_target_properties(dcr_sh4_patch_%s PROPERTIES PREFIX "" INTERPROCEDURAL_OPTIMIZATION FALSE
 RUNTIME_OUTPUT_DIRECTORY "%s" RUNTIME_OUTPUT_DIRECTORY_RELEASE "%s"
 LIBRARY_OUTPUT_DIRECTORY "%s" LIBRARY_OUTPUT_DIRECTORY_RELEASE "%s")
if(MSVC)
 target_compile_options(dcr_sh4_patch_%s PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>)
else()
 target_compile_options(dcr_sh4_patch_%s PRIVATE $<$<CONFIG:Release>:-O2> -fPIC)
endif()
`, id, id, id, filepath.ToSlash(lib), id, id, filepath.ToSlash(out), filepath.ToSlash(out), filepath.ToSlash(out), filepath.ToSlash(out), id, id)
}
func exportsSource(hash string) string {
	return fmt.Sprintf(`#include "generated_program.hpp"
#include <cstdint>
#ifdef _WIN32
#define DCR_PATCH_EXPORT extern "C" __declspec(dllexport)
#else
#define DCR_PATCH_EXPORT extern "C" __attribute__((visibility("default")))
#endif
DCR_PATCH_EXPORT std::uint32_t dcr_sh4_patch_abi_v1() { return 0x00010000u; }
DCR_PATCH_EXPORT const char* dcr_sh4_patch_boot_sha256_v1() { return "%s"; }
DCR_PATCH_EXPORT bool dcr_sh4_patch_register_v1(dcrecomp_generated::DCRuntime* runtime) {
 if (!runtime) return false;
 dcrecomp_generated::register_recompiled_program(*runtime);
 return true;
}
`, hash)
}

// Exact 4-KiB snapshots are captured by the interpreter ONLY for even RAM
// targets. The memory is rechecked at load and every dynamic native dispatch.
type dynamicSnapshot struct {
	entry uint64
	base  uint64
	bytes []byte
	path  string
}

var snapshotName = regexp.MustCompile(`^snapshot_([0-9A-F]{8})_([0-9A-F]{8})_([0-9A-F]{16})\.bin$`)

func exportsDynamicSource(bootHash string, snapshot dynamicSnapshot) string {
	var out strings.Builder
	out.WriteString(exportsSource(bootHash))
	out.WriteString("\nstatic const unsigned char kDcrSnapshot[] = {\n")
	for i, v := range snapshot.bytes {
		fmt.Fprintf(&out, "%d,", v)
		if i%32 == 31 {
			out.WriteString("\n")
		}
	}
	fmt.Fprintf(&out, `};
DCR_PATCH_EXPORT bool dcr_sh4_patch_dynamic_v1(std::uint32_t* base,std::uint32_t* len,const std::uint8_t** bytes) {
    if (!base || !len || !bytes) return false;
    *base=0x%08Xu; *len=%du; *bytes=kDcrSnapshot; return true;
}
DCR_PATCH_EXPORT std::uint32_t dcr_sh4_patch_dynamic_entry_v1() { return 0x%08Xu; }
`, snapshot.base, len(snapshot.bytes), snapshot.entry)
	return out.String()
}
func dynamicSeeds(root, game, bootHash, patchRoot, native string, dryRun bool) error {
	files, err := os.ReadDir(filepath.Join(patchRoot, "dynamic"))
	if os.IsNotExist(err) {
		fmt.Println("[DCR SH4-DYNAMIC] no RAM snapshots yet")
		return nil
	}
	if err != nil {
		return err
	}
	known := knownBase(filepath.Join(game, "generated", "function_map.csv"))
	// A compiler/ABI revision must invalidate old quarantine and native DLLs.
	emitterHash, err := fileHash(filepath.Join(root, "src", "codegen", "cpp_emitter.cpp"))
	if err != nil {
		return err
	}
	generatorHash, err := fileHash(filepath.Join(root, "src", "recompiler", "incremental.cpp"))
	if err != nil {
		return err
	}
	abiHash, err := fileHash(filepath.Join(root, "runtime", "guest_abi", "v1", "dc_runtime.hpp"))
	if err != nil {
		return err
	}
	candidates := 0
	for _, file := range files {
		if file.IsDir() {
			continue
		}
		matches := snapshotName.FindStringSubmatch(file.Name())
		if len(matches) != 4 {
			continue
		}
		entry, err1 := strconv.ParseUint("0x"+matches[1], 0, 32)
		base, err2 := strconv.ParseUint("0x"+matches[2], 0, 32)
		checksumName, err3 := strconv.ParseUint("0x"+matches[3], 0, 64)
		if err1 != nil || err2 != nil || err3 != nil || (entry&1) != 0 || entry < base || entry-base+32 > 4096 || known[entry] {
			continue
		}
		source := filepath.Join(patchRoot, "dynamic", file.Name())
		bytes, err := os.ReadFile(source)
		if err != nil || len(bytes) != 4096 || entry&0x1FFFFFFF < 0x0C000000 || entry&0x1FFFFFFF >= 0x10000000 || base&0xFFF != 0 {
			continue
		}
		h := fnv.New64a()
		_, _ = h.Write(bytes)
		if h.Sum64() != checksumName {
			fmt.Fprintln(os.Stderr, "[DCR SH4-DYNAMIC] invalid snapshot hash", file.Name())
			continue
		}
		snapshot := dynamicSnapshot{entry: entry, base: base, bytes: bytes, path: source}
		id := checksum([]byte("dynamic-ram-patch-v1|" + bootHash + "|" + file.Name() + "|" + checksum(bytes) + "|" + emitterHash + "|" + generatorHash + "|" + abiHash))[:16]
		dll := filepath.Join(native, "dcr_sh4_patch_"+id+".dll")
		if exists(dll) {
			continue
		}
		candidates++
		if dryRun {
			fmt.Printf("[DCR SH4-DYNAMIC] pending entry=0x%08X snapshot=%s\n", entry, file.Name())
			continue
		}
		failed := filepath.Join(patchRoot, "failed", id+".json")
		if exists(failed) {
			fmt.Printf("[DCR SH4-DYNAMIC] quarantined 0x%08X\n", entry)
			continue
		}
		// We use the normal incremental generator, but its input is a verified
		// immutable snapshot, rather than stale BOOTSTRAP.BIN code.
		err = patchBatch(root, game, source, bootHash, base, patchRoot, native, id, []uint64{entry}, &snapshot)
		if err != nil {
			fmt.Fprintln(os.Stderr, "[DCR SH4-DYNAMIC] WARN", file.Name(), err)
			_ = os.MkdirAll(filepath.Dir(failed), 0755)
			reason, _ := json.MarshalIndent(map[string]any{"schema": "dynamic-ram-patch-v1", "entry": fmt.Sprintf("0x%08X", entry), "snapshot": file.Name(), "reason": err.Error()}, "", "  ")
			_ = writeFile(failed, append(reason, '\n'))
			continue
		}
		fmt.Printf("[DCR SH4-DYNAMIC] published entry=0x%08X patch=dcr_sh4_patch_%s.dll\n", entry, id)
	}
	fmt.Printf("[DCR SH4-DYNAMIC] candidate snapshots=%d (AOT base untouched)\n", candidates)
	return nil
}

func patchBatch(root, game, image, hash string, base uint64, patchRoot, native, id string, batch []uint64, dynamic *dynamicSnapshot) error {
	compiler := filepath.Join(root, "build", "Release", "dc_incremental_recomp.exe")
	if !exists(compiler) {
		if e := execCommand(root, "cmake", "-S", root, "-B", filepath.Join(root, "build"), "-A", "x64"); e != nil {
			return e
		}
	}
	if e := execCommand(root, "cmake", "--build", filepath.Join(root, "build"), "--config", "Release", "--target", "dc_incremental_recomp", "--parallel", "1"); e != nil {
		return e
	}
	lib := filepath.Join(game, "build", "Release", "DreamcastRuntime.lib")
	if !exists(lib) {
		return failf("runtime import lib absent: %s", lib)
	}
	staging := filepath.Join(patchRoot, "staging", id)
	src, bld, out := filepath.Join(staging, "src"), filepath.Join(staging, "build"), filepath.Join(staging, "bin")
	for _, d := range []string{src, bld, out} {
		if e := os.MkdirAll(d, 0755); e != nil {
			return e
		}
	}
	cmd := []string{compiler, "--image=" + image, fmt.Sprintf("--base=0x%08X", base), "--output=" + src}
	for _, s := range batch {
		cmd = append(cmd, fmt.Sprintf("--seed=0x%08X", s))
	}
	if e := execCommand(root, cmd...); e != nil {
		return e
	}
	for _, name := range []string{"dc_runtime.hpp", "dc_arm7.hpp", "dc_native_overrides.hpp"} {
		if e := copyFile(filepath.Join(root, "runtime", "guest_abi", "v1", name), filepath.Join(src, name)); e != nil {
			return e
		}
	}
	code := exportsSource(hash)
	if dynamic != nil {
		code = exportsDynamicSource(hash, *dynamic)
	}
	if e := writeFile(filepath.Join(src, "incremental_exports.cpp"), []byte(code)); e != nil {
		return e
	}
	if e := writeFile(filepath.Join(src, "CMakeLists.txt"), []byte(cmakeSource(id, lib, out))); e != nil {
		return e
	}
	if e := execCommand(root, "cmake", "-S", src, "-B", bld, "-A", "x64"); e != nil {
		return e
	}
	if e := execCommand(root, "cmake", "--build", bld, "--config", "Release", "--parallel", "1"); e != nil {
		return e
	}
	dll := "dcr_sh4_patch_" + id + ".dll"
	from := filepath.Join(out, dll)
	if !exists(from) {
		return errors.New("patch DLL not produced")
	}
	temp, dest := filepath.Join(native, dll+".new"), filepath.Join(native, dll)
	if e := copyFile(from, temp); e != nil {
		return e
	}
	if e := os.Rename(temp, dest); e != nil {
		return e
	}
	sha, e := fileHash(dest)
	if e != nil {
		return e
	}
	formatted := []string{}
	for _, s := range batch {
		formatted = append(formatted, fmt.Sprintf("0x%08X", s))
	}
	meta := map[string]any{"schema": "native-seed-patch-v1", "boot_sha256": hash, "seeds": formatted, "module_sha256": sha, "tool": "dc_incremental_recomp"}
	if dynamic != nil {
		meta["schema"] = "dynamic-ram-patch-v1"
		meta["snapshot_sha256"] = checksum(dynamic.bytes)
		meta["snapshot_base"] = fmt.Sprintf("0x%08X", dynamic.base)
		meta["snapshot_entry"] = fmt.Sprintf("0x%08X", dynamic.entry)
	}
	b, _ := json.MarshalIndent(meta, "", "  ")
	return writeFile(filepath.Join(native, "dcr_sh4_patch_"+id+".json"), append(b, '\n'))
}
func nativeSeeds(args []string) error {
	fs := flag.NewFlagSet("native-seeds", flag.ContinueOnError)
	root := fs.String("root", "", "source root")
	game := fs.String("game-dir", "", "game cache directory")
	image := fs.String("image", "", "BOOT image")
	baseRaw := fs.String("base", "0x8C010000", "SH4 base")
	maxBatch := fs.Int("max-per-batch", 8, "max batch")
	dryRun := fs.Bool("dry-run", false, "dry run")
	dynamicMode := fs.Bool("dynamic", false, "compile captured runtime RAM snapshots")
	// Allow flags --base=VALUE, --root VALUE in any order.
	if e := fs.Parse(args); e != nil {
		return e
	}
	if *root == "" || *game == "" || *image == "" {
		return errors.New("--root, --game-dir, --image required")
	}
	base, e := strconv.ParseUint(*baseRaw, 0, 32)
	if e != nil {
		return e
	}
	rootAbs, e := filepath.Abs(*root)
	if e != nil {
		return e
	}
	gameAbs, e := filepath.Abs(*game)
	if e != nil {
		return e
	}
	imageAbs, e := filepath.Abs(*image)
	if e != nil {
		return e
	}
	stat, e := os.Stat(imageAbs)
	if e != nil {
		return e
	}
	hash, e := fileHash(imageAbs)
	if e != nil {
		return e
	}
	patchRoot := filepath.Join(gameAbs, "incremental")
	native := filepath.Join(patchRoot, "native")
	if e = os.MkdirAll(native, 0755); e != nil {
		return e
	}
	if e = writeFile(filepath.Join(patchRoot, "current_boot_sha256.txt"), []byte(hash+"\n")); e != nil {
		return e
	}
	seeds := readSeeds(filepath.Join(gameAbs, "interp_seeds.txt"), base, uint64(stat.Size()))
	known := knownBase(filepath.Join(gameAbs, "generated", "function_map.csv"))
	existing := map[uint64]bool{}
	metas, _ := filepath.Glob(filepath.Join(native, "dcr_sh4_patch_*.json"))
	for _, p := range metas {
		b, e := os.ReadFile(p)
		if e != nil {
			continue
		}
		var v struct {
			Schema string   `json:"schema"`
			Boot   string   `json:"boot_sha256"`
			Seeds  []string `json:"seeds"`
		}
		if json.Unmarshal(b, &v) != nil || v.Schema != "native-seed-patch-v1" || v.Boot != hash || !exists(strings.TrimSuffix(p, ".json")+".dll") {
			continue
		}
		for _, s := range v.Seeds {
			a, e := strconv.ParseUint(s, 0, 32)
			if e == nil {
				existing[a] = true
			}
		}
	}
	var pending []uint64
	for _, s := range seeds {
		if !known[s] && !existing[s] {
			pending = append(pending, s)
		}
	}
	fmt.Printf("[NATIVE SEEDS] recorded=%d base=%d native-cache=%d pending=%d\n", len(seeds), len(known), len(existing), len(pending))
	if *dryRun || len(pending) == 0 {
		if *dynamicMode {
			return dynamicSeeds(rootAbs, gameAbs, hash, patchRoot, native, *dryRun)
		}
		return nil
	}
	if *maxBatch < 1 {
		*maxBatch = 1
	}
	if *maxBatch > 16 {
		*maxBatch = 16
	}
	type batchItem []uint64
	var queue []batchItem
	for i := 0; i < len(pending); i += *maxBatch {
		j := i + *maxBatch
		if j > len(pending) {
			j = len(pending)
		}
		queue = append(queue, pending[i:j])
	}
	// Fingerprint must match Python hashing exactly so successful patches are reused.
	a, e := fileHash(filepath.Join(rootAbs, "src", "codegen", "cpp_emitter.cpp"))
	if e != nil {
		return e
	}
	b, e := fileHash(filepath.Join(rootAbs, "src", "recompiler", "incremental.cpp"))
	if e != nil {
		return e
	}
	c, e := fileHash(filepath.Join(rootAbs, "runtime", "guest_abi", "v1", "dc_runtime.hpp"))
	if e != nil {
		return e
	}
	for len(queue) > 0 {
		current := queue[0]
		queue = queue[1:]
		stringsDec := []string{}
		for _, s := range current {
			stringsDec = append(stringsDec, strconv.FormatUint(s, 10))
		}
		id := checksum([]byte("native-seed-patch-v1|" + hash + "|" + strings.Join(stringsDec, ",") + "|" + a + "|" + b + "|" + c))[:16]
		failed := filepath.Join(patchRoot, "failed", id+".json")
		if len(current) == 1 && exists(failed) {
			fmt.Printf("[NATIVE SEEDS] quarantined 0x%08X\n", current[0])
			continue
		}
		e := patchBatch(rootAbs, gameAbs, imageAbs, hash, base, patchRoot, native, id, current, nil)
		if e == nil {
			fmt.Printf("[NATIVE SEEDS] published %s (%d seeds)\n", id, len(current))
			continue
		}
		fmt.Fprintln(os.Stderr, "[NATIVE SEEDS] WARN:", e)
		if len(current) > 1 {
			mid := len(current) / 2
			queue = append([]batchItem{current[:mid], current[mid:]}, queue...)
			continue
		}
		meta := map[string]any{"schema": "native-seed-patch-v1", "seed": fmt.Sprintf("0x%08X", current[0]), "boot_sha256": hash, "reason": e.Error(), "retry": "after emitter/ABI change"}
		data, _ := json.MarshalIndent(meta, "", "  ")
		if er := writeFile(failed, append(data, '\n')); er != nil {
			return er
		}
		fmt.Printf("[NATIVE SEEDS] quarantined 0x%08X, interpreter remains active\n", current[0])
	}
	if *dynamicMode {
		return dynamicSeeds(rootAbs, gameAbs, hash, patchRoot, native, *dryRun)
	}
	return nil
}
func run(args []string) error {
	if len(args) == 0 {
		return errors.New("usage: dcr_native_tools sha256|abi|cmake-home|runtime-verify|native-seeds|portable-sync|modularize|metricfree|hot-recovery|adopt-safe")
	}
	switch args[0] {
	case "sha256":
		if len(args) != 2 {
			return errors.New("usage: sha256 FILE")
		}
		s, e := fileHash(args[1])
		if e == nil {
			fmt.Println(s)
		}
		return e
	case "abi":
		return abi(args[1:])
	case "cmake-home":
		return cmakeHome(args[1:])
	case "runtime-verify":
		return runtimeVerify(args[1:])
	case "native-seeds":
		return nativeSeeds(args[1:])
	case "portable-sync":
		return portableSync(args[1:])
	case "modularize":
		if len(args) != 4 || args[2] != "--runtime-root" {
			return errors.New("usage: modularize CPP_DIR --runtime-root RUNTIME_DIR")
		}
		return modularize(args[1], args[3])
	case "metricfree":
		if len(args) != 2 {
			return errors.New("usage: metricfree CPP_DIR")
		}
		return metricfree(args[1])
	case "hot-recovery":
		if len(args) != 2 {
			return errors.New("usage: hot-recovery CPP_DIR")
		}
		return hotRecovery(args[1])
	default:
		return failf("unknown operation %q", args[0])
	}
}
func main() {
	if e := run(os.Args[1:]); e != nil {
		fmt.Fprintln(os.Stderr, "[DCR NATIVE]", e)
		os.Exit(1)
	}
}
