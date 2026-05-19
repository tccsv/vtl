param(
    [Parameter(Mandatory=$true)][string]$SmpDir
)

$ErrorActionPreference = 'Stop'

# Удаляет ссылки на external .lib из <AdditionalDependencies> в vcxproj.
# Эти .lib (libgcrypt, libmfx, ffnvcodec и т.д.) приходят от отключённых
# опциональных deps и линкер их не найдёт.

if (-not (Test-Path $SmpDir)) { throw "SmpDir not found: $SmpDir" }

# Каждый элемент — имя .lib (без .lib и без d-суффикса), сопоставим оба варианта.
$libs = @(
    'libgcrypt', 'gcrypt',
    'libmfx', 'mfx',
    'mfxd',
    'libgnutls', 'gnutls',
    'libssh', 'ssh',
    'libmp3lame', 'mp3lame',
    'libvorbis', 'vorbis',
    'libvorbisenc', 'vorbisenc',
    'libogg', 'ogg',
    'libspeex', 'speex',
    'libopus', 'opus',
    'libilbc', 'ilbc',
    'libtheora', 'theora',
    'libtheoraenc', 'theoraenc',
    'libtheoradec', 'theoradec',
    'libx264', 'x264',
    'libx265', 'x265',
    'libxvid', 'xvid', 'xvidcore',
    'libvpx', 'vpx',
    'libgme', 'gme',
    'libmodplug', 'modplug',
    'libsoxr', 'soxr',
    'libfreetype', 'freetype',
    'libfontconfig', 'fontconfig',
    'libfribidi', 'fribidi',
    'libharfbuzz', 'harfbuzz',
    'libass', 'ass',
    'libxml2', 'xml2',
    'libcdio', 'cdio',
    'libcdio_paranoia', 'cdio_paranoia',
    'libbluray', 'bluray',
    'libssh2', 'ssh2',
    'opengl32',
    'libopenal', 'openal',
    'libsdl2', 'sdl2', 'SDL2',
    'libbz2', 'bz2',
    'libiconv', 'iconv',
    'liblzma', 'lzma',
    'libz', 'zlib', 'z'
)
# Добавляем debug-суффиксы для каждой
$allLibs = @()
foreach ($l in $libs) {
    $allLibs += $l
    $allLibs += "${l}d"
}

$patched = 0
Get-ChildItem -Path $SmpDir -Filter '*.vcxproj' | ForEach-Object {
    $path = $_.FullName
    $content = Get-Content -LiteralPath $path -Raw
    $changed = $false

    foreach ($l in $allLibs) {
        # Сопоставляем "libname.lib;" внутри AdditionalDependencies (может быть с любым case)
        $pattern = '(?i)' + [regex]::Escape("$l.lib") + ';'
        $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern, '')
        if ($newContent -ne $content) {
            $content = $newContent
            $changed = $true
        }
    }

    if ($changed) {
        Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding UTF8
        $patched++
        Write-Host "patched: $($_.Name)"
    }
}

Write-Host "Patched $patched vcxproj"
