param(
    [Parameter(Mandatory=$true)][string]$ConfigComponentsPath
)

$ErrorActionPreference = 'Stop'

# Отключает все CONFIG_LIB<EXT>_<KIND> 1 = 0 в config_components.h.
# Также отключает mediafoundation и hwaccel-кодеки которые требуют отключённых deps.

if (-not (Test-Path $ConfigComponentsPath)) { throw "file not found: $ConfigComponentsPath" }

$content = Get-Content -LiteralPath $ConfigComponentsPath -Raw

# Префиксы external libs которые мы выключили в config.h
$prefixes = @(
    'LIBAOM','LIBARIBB24','LIBARIBCAPTION','LIBCELT','LIBCODEC2',
    'LIBDAV1D','LIBFDK','LIBFREETYPE','LIBGSM','LIBILBC',
    'LIBJXL','LIBKVAZAAR','LIBMP3LAME','LIBOPENCORE','LIBOPENH264',
    'LIBOPENJPEG','LIBOPUS','LIBRAV1E','LIBSHINE','LIBSPEEX',
    'LIBSVTAV1','LIBTHEORA','LIBTWOLAME','LIBUAVS3D','LIBVO_AMRWBENC',
    'LIBVORBIS','LIBVPX','LIBWEBP','LIBX264','LIBX265',
    'LIBXAVS','LIBXAVS2','LIBXEVD','LIBXEVE','LIBXVID','LIBZVBI',
    'LIBSSH','LIBRTMP','LIBSRT','LIBSMBCLIENT','LIBZMQ',
    'LIBPLACEBO','LIBVMAF',
    'MEDIAFOUNDATION',
    'NVENC','NVDEC','CUVID',
    'QSV', 'MFX',
    'AMF',
    'V4L2_M2M',
    'VAAPI','VDPAU','VIDEOTOOLBOX',
    'CHROMAPRINT','SUBTITLES_FILTER',
    'GNUTLS','GCRYPT'
)

$changed = 0
foreach ($p in $prefixes) {
    # CONFIG_<P>_<anything> 1 -> 0
    $pattern = "(?m)^(\s*#\s*define\s+CONFIG_${p}[A-Z0-9_]*)\s+1\s*$"
    $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern, '$1 0')
    if ($newContent -ne $content) {
        $diff = ([regex]::Matches($newContent, "(?m)^\s*#\s*define\s+CONFIG_${p}[A-Z0-9_]*\s+0\s*$")).Count - ([regex]::Matches($content, "(?m)^\s*#\s*define\s+CONFIG_${p}[A-Z0-9_]*\s+0\s*$")).Count
        $changed += $diff
        $content = $newContent
    }
}

Set-Content -LiteralPath $ConfigComponentsPath -Value $content -NoNewline -Encoding ASCII
Write-Host "Disabled $changed components"
