param(
  [string]$ProfilePath = (Join-Path (Split-Path $PSScriptRoot -Parent | Split-Path -Parent) 'profiles\controller_profile.ini')
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$interop = @"
using System;
using System.Runtime.InteropServices;
public static class DCRControllerNative {
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
  [DllImport("xinput9_1_0.dll", EntryPoint="XInputGetState")] public static extern uint XInputGetState910(uint idx, out XINPUT_STATE state);
  public static uint XInputGetState(uint idx, out XINPUT_STATE state) {
    try { return XInputGetState14(idx, out state); }
    catch (DllNotFoundException) {
      try { return XInputGetState13(idx, out state); }
      catch (DllNotFoundException) {
        try { return XInputGetState910(idx, out state); }
        catch { state = new XINPUT_STATE(); return 1167; }
      }
      catch { state = new XINPUT_STATE(); return 1167; }
    }
    catch { state = new XINPUT_STATE(); return 1167; }
  }
}
"@
try { Add-Type -TypeDefinition $interop -Language CSharp -ErrorAction Stop } catch { }

function New-DefaultBindings {
  return [ordered]@{
    a='logical:a'; b='logical:b'; x='logical:x'; y='logical:y'; start='logical:start';
    up='logical:up'; down='logical:down'; left='logical:left'; right='logical:right'; c='logical:c'; z='logical:z';
    joy_x='logical:joy_x'; joy_y='logical:joy_y'; joy2_x='logical:joy2_x'; joy2_y='logical:joy2_y';
    ltrig='logical:ltrig'; rtrig='logical:rtrig'
  }
}

function Read-ControllerProfile([string]$Path) {
  $profile = [ordered]@{ backend='auto'; device=0; deadzone=0.18; bindings=(New-DefaultBindings) }
  if (-not (Test-Path $Path)) { return $profile }
  foreach ($line in Get-Content -LiteralPath $Path) {
    if ($line -notmatch '^\s*([^#;\[].*?)\s*=\s*(.*?)\s*$') { continue }
    $key=$matches[1].Trim().ToLowerInvariant(); $value=$matches[2].Trim()
    switch ($key) {
      'backend' { if (@('auto','xinput','winmm','ps4') -contains $value.ToLowerInvariant()) { $profile.backend=$value.ToLowerInvariant() } }
      'device' { try { $profile.device=[Math]::Max(0,[Math]::Min(15,[int]$value)) } catch {} }
      'deadzone' { try { $script:profile.deadzone=[Math]::Max(0.0,[Math]::Min(0.5,[double]$value)) } catch {} }
      default { if ($profile.bindings.Contains($key)) { $profile.bindings[$key]=$value } }
    }
  }
  return $profile
}

function Get-XInputState([int]$Index) {
  $s=New-Object DCRControllerNative+XINPUT_STATE
  if ([DCRControllerNative]::XInputGetState([uint32]$Index,[ref]$s) -ne 0) { return $null }
  return [pscustomobject]@{
    Backend='xinput'; Device=$Index; Buttons=[uint32]$s.Gamepad.wButtons; POV=[uint32]0xFFFF;
    LX=[int]$s.Gamepad.sThumbLX; LY=[int]$s.Gamepad.sThumbLY; RX=[int]$s.Gamepad.sThumbRX; RY=[int]$s.Gamepad.sThumbRY;
    X=0;Y=0;ZAxis=0;RAxis=0;U=0;V=0; LT=[int]$s.Gamepad.bLeftTrigger; RT=[int]$s.Gamepad.bRightTrigger
  }
}
function Get-WinMMState([int]$Index) {
  $j=New-Object DCRControllerNative+JOYINFOEX
  $j.dwSize=[Runtime.InteropServices.Marshal]::SizeOf([type][DCRControllerNative+JOYINFOEX]); $j.dwFlags=255
  if ([DCRControllerNative]::joyGetPosEx([uint32]$Index,[ref]$j) -ne 0) { return $null }
  return [pscustomobject]@{
    Backend='directinput/winmm'; Device=$Index; Buttons=[uint32]$j.dwButtons; POV=[uint32]$j.dwPOV;
    LX=0;LY=0;RX=0;RY=0;
    X=([int]$j.dwXpos-32767); Y=([int]$j.dwYpos-32767); ZAxis=([int]$j.dwZpos-32767); RAxis=([int]$j.dwRpos-32767);
    U=([int]$j.dwUpos-32767); V=([int]$j.dwVpos-32767); LT=0; RT=0
  }
}
function Get-WinMMStateAny([int]$Preferred) {
  $w=Get-WinMMState $Preferred; if($w){return $w}
  $count=[Math]::Min(16,[int][DCRControllerNative]::joyGetNumDevs())
  for($i=0;$i -lt $count;$i++){ if($i -eq $Preferred){continue}; $w=Get-WinMMState $i; if($w){return $w} }
  return $null
}
function Get-PadState([string]$Backend,[int]$Device) {
  if($Backend -eq 'xinput'){return Get-XInputState $Device}
  if($Backend -eq 'winmm' -or $Backend -eq 'ps4'){return Get-WinMMStateAny $Device}
  $x=Get-XInputState $Device; if($x){return $x}; return Get-WinMMStateAny $Device
}

$xMasks=@([uint32]0x1000,[uint32]0x2000,[uint32]0x4000,[uint32]0x8000,[uint32]0x0100,[uint32]0x0200,[uint32]0x0020,[uint32]0x0010,[uint32]0x0040,[uint32]0x0080,[uint32]0x0001,[uint32]0x0002,[uint32]0x0004,[uint32]0x0008)
function Test-PhysicalButton($State,[int]$Index) {
  if(-not $State -or $Index -lt 0 -or $Index -ge 32){return $false}
  if($State.Backend -eq 'xinput'){
    if($Index -ge $xMasks.Count){return $false}; return (($State.Buttons -band $xMasks[$Index]) -ne 0)
  }
  return (([uint64]$State.Buttons -band [uint64]((([int64]1) -shl $Index))) -ne 0)
}
function Test-Pov($State,[string]$Dir) {
  if(-not $State -or $State.Backend -eq 'xinput' -or $State.POV -eq 0xFFFF){return $false}
  $p=[uint32]($State.POV%36000)
  switch($Dir){
    'up' { return ($p -ge 31500 -or $p -le 4500) }
    'right' { return ($p -ge 4500 -and $p -le 13500) }
    'down' { return ($p -ge 13500 -and $p -le 22500) }
    'left' { return ($p -ge 22500 -and $p -le 31500) }
  }
  return $false
}
function Get-RawAxis($State,[string]$Name) {
  if(-not $State){return 0}; $n=$Name.ToLowerInvariant()
  if($State.Backend -eq 'xinput'){
    switch($n){'lx'{return [int]$State.LX};'ly'{return [int]$State.LY};'rx'{return [int]$State.RX};'ry'{return [int]$State.RY};default{return 0}}
  }
  switch($n){'x'{return [int]$State.X};'y'{return [int]$State.Y};'z'{return [int]$State.ZAxis};'r'{return [int]$State.RAxis};'u'{return [int]$State.U};'v'{return [int]$State.V};default{return 0}}
}
function Test-LogicalButton($State,[string]$Name) {
  if(-not $State){return $false}; $n=$Name.ToLowerInvariant()
  if($State.Backend -eq 'xinput'){
    $idx=@{a=0;b=1;x=2;y=3;c=4;z=5;start=7;up=10;down=11;left=12;right=13}[$n]
    if($null -eq $idx){return $false}; return Test-PhysicalButton $State $idx
  }
  $idx=@{x=0;a=1;b=2;y=3;c=4;z=5;start=9}[$n]
  if($null -ne $idx){return Test-PhysicalButton $State $idx}
  if(@('up','down','left','right') -contains $n){return Test-Pov $State $n}
  return $false
}
function Get-LogicalAxis($State,[string]$Name) {
  if(-not $State){return 0}; $n=$Name.ToLowerInvariant()
  if($State.Backend -eq 'xinput'){
    switch($n){'joy_x'{return [int]$State.LX};'joy_y'{return -[int]$State.LY};'joy2_x'{return [int]$State.RX};'joy2_y'{return -[int]$State.RY}}
  } else {
    switch($n){'joy_x'{return [int]$State.X};'joy_y'{return [int]$State.Y};'joy2_x'{return [int]$State.ZAxis};'joy2_y'{return [int]$State.RAxis}}
  }
  return 0
}
function Get-LogicalTrigger($State,[string]$Name) {
  if(-not $State){return 0}; $n=$Name.ToLowerInvariant()
  if($State.Backend -eq 'xinput'){ if($n -eq 'ltrig'){return [int]$State.LT}; if($n -eq 'rtrig'){return [int]$State.RT}; return 0 }
  # WinMM/DirectInput layouts are not consistent enough to assume U/V are
  # independent triggers. On native DS4 they may be unrelated or the two
  # triggers may be exposed as one combined centered axis. Use the dedicated
  # L2/R2 button bits as the safe zero-cross-talk default. Analog pressure is
  # enabled by a learned range:<axis>:<rest>:<full> binding.
  $button=if($n -eq 'ltrig'){6}elseif($n -eq 'rtrig'){7}else{-1}
  if($button -ge 0 -and (Test-PhysicalButton $State $button)){return 255}
  return 0
}
function Get-AxisFromBinding($State,[string]$Binding,[double]$Deadzone) {
  if(-not $Binding){return 0}; $b=$Binding.ToLowerInvariant(); $threshold=[int](32767*$Deadzone)
  if($b.StartsWith('logical:')){$v=Get-LogicalAxis $State $b.Substring(8); if([Math]::Abs($v) -le $threshold){return 0}; return $v}
  if(-not $b.StartsWith('axis:')){return 0}
  $rest=$b.Substring(5);$invert=$false
  if($rest.Contains(':invert')){$rest=$rest.Replace(':invert','');$invert=$true}
  if($rest.EndsWith('+') -or $rest.EndsWith('-')){if($rest.EndsWith('-')){$invert=-not $invert};$rest=$rest.Substring(0,$rest.Length-1)}
  $v=Get-RawAxis $State $rest; if([Math]::Abs($v) -le $threshold){return 0}; if($invert){$v=-$v}; return $v
}
function Test-Binding($State,[string]$Binding,[double]$Deadzone) {
  if(-not $Binding){return $false};$b=$Binding.ToLowerInvariant();$threshold=[int](32767*$Deadzone)
  if($b.StartsWith('logical:')){return Test-LogicalButton $State $b.Substring(8)}
  if($b.StartsWith('button:')){try{return Test-PhysicalButton $State ([int]$b.Substring(7))}catch{return $false}}
  if($b -eq 'trigger:lt'){return ([int]$State.LT -gt 80)}; if($b -eq 'trigger:rt'){return ([int]$State.RT -gt 80)}
  if($b.StartsWith('pov:')){return Test-Pov $State $b.Substring(4)}
  if($b.StartsWith('axis:')){
    $rest=$b.Substring(5);$dir='';$parts=$rest.Split(':');$axis=$parts[0]
    if($axis.EndsWith('+') -or $axis.EndsWith('-')){$dir=$axis.Substring($axis.Length-1);$axis=$axis.Substring(0,$axis.Length-1)}
    $v=Get-RawAxis $State $axis; if($dir -eq '+'){return $v -gt $threshold};if($dir -eq '-'){return $v -lt -$threshold};return [Math]::Abs($v) -gt $threshold
  }
  return $false
}
function Get-TriggerFromBinding($State,[string]$Binding,[double]$Deadzone) {
  if(-not $Binding){return 0};$b=$Binding.ToLowerInvariant()
  if($b.StartsWith('logical:')){return Get-LogicalTrigger $State $b.Substring(8)}
  if($b -eq 'trigger:lt'){return [int]$State.LT};if($b -eq 'trigger:rt'){return [int]$State.RT}
  if($b.StartsWith('range:')){
    $parts=$b.Split(':');if($parts.Count -ge 4){
      try {
        $raw=[double](Get-RawAxis $State $parts[1]);$rest=[double]$parts[2];$full=[double]$parts[3];$span=$full-$rest
        if([Math]::Abs($span) -lt 1.0){return 0}
        $t=($raw-$rest)/$span;return [Math]::Max(0,[Math]::Min(255,[int][Math]::Round($t*255.0)))
      } catch { return 0 }
    };return 0
  }
  if($b.StartsWith('axis:')){$v=[Math]::Abs((Get-AxisFromBinding $State $Binding $Deadzone));return [Math]::Min(255,[int]($v/128))}
  if(Test-Binding $State $Binding $Deadzone){return 255};return 0
}
function Get-MappedState($State,$Profile) {
  if(-not $State){return $null};$b=$Profile.bindings;$dz=[double]$Profile.deadzone
  $r=[ordered]@{Backend=$State.Backend;Device=$State.Device;Raw=$State}
  foreach($k in @('a','b','x','y','c','z','start','up','down','left','right')){$r[$k]=[bool](Test-Binding $State ([string]$b[$k]) $dz)}
  foreach($k in @('joy_x','joy_y','joy2_x','joy2_y')){$r[$k]=[int](Get-AxisFromBinding $State ([string]$b[$k]) $dz)}
  $r['ltrig']=[int](Get-TriggerFromBinding $State ([string]$b['ltrig']) $dz);$r['rtrig']=[int](Get-TriggerFromBinding $State ([string]$b['rtrig']) $dz)
  return [pscustomobject]$r
}

function Normalize-Axis([int]$v){return [Math]::Max(-1.0,[Math]::Min(1.0,$v/32767.0))}
function Raw-AxisSummary($s){if(-not $s){return '-'};if($s.Backend -eq 'xinput'){return "LX=$($s.LX)  LY=$($s.LY)  RX=$($s.RX)  RY=$($s.RY)  LT=$($s.LT)  RT=$($s.RT)"};return "X=$($s.X)  Y=$($s.Y)  Z=$($s.ZAxis)  R=$($s.RAxis)  U=$($s.U)  V=$($s.V)"}

$script:profile=Read-ControllerProfile $ProfilePath
$state=$null;$mapped=$null;$lastConnected=$false
$script:learnActive=$false;$script:learnStep=0;$script:learnBase=$null;$script:learnResult=[ordered]@{}
$script:learnSteps=@(
  @{Name='joy_x';Text='Mueve el STICK IZQUIERDO completamente a la DERECHA';Type='axis'},
  @{Name='joy_y';Text='Mueve el STICK IZQUIERDO completamente hacia ABAJO';Type='axis'},
  @{Name='joy2_x';Text='Mueve el STICK DERECHO completamente a la DERECHA';Type='axis'},
  @{Name='joy2_y';Text='Mueve el STICK DERECHO completamente hacia ABAJO';Type='axis'},
  @{Name='up';Text='Presiona ARRIBA en la cruceta';Type='button'},
  @{Name='a';Text='Presiona CROSS / A';Type='button'},
  @{Name='b';Text='Presiona CIRCLE / B';Type='button'},
  @{Name='start';Text='Presiona OPTIONS / START';Type='button'},
  @{Name='ltrig';Text='Pulsa L2 completamente, mantenlo un instante y SUELTALO';Type='trigger'},
  @{Name='rtrig';Text='Pulsa R2 completamente, mantenlo un instante y SUELTALO';Type='trigger'}
)
function Update-LearningPrompt {
  if(-not $script:learnActive){return}
  if($script:learnStep -lt 0 -or $script:learnStep -ge $script:learnSteps.Count){return}
  $nextText=[string]$script:learnSteps[$script:learnStep].Text
  $script:learnInfo.Text=('OBJETIVO - Paso {0}/{1}: {2}' -f ($script:learnStep+1),$script:learnSteps.Count,$nextText)
  $script:learnInfo.BackColor=[Drawing.Color]::FromArgb(255,244,190)
  $script:learnLive.Text=('Estado: esperando la accion del paso {0}...' -f ($script:learnStep+1))
  $script:learnStart.Text='Aprendizaje activo...'
  $script:learnStart.Enabled=$false
  $script:learnInfo.Refresh()
  $script:learnLive.Refresh()
  $script:learnInfo.Update()
  $script:learnLive.Update()
}
function Detect-RawChange($prev,$cur,[string]$type,[double]$dz){
  if(-not $prev -or -not $cur){return $null};$threshold=[int](32767*[Math]::Max(0.20,$dz))
  if($type -eq 'button'){
    if($cur.Backend -eq 'xinput'){
      for($i=0;$i -lt $xMasks.Count;$i++){if((Test-PhysicalButton $cur $i) -and -not(Test-PhysicalButton $prev $i)){return "button:$i"}}
    } else {
      for($i=0;$i -lt 32;$i++){if((Test-PhysicalButton $cur $i) -and -not(Test-PhysicalButton $prev $i)){return "button:$i"}}
      foreach($d in @('up','right','down','left')){if((Test-Pov $cur $d) -and -not(Test-Pov $prev $d)){return "pov:$d"}}
    }
  } else {
    $axes=if($cur.Backend -eq 'xinput'){@('lx','ly','rx','ry')}else{@('x','y','z','r','u','v')}
    foreach($a in $axes){$v=Get-RawAxis $cur $a;$old=Get-RawAxis $prev $a;if([Math]::Abs($v) -gt $threshold -and [Math]::Abs($v-$old) -gt [int]($threshold*0.75)){return "axis:$a"}}
  }
  return $null
}
function Get-LearningObservation($prev,$cur) {
  if(-not $prev -or -not $cur){return 'Esperando estado del mando...'}
  $events=New-Object System.Collections.Generic.List[string]
  if($cur.Backend -eq 'xinput'){
    for($i=0;$i -lt $xMasks.Count;$i++){
      if((Test-PhysicalButton $cur $i) -and -not(Test-PhysicalButton $prev $i)){[void]$events.Add(('button:{0}' -f $i))}
    }
  } else {
    for($i=0;$i -lt 32;$i++){
      if((Test-PhysicalButton $cur $i) -and -not(Test-PhysicalButton $prev $i)){[void]$events.Add(('button:{0}' -f $i))}
    }
    foreach($d in @('up','right','down','left')){
      if((Test-Pov $cur $d) -and -not(Test-Pov $prev $d)){[void]$events.Add(('pov:{0}' -f $d))}
    }
  }
  $axes=if($cur.Backend -eq 'xinput'){@('lx','ly','rx','ry')}else{@('x','y','z','r','u','v')}
  $bestAxis='';$bestDelta=0;$bestValue=0
  foreach($a in $axes){
    $v=[int](Get-RawAxis $cur $a);$old=[int](Get-RawAxis $prev $a);$delta=[Math]::Abs($v-$old)
    if($delta -gt $bestDelta){$bestDelta=$delta;$bestAxis=$a;$bestValue=$v}
  }
  if($events.Count -gt 0){return ('Detectado RAW: {0}' -f ($events -join ', '))}
  if($bestAxis -and $bestDelta -ge 512){return ('Movimiento RAW: axis:{0}  valor={1}  delta={2}' -f $bestAxis,$bestValue,$bestDelta)}
  return 'Esperando movimiento o boton...'
}



$script:lastNeutralState=$null
$script:triggerCapture=$null
function Trigger-IsDown($State,[string]$Name) {
  if(-not $State){return $false}
  if($State.Backend -eq 'xinput'){
    if($Name -eq 'ltrig'){return [int]$State.LT -gt 8}
    if($Name -eq 'rtrig'){return [int]$State.RT -gt 8}
    return $false
  }
  $button=if($Name -eq 'ltrig'){6}else{7};return Test-PhysicalButton $State $button
}
function Begin-TriggerCapture($Neutral,$Current,[string]$Name) {
  $button=if($Name -eq 'ltrig'){6}else{7}
  return [pscustomobject]@{Name=$Name;Neutral=$Neutral;Button=$button;BestAxis='';BestDelta=0;BestFull=0;Started=$true}
}
function Update-TriggerCapture($Capture,$Current) {
  if(-not $Capture -or -not $Current){return}
  if($Current.Backend -eq 'xinput'){return}
  foreach($a in @('x','y','z','r','u','v')){
    $rest=[int](Get-RawAxis $Capture.Neutral $a);$now=[int](Get-RawAxis $Current $a);$delta=[Math]::Abs($now-$rest)
    if($delta -gt [int]$Capture.BestDelta){$Capture.BestDelta=$delta;$Capture.BestAxis=$a;$Capture.BestFull=$now}
  }
}
function Finish-TriggerCapture($Capture) {
  if(-not $Capture){return $null}
  if($Capture.Neutral.Backend -eq 'xinput'){return $(if($Capture.Name -eq 'ltrig'){'trigger:lt'}else{'trigger:rt'})}
  # Require a meaningful analog excursion; otherwise keep the reliable
  # independent L2/R2 button bit rather than inventing an axis mapping.
  if([int]$Capture.BestDelta -ge 4096 -and $Capture.BestAxis){
    $rest=[int](Get-RawAxis $Capture.Neutral $Capture.BestAxis)
    return ('range:{0}:{1}:{2}' -f $Capture.BestAxis,$rest,[int]$Capture.BestFull)
  }
  return ('button:{0}' -f [int]$Capture.Button)
}

$form=New-Object Windows.Forms.Form
$form.Text='DreamcastRecomp v0.1 - Controller Visual Tester'
$form.StartPosition='CenterScreen';$form.Size=[Drawing.Size]::new(1120,780);$form.MinimumSize=[Drawing.Size]::new(1000,700)
$form.Font=[Drawing.Font]::new('Segoe UI',9)

# Fixed two-row layout. Using two independently docked controls on the Form can
# overlap on some Windows DPI/layout combinations, hiding the tab headers and the
# first lines of the Learning page. A TableLayoutPanel makes the separation hard.
$rootLayout=New-Object Windows.Forms.TableLayoutPanel
$rootLayout.Dock='Fill';$rootLayout.RowCount=2;$rootLayout.ColumnCount=1
$rootLayout.ColumnStyles.Add([Windows.Forms.ColumnStyle]::new([Windows.Forms.SizeType]::Percent,100))|Out-Null
$rootLayout.RowStyles.Add([Windows.Forms.RowStyle]::new([Windows.Forms.SizeType]::Absolute,76))|Out-Null
$rootLayout.RowStyles.Add([Windows.Forms.RowStyle]::new([Windows.Forms.SizeType]::Percent,100))|Out-Null
$form.Controls.Add($rootLayout)

$top=New-Object Windows.Forms.Panel;$top.Dock='Fill';$rootLayout.Controls.Add($top,0,0)
$title=New-Object Windows.Forms.Label;$title.Text='Controller Visual Tester';$title.Font=[Drawing.Font]::new('Segoe UI',16,[Drawing.FontStyle]::Bold);$title.Location='14,8';$title.Size='250,32';$top.Controls.Add($title)
$status=New-Object Windows.Forms.Label;$status.Location='280,10';$status.Size='790,24';$status.Text='Buscando mando...';$top.Controls.Add($status)
$profileLabel=New-Object Windows.Forms.Label;$profileLabel.Location='280,36';$profileLabel.Size='790,32';$profileLabel.Text="Perfil: $ProfilePath | backend=$($script:profile.backend) device=$($script:profile.device) deadzone=$([int]($script:profile.deadzone*100))%";$top.Controls.Add($profileLabel)

$tabs=New-Object Windows.Forms.TabControl;$tabs.Dock='Fill';$rootLayout.Controls.Add($tabs,0,1)
$tabVisual=New-Object Windows.Forms.TabPage;$tabVisual.Text='Mando visual';[void]$tabs.TabPages.Add($tabVisual)
$tabRaw=New-Object Windows.Forms.TabPage;$tabRaw.Text='Raw / diagnostico';[void]$tabs.TabPages.Add($tabRaw)
$tabLearn=New-Object Windows.Forms.TabPage;$tabLearn.Text='Aprendizaje';[void]$tabs.TabPages.Add($tabLearn)

$visual=New-Object Windows.Forms.Panel;$visual.Dock='Fill';$visual.BackColor=[Drawing.Color]::FromArgb(245,247,250);$tabVisual.Controls.Add($visual)

# Prevent visible background erase + repaint cycles in Windows PowerShell/WinForms.
$doubleBufferedProperty=[Windows.Forms.Control].GetProperty('DoubleBuffered',[Reflection.BindingFlags]'NonPublic,Instance')
if($doubleBufferedProperty){
  $doubleBufferedProperty.SetValue($visual,$true,$null)
  $doubleBufferedProperty.SetValue($form,$true,$null)
}

# Reuse GDI resources instead of allocating them on every Paint event.
$script:controllerBodyBrush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(225,229,235))
$script:controllerBodyPen=[Drawing.Pen]::new([Drawing.Color]::FromArgb(120,130,145),2)
$script:fontB=[Drawing.Font]::new('Segoe UI',12,[Drawing.FontStyle]::Bold)
$script:fontS=[Drawing.Font]::new('Consolas',9)
$script:onBrush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(82,180,110))
$script:offBrush=[Drawing.SolidBrush]::new([Drawing.Color]::White)
$script:linePen=[Drawing.Pen]::new([Drawing.Color]::FromArgb(75,85,100),2)
$script:gridPen=[Drawing.Pen]::new([Drawing.Color]::LightGray,1)

