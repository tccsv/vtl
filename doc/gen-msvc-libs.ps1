param(
    [string]$WinDir = (Join-Path (Split-Path -Parent $PSScriptRoot) "external_libs\windows")
)

# Разработческая утилита. Запускать руками раз при появлении новой DLL/новой версии:
# из x64 Native Tools Command Prompt for VS → powershell -File doc/gen-msvc-libs.ps1
#
# Что делает:
#   Берёт .dll из external_libs/windows/bin/, через dumpbin /exports читает экспорты,
#   делает .def, через lib /def генерирует MSVC import library (.lib),
#   кладёт рядом в external_libs/windows/lib/.
#
# Зачем:
#   FFmpeg/curl/openssl/libpq DLL в external_libs/windows/bin/ предсобраны MinGW'ом
#   и не несут .lib в комплекте. MSVC напрямую с DLL не линкуется — нужен import
#   library (.lib). Этот скрипт собирает .lib из exports DLL через dumpbin + lib.
#
# Эта утилита не вызывается из CMake. Готовые .lib коммитятся в репо.
# Требует dumpbin.exe и lib.exe — оба есть только в Developer Command Prompt for VS.

$ErrorActionPreference = "Stop"

$pairs = @(
    @{ Dll = "libcurl-4";       Lib = "libcurl"   },
    @{ Dll = "libssl-3-x64";    Lib = "libssl"    },
    @{ Dll = "libcrypto-3-x64"; Lib = "libcrypto" },
    @{ Dll = "libpq";           Lib = "libpq"     }
)

foreach ($p in $pairs) {
    $dllPath = Join-Path $WinDir "bin\$($p.Dll).dll"
    $libPath = Join-Path $WinDir "lib\$($p.Lib).lib"
    $defPath = Join-Path $WinDir "lib\$($p.Lib).def"

    Write-Host "Generating $($p.Lib).lib from $($p.Dll).dll"

    $dumpOut = & dumpbin /exports $dllPath
    $exports = $dumpOut |
        Select-String -Pattern '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)' |
        ForEach-Object { $_.Matches[0].Groups[1].Value }

    if (-not $exports) {
        Write-Host "  no exports found, skipping"
        continue
    }

    $def = @()
    $def += "LIBRARY $($p.Dll).dll"
    $def += "EXPORTS"
    $def += $exports
    $def -join "`r`n" | Out-File -Encoding ASCII $defPath

    & lib /nologo /def:$defPath /out:$libPath /machine:x64 | Out-Null

    Remove-Item $defPath -ErrorAction SilentlyContinue
    Remove-Item ([System.IO.Path]::ChangeExtension($libPath, ".exp")) -ErrorAction SilentlyContinue
}
