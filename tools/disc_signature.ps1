param(
    [Parameter(Mandatory=$true)][string]$Disc
)
$ErrorActionPreference = "Stop"
$item = Get-Item -LiteralPath $Disc
$payload = $item.FullName.ToLowerInvariant() + "`n" + $item.Length.ToString() + "`n" + $item.LastWriteTimeUtc.Ticks.ToString()
$sha = [System.Security.Cryptography.SHA256]::Create()
try {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
    $hash = $sha.ComputeHash($bytes)
    ([System.BitConverter]::ToString($hash)).Replace("-", "")
} finally {
    $sha.Dispose()
}