$visual.Add_Paint({
  param($sender,$e);$g=$e.Graphics;$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $w=$sender.ClientSize.Width;$h=$sender.ClientSize.Height
  $body=[Drawing.RectangleF]::new([single]($w*0.13),[single]($h*0.16),[single]($w*0.74),[single]($h*0.60))
  $g.FillEllipse($script:controllerBodyBrush,$body)
  $g.DrawEllipse($script:controllerBodyPen,$body)
  function Draw-Btn([string]$txt,[single]$x,[single]$y,[bool]$on){$r=[Drawing.RectangleF]::new([single]($x-23),[single]($y-23),46,46);$g.FillEllipse($(if($on){$script:onBrush}else{$script:offBrush}),$r);$g.DrawEllipse($script:linePen,$r);$sz=$g.MeasureString($txt,$script:fontB);$g.DrawString($txt,$script:fontB,[Drawing.Brushes]::Black,$x-$sz.Width/2,$y-$sz.Height/2)}
  function Draw-RectBtn([string]$txt,[single]$x,[single]$y,[single]$rw,[single]$rh,[bool]$on){$r=[Drawing.RectangleF]::new($x,$y,$rw,$rh);$g.FillRectangle($(if($on){$script:onBrush}else{$script:offBrush}),$r);$g.DrawRectangle($script:linePen,$x,$y,$rw,$rh);$g.DrawString($txt,$script:fontB,[Drawing.Brushes]::Black,$x+6,$y+5)}
  $m=$script:mapped
  $cx=[single]($w*0.27);$cy=[single]($h*0.37)
  $glyphUp=[string][char]0x2191;$glyphDown=[string][char]0x2193;$glyphLeft=[string][char]0x2190;$glyphRight=[string][char]0x2192
  Draw-RectBtn $glyphUp ($cx-22) ($cy-72) 44 48 ([bool]($m -and $m.up));Draw-RectBtn $glyphDown ($cx-22) ($cy+24) 44 48 ([bool]($m -and $m.down));Draw-RectBtn $glyphLeft ($cx-70) ($cy-24) 48 48 ([bool]($m -and $m.left));Draw-RectBtn $glyphRight ($cx+22) ($cy-24) 48 48 ([bool]($m -and $m.right))
  $fx=[single]($w*0.73);$fy=[single]($h*0.37);Draw-Btn 'Y' $fx ($fy-64) ([bool]($m -and $m.y));Draw-Btn 'A' $fx ($fy+64) ([bool]($m -and $m.a));Draw-Btn 'X' ($fx-64) $fy ([bool]($m -and $m.x));Draw-Btn 'B' ($fx+64) $fy ([bool]($m -and $m.b))
  Draw-Btn 'C' ($fx-112) ($fy-90) ([bool]($m -and $m.c));Draw-Btn 'Z' ($fx+112) ($fy-90) ([bool]($m -and $m.z))
  Draw-RectBtn 'START' ([single]($w*0.465)) ([single]($h*0.31)) 72 34 ([bool]($m -and $m.start))
  # analog sticks
  foreach($spec in @(@('L',[single]($w*0.39),[single]($h*0.59),'joy_x','joy_y'),@('R',[single]($w*0.61),[single]($h*0.59),'joy2_x','joy2_y'))){
    $label=$spec[0];$sx=[single]$spec[1];$sy=[single]$spec[2];$ax=$spec[3];$ay=$spec[4];$r=52
    $g.FillEllipse([Drawing.Brushes]::White,$sx-$r,$sy-$r,$r*2,$r*2);$g.DrawEllipse($script:linePen,$sx-$r,$sy-$r,$r*2,$r*2);$g.DrawLine($script:gridPen,$sx-$r,$sy,$sx+$r,$sy);$g.DrawLine($script:gridPen,$sx,$sy-$r,$sx,$sy+$r)
    $nx=if($m){Normalize-Axis ([int]$m.$ax)}else{0};$ny=if($m){Normalize-Axis ([int]$m.$ay)}else{0};$px=[single]($sx+$nx*($r-8));$py=[single]($sy+$ny*($r-8));$g.FillEllipse($script:onBrush,$px-8,$py-8,16,16);$g.DrawEllipse($script:linePen,$px-8,$py-8,16,16);$g.DrawString("$label  X=$('{0,6:N3}' -f $nx)  Y=$('{0,6:N3}' -f $ny)",$script:fontS,[Drawing.Brushes]::Black,$sx-85,$sy+$r+8)
  }
  # triggers
  $lt=if($m){[int]$m.ltrig}else{0};$rt=if($m){[int]$m.rtrig}else{0};$barW=180;$barH=24
  foreach($t in @(@('L2 / L',$lt,[single]($w*0.18)),@('R2 / R',$rt,[single]($w*0.66)))){$label=$t[0];$v=[int]$t[1];$x=[single]$t[2];$y=[single]($h*0.09);$g.FillRectangle([Drawing.Brushes]::White,$x,$y,$barW,$barH);$g.FillRectangle($script:onBrush,$x,$y,[single]($barW*$v/255.0),$barH);$g.DrawRectangle($script:linePen,$x,$y,$barW,$barH);$g.DrawString("$label  $v/255",$script:fontS,[Drawing.Brushes]::Black,$x,$y+$barH+4)}
  $g.DrawString('La vista muestra el estado YA MAPEADO a Dreamcast. Si el punto se mueve aqui, el juego recibe ese analogico.',$script:fontS,[Drawing.Brushes]::DimGray,18,[single]($h-42))
})

