package main

import (
  "archive/zip"
  "crypto/sha256"
  "encoding/hex"
  "encoding/json"
  "errors"
  "flag"
  "fmt"
  "io"
  "os"
  "path"
  "path/filepath"
  "strings"
)

func fail(err error) { fmt.Fprintln(os.Stderr, "ERROR:", err); os.Exit(1) }
func sha(name string) (string, error) {
  f, e := os.Open(name); if e != nil { return "", e }; defer f.Close()
  h := sha256.New(); if _,e=io.Copy(h,f);e!=nil { return "", e }
  return hex.EncodeToString(h.Sum(nil)),nil
}
func safe(s string) bool { return s!="" && !strings.HasPrefix(s,"/") && !strings.Contains(s,"\\") && !strings.Contains(s,":") && path.Clean(s)==s }
func main() {
  manifest := flag.String("spec", "", "release JSON spec")
  source := flag.String("source", "", "source release ZIP")
  outdir := flag.String("out", "dist", "destination directory")
  flag.Parse()
  if *manifest=="" || *source=="" { fail(errors.New("missing -spec / -source")) }
  contents,e:=os.ReadFile(*manifest);if e!=nil {fail(e)}
  var data map[string]json.RawMessage
  if e=json.Unmarshal(contents,&data);e!=nil {fail(e)}
  str:=func(key string) string { var v string; if e:=json.Unmarshal(data[key],&v);e!=nil { fail(fmt.Errorf("missing or invalid %s: %w",key,e)) }; return v }
  var schema int; if e=json.Unmarshal(data["schema_version"],&schema);e!=nil || schema!=1 {fail(errors.New("unknown spec schema"))}
  id:=str("game_id");originalSHA:=str("source_sha256");expectedSHA:=str("output_sha256")
  asset:=str("target_asset")
  if !safe(id) || !safe(asset) || path.Base(asset)!=asset || len(originalSHA)!=64 || len(expectedSHA)!=64 {fail(errors.New("invalid game / ZIP name / checksum"))}
  var files []map[string]string
  if e=json.Unmarshal(data["files"],&files);e!=nil || len(files)<4 {fail(errors.New("invalid file mapping list"))}
  actual,e:=sha(*source);if e!=nil{fail(e)}
  if !strings.EqualFold(actual,originalSHA){fail(fmt.Errorf("source SHA-256 mismatch: %s",actual))}
  input,e:=zip.OpenReader(*source);if e!=nil{fail(e)};defer input.Close()
  members:=map[string]*zip.File{}
  for _,f:=range input.File {
    if !safe(f.Name){fail(fmt.Errorf("unsafe source ZIP member: %q",f.Name))}
    if _,found:=members[f.Name];found{fail(fmt.Errorf("duplicate source member: %s",f.Name))}
    members[f.Name]=f
  }
  needed:=map[string]bool{"DreamcastRecompiled.exe":false,"bin/DreamcastRuntime.dll":false,"bin/dcr_game_module.dll":false,"portable_manifest.txt":false}
  used:=map[string]bool{}
  validID:=false
  for _,m:=range files {
    src,dst:=m["source"],m["target"]
    if !safe(src)||!safe(dst)||used[dst]{fail(fmt.Errorf("unsafe/duplicate file mapping: %s",dst))}
    used[dst]=true
    switch strings.ToLower(path.Ext(dst)) {case ".cdi",".gdi",".iso",".chd",".cue",".nrg":fail(errors.New("no game discs may be packaged"))}
    f,found:=members[src];if !found || f.FileInfo().IsDir(){fail(fmt.Errorf("source member missing: %s",src))}
    if _,ok:=needed[dst];ok{needed[dst]=true}
    if dst=="portable_manifest.txt"{
      r,err:=f.Open();if err!=nil{fail(err)}
      v,err:=io.ReadAll(r);r.Close();if err!=nil{fail(err)}
      for _,line:=range strings.Split(strings.ReplaceAll(string(v),"\r\n","\n"),"\n"){
        if line=="GAME="+id{validID=true}
      }
    }
  }
  for filename,found:=range needed{if !found{fail(fmt.Errorf("required file missing: %s",filename))}}
  if !validID{fail(errors.New("guest ID does not match manifest"))}
  if e=os.MkdirAll(*outdir,0755);e!=nil{fail(e)}
  target:=filepath.Join(*outdir,asset)
  output,e:=os.Create(target);if e!=nil{fail(e)}
  archive:=zip.NewWriter(output)
  for _,m:=range files {
    sourceFile,err:=members[m["source"]].Open();if err!=nil{fail(err)}
    destFile,err:=archive.CreateHeader(&zip.FileHeader{Name:m["target"],Method:zip.Deflate})
    if err==nil{_,err=io.Copy(destFile,sourceFile)}
    sourceFile.Close();if err!=nil{fail(err)}
  }
  if e=archive.Close();e!=nil{fail(e)}
  if e=output.Close();e!=nil{fail(e)}
  digest,e:=sha(target);if e!=nil{fail(e)}
  if !strings.EqualFold(digest,expectedSHA){fail(fmt.Errorf("exact ZIP checksum mismatch: %s",digest))}
  check,e:=zip.OpenReader(target);if e!=nil{fail(e)}
  for _,f:=range check.File {r,err:=f.Open();if err!=nil{fail(err)};_,err=io.Copy(io.Discard,r);r.Close();if err!=nil{fail(err)}}
  check.Close()
  if e=os.WriteFile(target+".sha256",[]byte(digest+"  "+asset+"\n"),0644);e!=nil{fail(e)}
  fmt.Printf("VERIFIED EXACT portable %s (%d files, sha256 %s)\n",target,len(files),digest)
}
