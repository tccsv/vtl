param(
    [Parameter(Mandatory=$true)][string]$SmpDir,
    [Parameter(Mandatory=$true)][string]$VsnasmDir
)

$ErrorActionPreference = 'Stop'

# Патчит SMP/*.vcxproj — заменяет глобальные nasm.props/nasm.targets
# импорты на путь к локальному VSNASM в external_sources/tools/vsnasm/.
# Идемпотентно: повторный запуск не делает ничего.

if (-not (Test-Path $SmpDir)) { throw "SmpDir not found: $SmpDir" }
if (-not (Test-Path $VsnasmDir)) { throw "VsnasmDir not found: $VsnasmDir" }

$patched = 0
Get-ChildItem -Path $SmpDir -Filter '*.vcxproj' | ForEach-Object {
    $path = $_.FullName
    $content = Get-Content -LiteralPath $path -Raw

    $needle1 = '$(VCTargetsPath)\BuildCustomizations\nasm.props'
    $needle2 = '$(VCTargetsPath)\BuildCustomizations\nasm.targets'
    $replace1 = "$VsnasmDir\nasm.props"
    $replace2 = "$VsnasmDir\nasm.targets"

    $changed = $false
    if ($content.Contains($needle1)) {
        $content = $content.Replace($needle1, $replace1)
        $changed = $true
    }
    if ($content.Contains($needle2)) {
        $content = $content.Replace($needle2, $replace2)
        $changed = $true
    }

    if ($changed) {
        Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding UTF8
        $patched++
        Write-Host "patched: $($_.Name)"
    }
}

Write-Host "Patched $patched vcxproj files"