$rawBox=New-Object Windows.Forms.TextBox;$rawBox.Dock='Fill';$rawBox.Multiline=$true;$rawBox.ReadOnly=$true;$rawBox.ScrollBars='Vertical';$rawBox.Font=[Drawing.Font]::new('Consolas',10);$rawBox.BackColor=[Drawing.Color]::FromArgb(25,27,31);$rawBox.ForeColor=[Drawing.Color]::Gainsboro;$tabRaw.Controls.Add($rawBox)

$learnTop=New-Object Windows.Forms.Panel;$learnTop.Dock='Top';$learnTop.Height=145;$tabLearn.Controls.Add($learnTop)
$script:learnInfo=New-Object Windows.Forms.Label;$learnInfo=$script:learnInfo;$learnInfo.Location='14,10';$learnInfo.Size='1000,44';$learnInfo.Font=[Drawing.Font]::new('Segoe UI',11,[Drawing.FontStyle]::Bold);$learnInfo.Text='Presiona Iniciar aprendizaje. Luego realiza SOLO la accion indicada en cada paso.';$learnTop.Controls.Add($learnInfo)
$script:learnLive=New-Object Windows.Forms.Label;$learnLive=$script:learnLive;$learnLive.Location='14,55';$learnLive.Size='1000,28';$learnLive.Font=[Drawing.Font]::new('Consolas',9);$learnLive.ForeColor=[Drawing.Color]::DarkBlue;$learnLive.Text='Estado: detenido.';$learnTop.Controls.Add($learnLive)
$script:learnStart=New-Object Windows.Forms.Button;$learnStart=$script:learnStart;$learnStart.Text='1) Iniciar aprendizaje';$learnStart.Location='14,100';$learnStart.Size='170,30';$learnTop.Controls.Add($learnStart)
$script:learnSave=New-Object Windows.Forms.Button;$learnSave=$script:learnSave;$learnSave.Text='Guardar como perfil';$learnSave.Location='194,100';$learnSave.Size='150,30';$learnSave.Enabled=$false;$learnTop.Controls.Add($learnSave)
$learnReset=New-Object Windows.Forms.Button;$learnReset.Text='Cancelar / reset';$learnReset.Location='354,100';$learnReset.Size='130,30';$learnTop.Controls.Add($learnReset)
$script:learnText=New-Object Windows.Forms.TextBox;$learnText=$script:learnText;$learnText.Dock='Fill';$learnText.Multiline=$true;$learnText.ReadOnly=$true;$learnText.Font=[Drawing.Font]::new('Consolas',10);$learnText.ScrollBars='Vertical';$tabLearn.Controls.Add($learnText)

