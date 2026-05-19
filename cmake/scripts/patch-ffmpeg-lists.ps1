param(
    [Parameter(Mandatory=$true)][string]$SmpDir
)

$ErrorActionPreference = 'Stop'

# Чистит SMP/lib*/<>_list.c — удаляет строки с &ff_lib<ext>_*, &ff_qsv*,
# &ff_nvenc*, &ff_nvdec*, &ff_cuvid*, &ff_amf*, &ff_mediafoundation*,
# &ff_libplacebo*, &ff_libvmaf* и подобные ссылки на отключённые external/hw кодеки.

if (-not (Test-Path $SmpDir)) { throw "SmpDir not found: $SmpDir" }

$lists = @(
    "$SmpDir\libavcodec\codec_list.c",
    "$SmpDir\libavcodec\bsf_list.c",
    "$SmpDir\libavcodec\parser_list.c",
    "$SmpDir\libavformat\demuxer_list.c",
    "$SmpDir\libavformat\muxer_list.c",
    "$SmpDir\libavformat\protocol_list.c",
    "$SmpDir\libavfilter\filter_list.c"
)

# Префиксы symbol names которые нужно вырезать
$prefixes = @(
    'lib',
    'qsv', 'qsvdec', 'qsvenc',
    'nvenc', 'nvdec', 'cuvid',
    'amf', 'amfenc',
    'mediafoundation',
    'h264_qsv', 'h264_nvenc', 'h264_amf', 'h264_mediafoundation',
    'hevc_qsv', 'hevc_nvenc', 'hevc_amf', 'hevc_mediafoundation',
    'mpeg2_qsv', 'mpeg2_nvenc', 'mpeg2_amf',
    'vp9_qsv', 'av1_qsv', 'av1_nvenc', 'av1_amf',
    'mjpeg_qsv', 'jpeg_qsv',
    'librtmp', 'libsrt', 'libssh', 'libsmbclient', 'libzmq', 'libamqp',
    'libmodplug', 'libgme', 'bluray', 'dash', 'imf', 'chromaprint',
    'webm_dash', 'ffrtmpcrypt', 'tls',
    'pcm_bluray',
    'rtmp', 'rtmpt', 'rtmpe', 'rtmps', 'rtmpte', 'rtmpts', 'rtmphttp',
    'ffrtmphttp', 'ffrtmp',
    # filters требующие внешние deps
    'vf_libvmaf', 'vf_subtitles', 'vf_libplacebo', 'vf_zscale',
    'vf_chromaprint', 'vf_ass', 'vf_drawtext', 'vf_freezedetect',
    'vf_drawbox', 'vf_drawgrid',
    'af_rubberband', 'af_sofalizer', 'af_lv2', 'af_ladspa',
    'vf_dnn_classify', 'vf_dnn_detect', 'vf_dnn_processing',
    'asrc_flite', 'vsrc_libplacebo', 'vsrc_libvmaf',
    'avf_showcqt', 'vf_pp',
    # codec/parser entries чьи .c исключены (zlib-зависимые + png + прочие)
    'apng', 'cscd', 'dxa', 'exr', 'flashsv', 'flashsv2',
    'g2m', 'lscr', 'mscc', 'mwsc',
    'png', 'rscc', 'srgc',
    'screenpresso', 'svq3', 'tdsc', 'tiff', 'zmbv',
    'tscc', 'wcmv', 'zerocodec',
    'lcl', 'mszh', 'zlib',
    'mvha', 'pdv', 'rasc'
)

$totalRemoved = 0
foreach ($path in $lists) {
    if (-not (Test-Path $path)) { continue }
    $content = Get-Content -LiteralPath $path -Raw
    $before = $content
    foreach ($p in $prefixes) {
        # match: "&ff_<prefix><any chars>," — без обязательного _ между prefix и suffix
        $pattern = '(?m)^\s*&ff_' + [regex]::Escape($p) + '[A-Za-z0-9_]*,\s*\r?\n'
        $content = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern, '')
    }
    if ($content -ne $before) {
        Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding ASCII
        $diff = ($before.Split("`n").Count - $content.Split("`n").Count)
        $totalRemoved += $diff
        Write-Host ("patched: " + (Split-Path -Leaf $path) + " (removed $diff lines)")
    }
}

Write-Host "Total $totalRemoved entries removed across list files"
