param(
  [string]$ProfilePath = (Join-Path (Split-Path $PSScriptRoot -Parent | Split-Path -Parent) 'profiles\controller_profile.ini')
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$interop = @"
using System;
using System.Runtime.InteropServices;
public static class DCRPadNative {
  [StructLayout(LayoutKind.Sequential)] public struct JOYINFOEX {
    public uint dwSize, dwFlags, dwXpos, dwYpos, dwZpos, dwRpos, dwUpos, dwVpos, dwButtons, dwButtonNumber, dwPOV, dwReserved1, dwReserved2;
  }
  [StructLayout(LayoutKind.Sequential)] public struct XINPUT_GAMEPAD {
    public ushort wButtons; public byte bLeftTrigger, bRightTrigger; public short sThumbLX, sThumbLY, sThumbRX, sThumbRY;
  }
  [StructLayout(LayoutKind.Sequential)] public struct XINPUT_STATE { public uint dwPacketNumber; public XINPUT_GAMEPAD Gamepad; }
  [DllImport("winmm.dll")] public static extern uint joyGetNumDevs();
  [DllImport("winmm.dll")] public static extern uint joyGetPosEx(uint uJoyID, ref JOYINFOEX pji);
  [DllImport("xinput1_4.dll", EntryPoint="XInputGetState")] public static extern uint XInputGetState14(uint idx, out XINPUT_STATE state);
  [DllImport("xinput1_3.dll", EntryPoint="XInputGetState")] public static extern uint XInputGetState13(uint idx, out XINPUT_STATE state);
  public static uint XInputGetState(uint idx, out XINPUT_STATE state) {
    try { return XInputGetState14(idx, out state); } catch (DllNotFoundException) { try { return XInputGetState13(idx, out state); } catch { state = new XINPUT_STATE(); return 1167; } }
    catch { state = new XINPUT_STATE(); return 1167; }
  }
}
"@
try { Add-Type -TypeDefinition $interop -Language CSharp -ErrorAction Stop } catch { }

$dcControls = @('A','B','X','Y','START','UP','DOWN','LEFT','RIGHT','C','Z','JOY_X','JOY_Y','JOY2_X','JOY2_Y','LTRIG','RTRIG')
$defaultAuto = [ordered]@{
 A='logical:a'; B='logical:b'; X='logical:x'; Y='logical:y'; START='logical:start';
 UP='logical:up'; DOWN='logical:down'; LEFT='logical:left'; RIGHT='logical:right'; C='logical:c'; Z='logical:z';
 JOY_X='logical:joy_x'; JOY_Y='logical:joy_y'; JOY2_X='logical:joy2_x'; JOY2_Y='logical:joy2_y'; LTRIG='logical:ltrig'; RTRIG='logical:rtrig'
}
$defaultXInput = [ordered]@{
 A='button:0'; B='button:1'; X='button:2'; Y='button:3'; START='button:7';
 UP='button:10'; DOWN='button:11'; LEFT='button:12'; RIGHT='button:13'; C='button:4'; Z='button:5';
 JOY_X='axis:LX'; JOY_Y='axis:LY:invert'; JOY2_X='axis:RX'; JOY2_Y='axis:RY:invert'; LTRIG='trigger:LT'; RTRIG='trigger:RT'
}
$defaultPS4 = [ordered]@{
 A='logical:a'; B='logical:b'; X='logical:x'; Y='logical:y'; START='logical:start';
 UP='logical:up'; DOWN='logical:down'; LEFT='logical:left'; RIGHT='logical:right'; C='logical:c'; Z='logical:z';
 JOY_X='logical:joy_x'; JOY_Y='logical:joy_y'; JOY2_X='logical:joy2_x'; JOY2_Y='logical:joy2_y'; LTRIG='logical:ltrig'; RTRIG='logical:rtrig'
}

function Get-XInputState([int]$Index) {
  $s = New-Object DCRPadNative+XINPUT_STATE
  $rc = [DCRPadNative]::XInputGetState([uint32]$Index, [ref]$s)
  if ($rc -ne 0) { return $null }
  return [pscustomobject]@{ Backend='xinput'; Device=$Index; Buttons=[uint32]$s.Gamepad.wButtons; LX=[int]$s.Gamepad.sThumbLX; LY=[int]$s.Gamepad.sThumbLY; RX=[int]$s.Gamepad.sThumbRX; RY=[int]$s.Gamepad.sThumbRY; LT=[int]$s.Gamepad.bLeftTrigger; RT=[int]$s.Gamepad.bRightTrigger; POV=0xFFFF }
}
function Get-WinMMState([int]$Index) {
  $j = New-Object DCRPadNative+JOYINFOEX
  $j.dwSize = [Runtime.InteropServices.Marshal]::SizeOf([type][DCRPadNative+JOYINFOEX]); $j.dwFlags = 255
  if ([DCRPadNative]::joyGetPosEx([uint32]$Index, [ref]$j) -ne 0) { return $null }
  return [pscustomobject]@{ Backend='winmm'; Device=$Index; Buttons=[uint32]$j.dwButtons; X=[int]$j.dwXpos-32767; Y=[int]$j.dwYpos-32767; Z=[int]$j.dwZpos-32767; R=[int]$j.dwRpos-32767; U=[int]$j.dwUpos-32767; V=[int]$j.dwVpos-32767; LT=0; RT=0; POV=[uint32]$j.dwPOV }
}
function Get-WinMMStateAny([int]$Preferred) {
  $w=Get-WinMMState $Preferred; if($w){return $w}
  $count=[Math]::Min(16,[int][DCRPadNative]::joyGetNumDevs())
  for($i=0;$i -lt $count;$i++){if($i -eq $Preferred){continue};$w=Get-WinMMState $i;if($w){return $w}}
  return $null
}
function Get-PadState { param([string]$Backend,[int]$Device)
  if ($Backend -eq 'xinput') { return Get-XInputState $Device }
  if ($Backend -eq 'winmm' -or $Backend -eq 'ps4') { return Get-WinMMStateAny $Device }
  $x=Get-XInputState $Device; if ($x) { return $x }; return Get-WinMMStateAny $Device
}
function Get-XInputPressed([uint32]$b) {
  $masks=@(0x1000,0x2000,0x4000,0x8000,0x0100,0x0200,0x0020,0x0010,0x0040,0x0080,0x0001,0x0002,0x0004,0x0008)
  $r=@(); for($i=0;$i -lt $masks.Count;$i++){if(($b -band $masks[$i]) -ne 0){$r+=$i}}; return $r
}
function Detect-Input($prev,$cur,[double]$dz) {
  if(-not $cur){return $null}; $threshold=[int](32767*$dz)
  if($cur.Backend -eq 'xinput'){
    $pb=if($prev){Get-XInputPressed $prev.Buttons}else{@()}; $cb=Get-XInputPressed $cur.Buttons
    foreach($n in $cb){if($pb -notcontains $n){return "button:$n"}}
    foreach($t in @('LT','RT')){if($cur.$t -gt 80 -and (-not $prev -or $prev.$t -le 80)){return "trigger:$t"}}
    foreach($a in @('LX','LY','RX','RY')){ $v=[int]$cur.$a; $old=if($prev){[int]$prev.$a}else{0}; if([Math]::Abs($v) -gt $threshold -and [Math]::Abs($old) -le $threshold){return "axis:$a" + $(if($v -ge 0){'+'}else{'-'})} }
  } else {
    for($i=0;$i -lt 32;$i++){ $mask=[uint64]((([int64]1) -shl $i)); if(([uint64]$cur.Buttons -band $mask) -and (-not $prev -or -not ([uint64]$prev.Buttons -band $mask))){return "button:$i"} }
    if($cur.POV -ne 0xFFFF -and (-not $prev -or $prev.POV -eq 0xFFFF)){ $p=$cur.POV%36000; if($p -ge 31500 -or $p -le 4500){return'pov:up'}elseif($p -le 13500){return'pov:right'}elseif($p -le 22500){return'pov:down'}else{return'pov:left'} }
    foreach($a in @('X','Y','Z','R','U','V')){ $v=[int]$cur.$a; $old=if($prev){[int]$prev.$a}else{0}; if([Math]::Abs($v) -gt $threshold -and [Math]::Abs($old) -le $threshold){return "axis:$a" + $(if($v -ge 0){'+'}else{'-'})} }
  }
  return $null
}

$form=New-Object Windows.Forms.Form; $form.Text='DreamcastRecomp - Configurador de Mando'; $form.Size=New-Object Drawing.Size(760,760); $form.StartPosition='CenterScreen'; $form.MinimumSize=New-Object Drawing.Size(700,650)
$font=New-Object Drawing.Font('Segoe UI',9)
$form.Font=$font

$top=New-Object Windows.Forms.Panel; $top.Dock='Top'; $top.Height=110; $form.Controls.Add($top)
$l1=New-Object Windows.Forms.Label; $l1.Text='Backend'; $l1.Location='12,14'; $l1.AutoSize=$true; $top.Controls.Add($l1)
$backend=New-Object Windows.Forms.ComboBox; $backend.DropDownStyle='DropDownList'; [void]$backend.Items.AddRange(@('auto','xinput','ps4','winmm')); $backend.SelectedIndex=0; $backend.Location='80,10'; $backend.Width=110; $top.Controls.Add($backend)
$l2=New-Object Windows.Forms.Label; $l2.Text='Dispositivo'; $l2.Location='215,14'; $l2.AutoSize=$true; $top.Controls.Add($l2)
$device=New-Object Windows.Forms.NumericUpDown; $device.Minimum=0;$device.Maximum=15;$device.Location='285,10';$device.Width=60;$top.Controls.Add($device)
$refresh=New-Object Windows.Forms.Button; $refresh.Text='Probar mando';$refresh.Location='370,8';$refresh.Size='110,28';$top.Controls.Add($refresh)
$status=New-Object Windows.Forms.Label;$status.Text='Sin probar';$status.Location='500,14';$status.Size='230,22';$top.Controls.Add($status)
$dzLabel=New-Object Windows.Forms.Label;$dzLabel.Text='Deadzone 18%';$dzLabel.Location='12,55';$dzLabel.Size='100,22';$top.Controls.Add($dzLabel)
$dead=New-Object Windows.Forms.TrackBar;$dead.Minimum=0;$dead.Maximum=50;$dead.Value=18;$dead.TickFrequency=5;$dead.Location='110,43';$dead.Width=250;$top.Controls.Add($dead)
$live=New-Object Windows.Forms.Label;$live.Text='Estado: -';$live.Location='380,52';$live.Size='340,45';$top.Controls.Add($live)

$grid=New-Object Windows.Forms.DataGridView; $grid.Dock='Fill';$grid.AllowUserToAddRows=$false;$grid.AllowUserToDeleteRows=$false;$grid.RowHeadersVisible=$false;$grid.AutoSizeColumnsMode='Fill';$grid.SelectionMode='FullRowSelect';$grid.MultiSelect=$false
[void]$grid.Columns.Add('dc','Dreamcast');[void]$grid.Columns.Add('src','Entrada física'); $cap=New-Object Windows.Forms.DataGridViewButtonColumn;$cap.Name='cap';$cap.HeaderText='';$cap.Text='Capturar';$cap.UseColumnTextForButtonValue=$true;$cap.FillWeight=35;[void]$grid.Columns.Add($cap)
foreach($c in $dcControls){$idx=$grid.Rows.Add();$grid.Rows[$idx].Cells['dc'].Value=$c;$grid.Rows[$idx].Cells['dc'].ReadOnly=$true;if($defaultAuto.Contains($c)){$grid.Rows[$idx].Cells['src'].Value=$defaultAuto[$c]}}
$form.Controls.Add($grid)

$bottom=New-Object Windows.Forms.Panel;$bottom.Dock='Bottom';$bottom.Height=92;$form.Controls.Add($bottom)
$load=New-Object Windows.Forms.Button;$load.Text='Cargar perfil';$load.Location='12,12';$load.Size='105,30';$bottom.Controls.Add($load)
$autoPreset=New-Object Windows.Forms.Button;$autoPreset.Text='Auto';$autoPreset.Location='125,12';$autoPreset.Size='75,30';$bottom.Controls.Add($autoPreset)
$ps4Preset=New-Object Windows.Forms.Button;$ps4Preset.Text='PS4 nativo';$ps4Preset.Location='208,12';$ps4Preset.Size='95,30';$bottom.Controls.Add($ps4Preset)
$xinputPreset=New-Object Windows.Forms.Button;$xinputPreset.Text='XInput';$xinputPreset.Location='311,12';$xinputPreset.Size='80,30';$bottom.Controls.Add($xinputPreset)
$save=New-Object Windows.Forms.Button;$save.Text='Guardar perfil';$save.Location='399,12';$save.Size='115,30';$bottom.Controls.Add($save)
$hint=New-Object Windows.Forms.Label;$hint.Text='Auto: Xbox/XInput primero, luego PS4/DirectInput. PS4 nativo no requiere DS4Windows.';$hint.Location='12,49';$hint.Size='510,34';$bottom.Controls.Add($hint)
$pathLabel=New-Object Windows.Forms.Label;$pathLabel.Text=$ProfilePath;$pathLabel.Location='530,9';$pathLabel.Size='200,70';$bottom.Controls.Add($pathLabel)

$captureRow=-1;$previous=$null
$dead.Add_ValueChanged({$dzLabel.Text="Deadzone $($dead.Value)%"})
$refresh.Add_Click({$s=Get-PadState $backend.Text ([int]$device.Value); if($s){$status.Text="Conectado: $($s.Backend) #$($s.Device)";$status.ForeColor=[Drawing.Color]::DarkGreen}else{$status.Text='No detectado';$status.ForeColor=[Drawing.Color]::DarkRed}})
$grid.Add_CellContentClick({param($sender,$e); if($e.RowIndex -ge 0 -and $grid.Columns[$e.ColumnIndex].Name -eq 'cap'){ $script:captureRow=$e.RowIndex;$script:previous=Get-PadState $backend.Text ([int]$device.Value);$status.Text="Capturando $($grid.Rows[$e.RowIndex].Cells['dc'].Value)..." }})
$autoPreset.Add_Click({$backend.SelectedItem='auto'; foreach($r in $grid.Rows){$k=[string]$r.Cells['dc'].Value;if($defaultAuto.Contains($k)){$r.Cells['src'].Value=$defaultAuto[$k]}}})
$ps4Preset.Add_Click({$backend.SelectedItem='ps4'; foreach($r in $grid.Rows){$k=[string]$r.Cells['dc'].Value;if($defaultPS4.Contains($k)){$r.Cells['src'].Value=$defaultPS4[$k]}}})
$xinputPreset.Add_Click({$backend.SelectedItem='xinput'; foreach($r in $grid.Rows){$k=[string]$r.Cells['dc'].Value;if($defaultXInput.Contains($k)){$r.Cells['src'].Value=$defaultXInput[$k]}}})
$save.Add_Click({
  $dir=Split-Path $ProfilePath -Parent;if($dir -and -not(Test-Path $dir)){New-Item -ItemType Directory -Force -Path $dir|Out-Null}
  $lines=@('[DreamcastRecompController]','version=2',"backend=$($backend.Text)","device=$([int]$device.Value)","deadzone=$([Math]::Round($dead.Value/100.0,2))")
  foreach($r in $grid.Rows){$k=([string]$r.Cells['dc'].Value).ToLower();$v=[string]$r.Cells['src'].Value;if($v){$lines+="$k=$v"}}
  Set-Content -Encoding ASCII -Path $ProfilePath -Value $lines; $status.Text='Perfil guardado';$pathLabel.Text=$ProfilePath
})
$load.Add_Click({if(Test-Path $ProfilePath){$kv=@{};foreach($line in Get-Content $ProfilePath){if($line -match '^\s*([^#;\[].*?)\s*=\s*(.*?)\s*$'){$kv[$matches[1].ToLower()]=$matches[2]}};if($kv.backend){$backend.SelectedItem=$kv.backend};if($kv.device){$device.Value=[int]$kv.device};if($kv.deadzone){$dead.Value=[Math]::Min(50,[Math]::Max(0,[int]([double]$kv.deadzone*100)))};foreach($r in $grid.Rows){$k=([string]$r.Cells['dc'].Value).ToLower();if($kv.ContainsKey($k)){$r.Cells['src'].Value=$kv[$k]}};$status.Text='Perfil cargado'}})

$timer=New-Object Windows.Forms.Timer;$timer.Interval=60;$timer.Add_Tick({
  $s=Get-PadState $backend.Text ([int]$device.Value);if($s){$live.Text="Estado: $($s.Backend) #$($s.Device) botones=0x$('{0:X8}' -f $s.Buttons) POV=$($s.POV)"}
  if($captureRow -ge 0 -and $s){$spec=Detect-Input $previous $s ($dead.Value/100.0);if($spec){
    $target=[string]$grid.Rows[$captureRow].Cells['dc'].Value
    if($target -match '^JOY' -and $spec -match '^axis:(.+)[+-]$'){$spec="axis:$($matches[1])";if($target -match '_Y$'){$spec+=':invert'}}
    $grid.Rows[$captureRow].Cells['src'].Value=$spec;$status.Text="Capturado $target = $spec";$script:captureRow=-1
  };$script:previous=$s}
});$timer.Start()
$form.Add_FormClosed({$timer.Stop()})
[void]$form.ShowDialog()
