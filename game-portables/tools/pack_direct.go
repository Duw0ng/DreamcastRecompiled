// Build a minimal, game-specific DreamcastRecompiled portable from a verified
// source release. Go standard library only. File ordering is intentional.
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
type File struct { Source string `json:"source"`; Target string `json:"target"` }
type Spec struct {
 SchemaVersion int `json:"schema_version"`
 GameID string `json:"game_id"`
 GameTitle string `json:"game_title"`
 SourceReleaseTag string `json:"source_release_tag"`
 SourceAsset string `json:"source_asset"`
 SourceSHA256 string `json:"source_sha256"`
 TargetTag string `json:"target_tag"`
 TargetAsset string `json:"target_asset"`
 OutputSHA256 string `json:"output_sha256"`
 Files []File `json:"files"`
}
func digest(name string) (string,error) {
 f,e:=os.Open(name);if e!=nil{return "",e};defer f.Close()
 h:=sha256.New();if _,e=io.Copy(h,f);e!=nil{return "",e}
 return hex.EncodeToString(h.Sum(nil)),nil
}
func safe(s string)bool{
 return s!=""&&!strings.HasPrefix(s,"/")&&!strings.Contains(s,"\")&&!strings.Contains(s,":")&&path.Clean(s)==s&&!strings.HasPrefix(s,"../")&&s!=".."
}
func build(specFile,src,dstDir string)error{
 raw,e:=os.ReadFile(specFile);if e!=nil{return e}
 var s Spec;if e=json.Unmarshal(raw,&s);e!=nil{return e}
 if s.SchemaVersion!=1||s.GameID==""||len(s.Files)==0{return errors.New("bad portable spec")}
 if !safe(s.SourceAsset)||!safe(s.TargetAsset)||path.Base(s.SourceAsset)!=s.SourceAsset||path.Base(s.TargetAsset)!=s.TargetAsset{return errors.New("unsafe asset names")}
 if len(s.SourceSHA256)!=64||len(s.OutputSHA256)!=64{return errors.New("both SHA-256 digests must be pinned")}
 hash,e:=digest(src);if e!=nil{return e}
 if !strings.EqualFold(hash,s.SourceSHA256){return fmt.Errorf("source SHA-256 mismatch: %s",hash)}
 source,e:=zip.OpenReader(src);if e!=nil{return e};defer source.Close()
 members:=map[string]*zip.File{}
 for _,f:=range source.File{
  if !safe(f.Name){return fmt.Errorf("unsafe source member: %s",f.Name)}
  if _,ok:=members[f.Name];ok{return fmt.Errorf("duplicate source member: %s",f.Name)}
  members[f.Name]=f
 }
 mandatory:=map[string]bool{"DreamcastRecompiled.exe":false,"bin/DreamcastRuntime.dll":false,"bin/dcr_game_module.dll":false,"portable_manifest.txt":false}
 taken:=map[string]bool{};correctGame:=false
 for _,m:=range s.Files{
  if !safe(m.Source)||!safe(m.Target)||taken[m.Target]{return fmt.Errorf("unsafe/duplicate mapping: %+v",m)}
  taken[m.Target]=true
  ext:=strings.ToLower(path.Ext(m.Target))
  switch ext{case ".cdi",".gdi",".iso",".chd",".cue",".nrg":return fmt.Errorf("game disc image prohibited: %s",m.Target)}
  f,ok:=members[m.Source];if !ok||f.FileInfo().IsDir(){return fmt.Errorf("missing file: %s",m.Source)}
  if _,ok=mandatory[m.Target];ok{mandatory[m.Target]=true}
  if m.Target=="portable_manifest.txt"{
   r,err:=f.Open();if err!=nil{return err}
   data,err:=io.ReadAll(r);r.Close();if err!=nil{return err}
   for _,ln:=range strings.Split(strings.ReplaceAll(string(data),"
","
"),"
"){
    if ln=="GAME="+s.GameID{correctGame=true}
   }
  }
 }
 for name,ok:=range mandatory{if !ok{return fmt.Errorf("required file missing: %s",name)}}
 if !correctGame{return errors.New("game identity mismatch")}
 if e=os.MkdirAll(dstDir,0755);e!=nil{return e}
 dest:=filepath.Join(dstDir,s.TargetAsset)
 fd,e:=os.Create(dest);if e!=nil{return e}
 writer:=zip.NewWriter(fd)
 for _,m:=range s.Files{
  reader,err:=members[m.Source].Open()
  if err!=nil{writer.Close();fd.Close();return err}
  header:=&zip.FileHeader{Name:m.Target,Method:zip.Deflate}
  to,err:=writer.CreateHeader(header)
  if err==nil{_,err=io.Copy(to,reader)}
  reader.Close()
  if err!=nil{writer.Close();fd.Close();return err}
 }
 if e=writer.Close();e!=nil{fd.Close();return e}
 if e=fd.Close();e!=nil{return e}
 outputHash,e:=digest(dest);if e!=nil{return e}
 if !strings.EqualFold(outputHash,s.OutputSHA256){return fmt.Errorf("EXACT output SHA-256 mismatch: %s",outputHash)}
 check,e:=zip.OpenReader(dest);if e!=nil{return e};defer check.Close()
 for _,f:=range check.File{
  r,err:=f.Open();if err!=nil{return err}
  _,err=io.Copy(io.Discard,r);r.Close();if err!=nil{return err}
 }
 if e=os.WriteFile(dest+".sha256",[]byte(outputHash+"  "+s.TargetAsset+"
"),0644);e!=nil{return e}
 fmt.Printf("EXACT PORTABLE VERIFIED: %s | %d files | sha256 %s
",dest,len(s.Files),outputHash)
 return nil
}
func main(){
 spec:=flag.String("spec","","Portable release manifest")
 src:=flag.String("source","","Pinned source release ZIP")
 dst:=flag.String("out","dist","Output directory")
 flag.Parse()
 if *spec==""||*src==""{fmt.Fprintln(os.Stderr,"-spec and -source required");os.Exit(2)}
 if e:=build(*spec,*src,*dst);e!=nil{fmt.Fprintln(os.Stderr,"ERROR:",e);os.Exit(1)}
}
