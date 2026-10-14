package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestAbiFingerprintMatchesFrozen(t *testing.T) {
	f := filepath.Join("..", "..", "runtime", "guest_abi", "v1", "dc_runtime.hpp")
	h, e := abiBody(f)
	if e != nil {
		t.Fatal(e)
	}
	if h != "00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f" {
		t.Fatalf("unexpected ABI digest %s", h)
	}
}
func TestSeedsSkipDynamicAndUnaligned(t *testing.T) {
	p := filepath.Join(t.TempDir(), "seeds.txt")
	_ = os.WriteFile(p, []byte("# runtime-code: 0x8C010400\n0x8C010010\n0x8C010011\n0x8C010020  # useful\n0x8C010010\n0x8C0104F0\n"), 0644)
	got := readSeeds(p, 0x8C010000, 256)
	if len(got) != 2 || got[0] != 0x8C010010 || got[1] != 0x8C010020 {
		t.Fatalf("bad seeds: %v", got)
	}
}
func TestRuntimeManifestReuse(t *testing.T) {
	root := t.TempDir()
	src := filepath.Join(root, "runtime")
	build := filepath.Join(root, "build")
	_ = os.MkdirAll(src, 0755)
	_ = os.WriteFile(filepath.Join(src, "dc_runtime.cpp"), []byte("int main(){}"), 0644)
	for _, dll := range requiredDLLs {
		p := filepath.Join(build, filepath.FromSlash(dll))
		_ = os.MkdirAll(filepath.Dir(p), 0755)
		_ = os.WriteFile(p, []byte("test"), 0644)
	}
	out := filepath.Join(root, "manifest.json")
	if e := runtimeVerify([]string{"write", src, build, out}); e != nil {
		t.Fatal(e)
	}
	if e := runtimeVerify([]string{"check", src, build, out}); e != nil {
		t.Fatal(e)
	}
	b, e := os.ReadFile(out)
	if e != nil {
		t.Fatal(e)
	}
	var decoded map[string]any
	if json.Unmarshal(b, &decoded) != nil || decoded["format"] != float64(1) {
		t.Fatal("invalid format")
	}
	_ = os.WriteFile(filepath.Join(src, "dc_runtime.cpp"), []byte("int updated(){}"), 0644)
	if e := runtimeVerify([]string{"check", src, build, out}); e == nil {
		t.Fatal("manifest should reject changed source")
	}
}
func TestFingerprintAndCmakeHome(t *testing.T) {
	p := filepath.Join(t.TempDir(), "CMakeCache.txt")
	src := filepath.Dir(p)
	_ = os.WriteFile(p, []byte("CMAKE_HOME_DIRECTORY:INTERNAL="+src+"\n"), 0644)
	if e := cmakeHome([]string{p, src}); e != nil {
		t.Fatal(e)
	}
	if e := cmakeHome([]string{p, src + "x"}); e == nil {
		t.Fatal("mismatch should fail")
	}
	if !strings.Contains(cmakeSource("abc", "C:/lib/A.lib", "C:/tmp/out"), "dcr_sh4_patch_abc") {
		t.Fatal("invalid generated CMake")
	}
}

func TestDynamicSnapshotExports(t *testing.T) {
	data := make([]byte, 4096)
	for i := range data {
		data[i] = byte(i & 255)
	}
	snapshot := dynamicSnapshot{entry: 0x8CFD2000, base: 0x8CFD2000, bytes: data}
	source := exportsDynamicSource(strings.Repeat("a", 64), snapshot)
	for _, part := range []string{"dcr_sh4_patch_dynamic_v1", "dcr_sh4_patch_dynamic_entry_v1", "0x8CFD2000u", "*len=4096u"} {
		if !strings.Contains(source, part) {
			t.Fatalf("missing %s", part)
		}
	}
	if strings.Contains(source, `\nstatic const`) {
		t.Fatal("escaped line break in C++ source")
	}
}
