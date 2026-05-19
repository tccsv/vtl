param(
    [Parameter(Mandatory=$true)][string]$ConfigPath
)

$ErrorActionPreference = 'Stop'

# Отключает все опциональные external-зависимости в config.h ShiftMediaProject.
# Меняет #define CONFIG_<LIB> 1 на 0 для всех нужных нам отключений,
# плюс HAVE_<LIB> 1 на 0.
# Идемпотентно: повторный запуск не вносит изменений (значения уже 0).

if (-not (Test-Path $ConfigPath)) { throw "config.h not found: $ConfigPath" }

# Список опциональных deps которые отключаем.
# Базовое правило: всё что требует external sdk / лицензированных компонентов.
$disableConfig = @(
    'GPL', 'VERSION3',                       # лицензии (не нужны без libx264 etc.)
    'GCRYPT', 'GMP', 'GNUTLS', 'LIBTLS',     # crypto/TLS — мы заменим на Schannel
    'LIBMP3LAME', 'LIBVORBIS', 'LIBSPEEX',
    'LIBOPUS', 'LIBILBC', 'LIBTHEORA',
    'LIBX264', 'LIBX265', 'LIBXVID', 'LIBVPX', 'LIBAOM', 'LIBSVTAV1',
    'LIBDAV1D', 'LIBVMAF', 'LIBVPL',
    'LIBGME', 'LIBMODPLUG', 'LIBSOXR',
    'LIBFREETYPE', 'FONTCONFIG', 'LIBFRIBIDI', 'LIBHARFBUZZ', 'LIBASS',
    'LIBXML2',
    'LIBSSH',
    'LIBCDIO', 'LIBCDIO_PARANOIA', 'LIBBLURAY',
    'OPENGL', 'OPENCL', 'VULKAN',
    'LIBMFX', 'QSV', 'FFNVCODEC', 'CUDA', 'CUDA_LLVM', 'CUDA_NVCC', 'CUVID', 'NVENC', 'NVDEC',
    'AMF',
    'LIBSDL2', 'SDL2',
    'LIBFDK_AAC', 'LIBOPENH264', 'LIBOPENJPEG', 'LIBWEBP',
    'LIBSHINE', 'LIBTWOLAME', 'LIBWAVPACK', 'LIBZIMG',
    'LIBRSVG', 'LIBKVAZAAR',
    'LIBJXL', 'LIBARIBB24', 'LIBARIBCAPTION',
    'LIBCACA', 'LIBCELT', 'LIBCHROMAPRINT',
    'LIBCODEC2', 'LIBDC1394', 'LIBDRM',
    'LIBFLITE', 'LIBGSM', 'LIBIEC61883',
    'LIBKLVANC', 'LIBOPENCORE_AMRNB', 'LIBOPENCORE_AMRWB',
    'LIBPLACEBO', 'LIBPULSE', 'LIBQUIRC',
    'LIBRABBITMQ', 'LIBRIST', 'LIBRTMP', 'LIBRUBBERBAND',
    'LIBSMBCLIENT', 'LIBSNAPPY', 'LIBSRT', 'LIBSSL', 'LIBOPENMPT',
    'LIBTENSORFLOW', 'LIBTESSERACT', 'LIBUAVS3D', 'LIBVIDSTAB',
    'LIBVO_AMRWBENC', 'LIBVPX_VP8', 'LIBVPX_VP9', 'LIBVVENC', 'LIBVVDEC',
    'LIBZMQ', 'LIBZVBI',
    'MEDIACODEC', 'MEDIAFOUNDATION',
    'OMX', 'OMX_RPI',
    'OPENAL', 'OSS',
    'PULSE',
    'RKMPP',
    'V4L2_M2M', 'V4L2_REQUEST',
    'VAAPI', 'VDPAU', 'VIDEOTOOLBOX',
    'XLIB', 'XVMC',
    'BZLIB',
    'LZMA',
    'ICONV'
)

$content = Get-Content -LiteralPath $ConfigPath -Raw
$changed = 0

foreach ($name in $disableConfig) {
    # CONFIG_<name> 1 -> 0  (вариант с пробелами после #, как в SMP config.h)
    $pattern1 = "(?m)^(\s*#\s*define\s+CONFIG_$name)\s+1\s*$"
    $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern1, '$1 0')
    if ($newContent -ne $content) { $changed++; $content = $newContent }

    # HAVE_<name> 1 -> 0
    $pattern2 = "(?m)^(\s*#\s*define\s+HAVE_$name)\s+1\s*$"
    $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern2, '$1 0')
    if ($newContent -ne $content) { $changed++; $content = $newContent }
}

# Включаем Schannel вместо отключённых TLS-бэкендов (мы под Windows, native)
$content = [System.Text.RegularExpressions.Regex]::Replace(
    $content,
    "(?m)^(\s*#define\s+CONFIG_SCHANNEL)\s+0\s*$",
    '$1 1'
)

Set-Content -LiteralPath $ConfigPath -Value $content -NoNewline -Encoding ASCII

Write-Host "patched $changed entries in $ConfigPath"
