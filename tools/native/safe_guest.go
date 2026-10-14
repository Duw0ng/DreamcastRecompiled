package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

func normalizeManifestValue(s string) string {
	return strings.Join(strings.Fields(strings.ToUpper(strings.TrimSpace(s))), " ")
}
func iniManifest(path string) (map[string]string, error) {
	f, e := os.Open(path)
	if e != nil {
		return nil, e
	}
	defer f.Close()
	out := map[string]string{}
	scan := bufio.NewScanner(f)
	for scan.Scan() {
		line := strings.TrimSpace(strings.TrimPrefix(scan.Text(), "\ufeff"))
		if line == "" || strings.HasPrefix(line, "#") || strings.HasPrefix(line, ";") {
			continue
		}
		key, val, ok := strings.Cut(line, "=")
		if ok {
			out[strings.TrimSpace(key)] = strings.TrimSpace(val)
		}
	}
	return out, scan.Err()
}
func adoptSafeGuest(args []string) error {
	fs := flag.NewFlagSet("adopt-safe", flag.ContinueOnError)
	manifest := fs.String("manifest", "", "")
	seed := fs.String("seed", "", "")
	module := fs.String("module", "", "")
	marker := fs.String("marker", "", "")
	legacy := fs.String("legacy-marker", "", "")
	slug := fs.String("slug", "", "")
	title := fs.String("title", "", "")
	volume := fs.String("volume", "", "")
	abi := fs.String("abi", "", "")
	if e := fs.Parse(args); e != nil {
		return e
	}
	if *manifest == "" || *seed == "" || *module == "" || *marker == "" || *legacy == "" || *slug == "" || *abi == "" {
		return fmt.Errorf("adopt-safe requires manifest, seed, module, marker, legacy-marker, slug, abi")
	}
	m, e := iniManifest(*manifest)
	if e != nil {
		return e
	}
	expected := strings.ToLower(m["MODULE_SHA256"])
	var bad []string
	if m["DCR_PORTABLE_SAFE"] != "1" {
		bad = append(bad, "portable-safe flag")
	}
	if m["DCR_GUEST_ABI"] != *abi {
		bad = append(bad, "guest ABI")
	}
	if normalizeManifestValue(m["GAME_SLUG"]) != normalizeManifestValue(*slug) {
		bad = append(bad, "game slug")
	}
	if m["TITLE"] != "" && normalizeManifestValue(m["TITLE"]) != normalizeManifestValue(*title) {
		bad = append(bad, "title")
	}
	if m["VOLUME"] != "" && normalizeManifestValue(m["VOLUME"]) != normalizeManifestValue(*volume) {
		bad = append(bad, "volume")
	}
	if len(expected) != 64 {
		bad = append(bad, "manifest SHA256")
	}
	if !exists(*seed) {
		bad = append(bad, "seed file")
	}
	if len(bad) > 0 {
		return fmt.Errorf("[SAFE GUEST] seed inapplicable: %s", strings.Join(bad, ", "))
	}
	actual, e := fileHash(*seed)
	if e != nil {
		return e
	}
	if actual != expected {
		return fmt.Errorf("[SAFE GUEST] seed checksum mismatch: %s", actual)
	}
	prior := ""
	if exists(*module) {
		prior, _ = fileHash(*module)
	}
	if prior != expected {
		temporary := *module + ".new"
		if e = os.MkdirAll(filepath.Dir(*module), 0755); e != nil {
			return e
		}
		if e = copyFile(*seed, temporary); e != nil {
			return e
		}
		defer os.Remove(temporary)
		got, er := fileHash(temporary)
		if er != nil {
			return er
		}
		if got != expected {
			return fmt.Errorf("[SAFE GUEST] adopted module failed SHA256 verification")
		}
		// Replace only after SHA verification; do not touch a valid old DLL on failure.
		if e = os.Rename(temporary, *module); e != nil {
			return e
		}
		fmt.Println("[SAFE GUEST] adopted existing safe module without guest recompilation")
	} else {
		fmt.Println("[SAFE GUEST] current native module matches the safe seed")
	}
	source := m["SOURCE"]
	if source == "" {
		source = "SAFE_SEED"
	}
	value := fmt.Sprintf("DCR_PORTABLE_GUEST=1\nDCR_GUEST_ABI=%s\nDCR_SAFE_SEED_SHA256=%s\nDCR_SAFE_SEED_SOURCE=%s\n", *abi, expected, source)
	if e = writeFile(*marker, []byte(value)); e != nil {
		return e
	}
	if e = writeFile(*legacy, []byte(value)); e != nil {
		return e
	}
	fmt.Println("[SAFE GUEST] PORTABLE SAFE = YES | SHA256=" + expected)
	return nil
}
