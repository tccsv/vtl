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

    # Заменяем путь импорта nasm.props/nasm.targets на локальный VSNASM —
    # независимо от того, что там стоит сейчас: токен $(VCTargetsPath)\... или
    # абсолютный путь с чужой машины (SMP-проекты коммитятся с absolute paths).
    $before = $content
    $props_eval   = { 'Project="' + $VsnasmDir + '\nasm.props"' }
    $targets_eval = { 'Project="' + $VsnasmDir + '\nasm.targets"' }
    $content = [regex]::Replace($content, 'Project="[^"]*nasm\.props"',   $props_eval)
    $content = [regex]::Replace($content, 'Project="[^"]*nasm\.targets"', $targets_eval)
    $changed = ($content -ne $before)

    if ($changed) {
        Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding UTF8
        $patched++
        Write-Host "patched: $($_.Name)"
    }
}

Write-Host "Patched $patched vcxproj files"