$learnStart.Add_Click({
  try {
    if(-not $script:state){[Windows.Forms.MessageBox]::Show('No hay mando detectado.','DreamcastRecomp')|Out-Null;return}
    $script:learnResult=[ordered]@{}
    $script:learnStep=0
    $script:learnActive=$true
    $script:learnBase=$script:state
    $script:triggerCapture=$null
    $script:learnText.Clear()
    $script:learnSave.Enabled=$false
    Update-LearningPrompt
    $script:learnText.AppendText("Aprendizaje iniciado.`r`nCada linea aparece cuando el paso fue detectado correctamente.`r`n`r`n")
  } catch {
    $script:learnActive=$false
    $script:learnStart.Text='1) Iniciar aprendizaje'
    $script:learnStart.Enabled=$true
    [Windows.Forms.MessageBox]::Show(('No se pudo iniciar aprendizaje:`r`n{0}' -f $_.Exception.Message),'DreamcastRecomp')|Out-Null
  }
})
$learnReset.Add_Click({$script:learnActive=$false;$script:learnStep=0;$script:learnResult=[ordered]@{};$script:triggerCapture=$null;$script:learnInfo.Text='Aprendizaje cancelado. Pulsa Iniciar aprendizaje para comenzar de nuevo.';$script:learnInfo.BackColor=[Drawing.SystemColors]::Control;$script:learnLive.Text='Estado: detenido.';$script:learnSave.Enabled=$false;$script:learnStart.Text='1) Iniciar aprendizaje';$script:learnStart.Enabled=$true})
$learnSave.Add_Click({
  if($script:learnResult.Count -eq 0){return};$p=Read-ControllerProfile $ProfilePath
  foreach($k in $script:learnResult.Keys){$p.bindings[$k]=$script:learnResult[$k]}
  $dir=Split-Path $ProfilePath -Parent;if($dir -and -not(Test-Path $dir)){New-Item -ItemType Directory -Force -Path $dir|Out-Null}
  $lines=@('[DreamcastRecompController]','version=2',"backend=$($p.backend)","device=$($p.device)","deadzone=$($p.deadzone)")
  foreach($k in $p.bindings.Keys){$lines+="$k=$($p.bindings[$k])"};Set-Content -Encoding ASCII -LiteralPath $ProfilePath -Value $lines
  $script:profile=Read-ControllerProfile $ProfilePath;$profileLabel.Text="Perfil: $ProfilePath | backend=$($script:profile.backend) device=$($script:profile.device) deadzone=$([int]($script:profile.deadzone*100))%";$learnInfo.Text='Perfil guardado. El tester ya esta usando esos bindings.';$learnLive.Text='Estado: perfil guardado correctamente.'
})

