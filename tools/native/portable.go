package main

import (
	"archive/zip"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

func copyRequired(src, dst string) error {
	if !exists(src) {
		return fmt.Errorf("missing required portable input: %s", src)
	}
	if e := os.MkdirAll(filepath.Dir(dst), 0755); e != nil {
		return e
	}
	return copyFile(src, dst)
}
func zipDirectory(root, dest string) error {
	f, e := os.Create(dest)
	if e != nil {
		return e
	}
	zw := zip.NewWriter(f)
	var files []string
	e = filepath.WalkDir(root, func(p string, d os.DirEntry, er error) error {
		if er != nil {
			return er
		}
		if !d.IsDir() {
			files = append(files, p)
		}
		return nil
	})
	if e != nil {
		zw.Close()
		f.Close()
		return e
	}
	sort.Strings(files)
	for _, p := range files {
		rel, _ := filepath.Rel(root, p)
		entry, e := zw.Create(filepath.ToSlash(rel))
		if e != nil {
			zw.Close()
			f.Close()
			return e
		}
		in, e := os.Open(p)
		if e != nil {
			zw.Close()
			f.Close()
			return e
		}
		_, e = io.Copy(entry, in)
		in.Close()
		if e != nil {
			zw.Close()
			f.Close()
			return e
		}
	}
	if e = zw.Close(); e != nil {
		f.Close()
		return e
	}
	return f.Close()
}
func portableSync(args []string) error {
	fs := flag.NewFlagSet("portable-sync", flag.ContinueOnError)
	root := fs.String("root", "", "")
	game := fs.String("game-dir", "", "")
	build := fs.String("build-dir", "", "")
	slug := fs.String("slug", "", "")
	title := fs.String("title", "", "")
	abi := fs.String("abi", "1", "")
	runtimeID := fs.String("runtime-id", "", "")
	if e := fs.Parse(args); e != nil {
		return e
	}
	if *root == "" || *game == "" || *build == "" || *slug == "" || *runtimeID == "" {
		return errors.New("portable-sync needs --root --game-dir --build-dir --slug --runtime-id")
	}
	final, staging := filepath.Join(*game, "portable"), filepath.Join(*game, "portable.__sync__")
	os.RemoveAll(staging)
	if e := os.MkdirAll(filepath.Join(staging, "bin", "plugins"), 0755); e != nil {
		return e
	}
	defer os.RemoveAll(staging)
	var pairs [][2]string
	pairs = append(pairs, [2]string{filepath.Join(*root, "portable", "DreamcastRecompiled.exe"), filepath.Join(staging, "DreamcastRecompiled.exe")})
	for _, name := range []string{"dreamcast_program.exe", "DreamcastRuntime.dll", "dcr_game_module.dll"} {
		pairs = append(pairs, [2]string{filepath.Join(*build, "Release", name), filepath.Join(staging, "bin", name)})
	}
	for _, name := range []string{"dcr_network.dll", "dcr_maple.dll", "dcr_gdrom.dll"} {
		pairs = append(pairs, [2]string{filepath.Join(*build, "plugins", "Release", name), filepath.Join(staging, "bin", "plugins", name)})
	}
	for _, name := range []string{"dc_disc_probe.exe", "dc_boot_prepare.exe"} {
		pairs = append(pairs, [2]string{filepath.Join(*root, "build", "Release", name), filepath.Join(staging, "bin", name)})
	}
	for _, p := range pairs {
		if e := copyRequired(p[0], p[1]); e != nil {
			return e
		}
	}
	native := filepath.Join(*game, "incremental", "native")
	patches, _ := filepath.Glob(filepath.Join(native, "dcr_sh4_patch_*.dll"))
	sort.Strings(patches)
	for _, p := range patches {
		sub := filepath.Join(staging, "bin", "incremental", "native")
		if e := copyRequired(p, filepath.Join(sub, filepath.Base(p))); e != nil {
			return e
		}
		meta := strings.TrimSuffix(p, ".dll") + ".json"
		if exists(meta) {
			if e := copyRequired(meta, filepath.Join(sub, filepath.Base(meta))); e != nil {
				return e
			}
		}
	}
	data := fmt.Sprintf("DCR_PORTABLE_GUEST=1\nGUEST_ABI=%s\nRUNTIME=%s\nGAME=%s\nTITLE=%s\n", *abi, *runtimeID, *slug, *title)
	if e := writeFile(filepath.Join(staging, "portable_manifest.txt"), []byte(data)); e != nil {
		return e
	}
	if e := writeFile(filepath.Join(staging, "README_TESTER.txt"), []byte("DreamcastRecompiled portable auto-sync\r\n\r\nOpen DreamcastRecompiled.exe and select your original CDI/GDI. The portable does not include retail game images.\r\n")); e != nil {
		return e
	}
	forbidden := map[string]bool{"ip.bin": true, "boot.disc.bin": true, "bootstrap.bin": true, "boot.bin": true}
	e := filepath.WalkDir(staging, func(p string, d os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			return nil
		}
		if forbidden[strings.ToLower(d.Name())] || strings.EqualFold(filepath.Ext(p), ".cdi") || strings.EqualFold(filepath.Ext(p), ".gdi") {
			return fmt.Errorf("retail file in portable: %s", p)
		}
		return nil
	})
	if e != nil {
		return e
	}
	if e := os.RemoveAll(final); e != nil {
		return e
	}
	if e := os.Rename(staging, final); e != nil {
		return e
	}
	dest := filepath.Join(*game, *slug+"_portable_latest.zip")
	if e := zipDirectory(final, dest); e != nil {
		return e
	}
	fmt.Printf("[PORTABLE] READY: %s\n[PORTABLE] ZIP: %s\n", final, dest)
	return nil
}