$script:lastVisualFingerprint=''
$script:lastStatusText=''
$script:lastRawText=''

$timer=New-Object Windows.Forms.Timer;$timer.Interval=33;$timer.Add_Tick({
  $s=Get-PadState ([string]$script:profile.backend) ([int]$script:profile.device);$script:state=$s;$script:mapped=if($s){Get-MappedState $s $script:profile}else{$null}

  $newStatusText=if($s){"CONECTADO  |  backend=$($s.Backend)  device=#$($s.Device)  |  buttons=0x$('{0:X8}' -f $s.Buttons)  POV=$($s.POV)"}else{'NO SE DETECTA MANDO. Conecta el control por USB/Bluetooth y cierra DS4Windows para probar PS4 nativo.'}
  if($newStatusText -ne $script:lastStatusText){
    $status.Text=$newStatusText
    $status.ForeColor=if($s){[Drawing.Color]::DarkGreen}else{[Drawing.Color]::DarkRed}
    $script:lastStatusText=$newStatusText
  }
  if($s -and $mapped){
    $bindingLines = @($script:profile.bindings.GetEnumerator() | ForEach-Object { ('{0}={1}' -f $_.Key,$_.Value) })
    $diagLines = @(
      'DreamcastRecomp v0.1 - Controller diagnostics',
      '===============================================',
      ('Backend: {0}' -f $s.Backend),
      ('Device : {0}' -f $s.Device),
      ('Profile: {0}' -f $ProfilePath),
      ('Deadzone: {0}%' -f [int]($script:profile.deadzone*100)),
      '',
      'RAW',
      '---',
      ('Buttons = 0x{0:X8}' -f $s.Buttons),
      ('POV     = {0}' -f $s.POV),
      (Raw-AxisSummary $s),
      '',
      'MAPPED TO DREAMCAST',
      '-------------------',
      ('DPad U/D/L/R = {0} / {1} / {2} / {3}' -f $mapped.up,$mapped.down,$mapped.left,$mapped.right),
      ('A/B/X/Y      = {0} / {1} / {2} / {3}' -f $mapped.a,$mapped.b,$mapped.x,$mapped.y),
      ('C/Z/START    = {0} / {1} / {2}' -f $mapped.c,$mapped.z,$mapped.start),
      ('Left stick   = X={0}  Y={1}  normalized={2},{3}' -f $mapped.joy_x,$mapped.joy_y,('{0:N3}' -f (Normalize-Axis $mapped.joy_x)),('{0:N3}' -f (Normalize-Axis $mapped.joy_y))),
      ('Right stick  = X={0}  Y={1}  normalized={2},{3}' -f $mapped.joy2_x,$mapped.joy2_y,('{0:N3}' -f (Normalize-Axis $mapped.joy2_x)),('{0:N3}' -f (Normalize-Axis $mapped.joy2_y))),
      ('Triggers     = L={0}/255  R={1}/255' -f $mapped.ltrig,$mapped.rtrig),
      '',
      'BINDINGS',
      '--------'
    )
    $newRawText=(($diagLines + $bindingLines) -join "`r`n")
    if($tabs.SelectedTab -eq $tabRaw -and $newRawText -ne $script:lastRawText){
      $rawBox.Text=$newRawText
      $script:lastRawText=$newRawText
    }
  } else {
    if($tabs.SelectedTab -eq $tabRaw -and $script:lastRawText -ne 'Sin mando detectado.'){
      $rawBox.Text='Sin mando detectado.'
      $script:lastRawText='Sin mando detectado.'
    }
  }
  # Keep a fresh neutral reference whenever neither trigger is held. This is
  # what lets the learner handle both separate trigger axes and one combined
  # DirectInput axis without guessing U/V.
  if($s -and -not(Trigger-IsDown $s 'ltrig') -and -not(Trigger-IsDown $s 'rtrig')){$script:lastNeutralState=$s}
  if($script:learnActive -and $s){
    $step=$script:learnSteps[$script:learnStep]
    $observation=Get-LearningObservation $script:learnBase $s
    $script:learnLive.Text=('Estado: {0}' -f $observation)
    if($step.Type -eq 'trigger'){
      $down=Trigger-IsDown $s $step.Name
      if(-not $script:triggerCapture -and $down){
        $neutral=if($script:lastNeutralState){$script:lastNeutralState}else{$script:learnBase}
        $script:triggerCapture=Begin-TriggerCapture $neutral $s $step.Name
        Update-TriggerCapture $script:triggerCapture $s
        $script:learnLive.Text=('Estado: {0} detectado; mantenlo y luego SUELTALO.' -f $step.Name.ToUpperInvariant())
      } elseif($script:triggerCapture -and $down){
        Update-TriggerCapture $script:triggerCapture $s
        $script:learnLive.Text=('Estado: capturando {0}... eje candidato={1} delta={2}. Ahora sueltalo.' -f $step.Name.ToUpperInvariant(),$script:triggerCapture.BestAxis,$script:triggerCapture.BestDelta)
      } elseif($script:triggerCapture -and -not $down){
        $binding=Finish-TriggerCapture $script:triggerCapture;$script:triggerCapture=$null
        $script:learnResult[$step.Name]=$binding;$script:learnText.AppendText("$($step.Name) = $binding`r`n");$script:learnLive.Text=('Detectado: {0} = {1}' -f $step.Name,$binding);$script:learnStep++
        if($script:learnStep -ge $script:learnSteps.Count){$script:learnActive=$false;$script:learnInfo.Text='Aprendizaje completo. Revisa el resultado y pulsa Guardar como perfil si coincide.';$script:learnInfo.BackColor=[Drawing.Color]::FromArgb(220,255,220);$script:learnLive.Text='Estado: aprendizaje completo.';$script:learnSave.Enabled=$true;$script:learnStart.Text='1) Iniciar aprendizaje';$script:learnStart.Enabled=$true}
        else{Update-LearningPrompt}
        $script:learnBase=$s
      }
    } else {
      $hit=Detect-RawChange $script:learnBase $s $step.Type ([double]$script:profile.deadzone)
      if($hit){
        $binding=$hit
        if($step.Type -eq 'axis' -and $step.Name -match 'joy.*_y$' -and $s.Backend -eq 'xinput'){$binding="$hit`:invert"}
        $script:learnResult[$step.Name]=$binding;$script:learnText.AppendText("$($step.Name) = $binding`r`n");$script:learnLive.Text=('Detectado: {0} = {1}' -f $step.Name,$binding);$script:learnStep++
        if($script:learnStep -ge $script:learnSteps.Count){$script:learnActive=$false;$script:learnInfo.Text='Aprendizaje completo. Revisa el resultado y pulsa Guardar como perfil si coincide.';$script:learnInfo.BackColor=[Drawing.Color]::FromArgb(220,255,220);$script:learnLive.Text='Estado: aprendizaje completo.';$script:learnSave.Enabled=$true;$script:learnStart.Text='1) Iniciar aprendizaje';$script:learnStart.Enabled=$true}
        else{Update-LearningPrompt}
        $script:learnBase=$s
      }
    }
  }
  $m=$script:mapped
  $visualFingerprint=if($m){
    # Quantize analog values to visual resolution. Tiny HID jitter should not
    # request a repaint when the stick dot would remain on the same pixels.
    $qx=[int]([int]$m.joy_x/512);$qy=[int]([int]$m.joy_y/512)
    $qrx=[int]([int]$m.joy2_x/512);$qry=[int]([int]$m.joy2_y/512)
    $ql=[int]([int]$m.ltrig/2);$qr=[int]([int]$m.rtrig/2)
    ('{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}|{8}|{9}|{10}|{11}|{12}|{13}|{14}|{15}|{16}' -f
      $m.a,$m.b,$m.x,$m.y,$m.c,$m.z,$m.start,$m.up,$m.down,$m.left,$m.right,
      $qx,$qy,$qrx,$qry,$ql,$qr)
  }else{'disconnected'}
  if($tabs.SelectedTab -eq $tabVisual -and $visualFingerprint -ne $script:lastVisualFingerprint){
    $script:lastVisualFingerprint=$visualFingerprint
    $visual.Invalidate()
  }
});$timer.Start()

# Ensure the current state is drawn immediately when returning to the visual tab.
$tabs.Add_SelectedIndexChanged({
  if($tabs.SelectedTab -eq $tabVisual){$visual.Invalidate()}
  elseif($tabs.SelectedTab -eq $tabRaw){$script:lastRawText=''}
  elseif($tabs.SelectedTab -eq $tabLearn -and -not $script:learnActive -and $script:learnResult.Count -eq 0){
    $script:learnInfo.Text='Presiona Iniciar aprendizaje. Luego realiza SOLO la accion indicada en cada paso.'
    $script:learnInfo.BackColor=[Drawing.SystemColors]::Control
    $script:learnLive.Text='Estado: esperando que pulses Iniciar aprendizaje.'
    $script:learnStart.Text='1) Iniciar aprendizaje'
    $script:learnStart.Enabled=$true
  }
})

$form.Add_FormClosed({
  $timer.Stop()
  foreach($resource in @($script:controllerBodyBrush,$script:controllerBodyPen,$script:fontB,$script:fontS,$script:onBrush,$script:offBrush,$script:linePen,$script:gridPen)){
    if($resource){$resource.Dispose()}
  }
})
[void]$form.ShowDialog()
