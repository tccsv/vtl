param(
    [Parameter(Mandatory=$true)][string]$SmpDir
)

$ErrorActionPreference = 'Stop'

# Убирает .c файлы которые ссылаются на отключённые внешние SDK
# (Intel Media SDK, NVIDIA Codec SDK, libmp3lame, libx264, ...).
# Все эти файлы в FFmpeg включают <external.h> безусловно и не собираются
# без внешних либ.

if (-not (Test-Path $SmpDir)) { throw "SmpDir not found: $SmpDir" }

# Hardware acceleration wrappers (требуют CUDA / Intel Media SDK)
$exclude = @(
    'hwcontext_qsv_wrap.c',
    'hwcontext_cuda_wrap.c'
)

# Resampler wrappers
$exclude += @(
    'soxr_resample.c'
)

# libavcodec — wrappers вокруг внешних кодеков
$exclude += @(
    'libaomdec.c', 'libaomenc.c',
    'libaribb24.c', 'libaribcaption.c',
    'libcelt_dec.c',
    'libcodec2.c',
    'libdav1d.c',
    'libfdk-aacdec.c', 'libfdk-aacenc.c',
    'libgsmdec.c', 'libgsmenc.c',
    'libilbc.c',
    'libjxl.c', 'libjxldec.c', 'libjxlenc.c',
    'libkvazaar.c',
    'libmp3lame.c',
    'libopencore-amrnb.c', 'libopencore-amrwb.c',
    'libopenh264.c', 'libopenh264dec.c', 'libopenh264enc.c',
    'libopenjpegdec.c', 'libopenjpegenc.c',
    'libopus.c', 'libopusdec.c', 'libopusenc.c',
    'librav1e.c',
    'libshine.c',
    'libspeexdec.c', 'libspeexenc.c',
    'libsvtav1.c',
    'libtheora.c', 'libtheoraenc.c',
    'libtwolame.c',
    'libuavs3d.c',
    'libvo-amrwbenc.c',
    'libvorbisdec.c', 'libvorbisenc.c',
    'libvpxdec.c', 'libvpxenc.c',
    'libwebpenc.c', 'libwebpenc_animencoder.c', 'libwebpenc_common.c',
    'libx264.c',
    'libx265.c',
    'libxavs.c', 'libxavs2.c',
    'libxevd.c', 'libxeve.c',
    'libxvid.c',
    'libzvbi-teletextdec.c',
    'mediafoundation.c', 'mediafoundationenc.c',
    'libplacebo.c'
)

# libavformat — внешние демуксеры/муксеры
$exclude += @(
    'librtmp.c',
    'librist.c',
    'libsrt.c',
    'libssh.c',
    'libsmbclient.c',
    'libzmq.c',
    'libamqp.c',
    'libtls.c',
    'tls_gnutls.c',
    'tls_libtls.c',
    'tls_openssl.c',
    'libmodplug.c',
    'libgme.c',
    'bluray.c',
    'dashdec.c', 'dashenc.c',
    'imfdec.c', 'imf_cpl.c',
    'rtmpcrypt.c', 'rtmpdh.c', 'rtmpproto.c', 'rtmphttp.c', 'rtmpe.c',
    # zlib-зависимые (uncompress/compress/inflate/deflate без CONFIG_ZLIB защиты)
    'zlib_wrapper.c',
    'dxa.c', 'exr.c', 'exrenc.c', 'rscc.c', 'mscc.c',
    'flashsv.c', 'flashsvenc.c', 'flashsv2enc.c',
    'g2meet.c', 'lscrdec.c', 'cscd.c', 'tiffenc.c',
    'screenpresso.c', 'tdsc.c', 'svq3.c', 'tiff.c',
    'zmbvenc.c', 'zmbv.c',
    'pngdec.c', 'pngenc.c', 'png_parser.c', 'lscrdec.c',
    'pngdsp.c', 'pngdsp_init.c',
    'mwsc.c',
    'tscc.c', 'wcmv.c', 'zerocodec.c',
    'lcldec.c', 'lclenc.c',
    'mvha.c', 'pdvdec.c', 'rasc.c',
    'chromaprint.c'
)

# libavfilter — внешние фильтры
$exclude += @(
    'vf_libvmaf.c',
    'vf_subtitles.c',
    'vf_libplacebo.c',
    'vf_zscale.c',
    'vf_chromaprint.c',
    'vf_ass.c',
    'vf_drawtext.c',
    'vf_freezedetect.c',
    'af_rubberband.c',
    'af_sofalizer.c',
    'af_lv2.c',
    'af_ladspa.c',
    'vf_dnn_classify.c', 'vf_dnn_detect.c', 'vf_dnn_processing.c',
    'asrc_flite.c',
    'vsrc_libplacebo.c', 'vsrc_libvmaf.c',
    'avf_showcqt.c', 'avf_showcqt_template.c', 'avf_showcqt_init.c',
    'vf_drawbox.c',
    'vf_pp.c'
)

$patched = 0
Get-ChildItem -Path $SmpDir -Filter '*_files.props' | ForEach-Object {
    $path = $_.FullName
    $content = Get-Content -LiteralPath $path -Raw
    $changed = $false

    foreach ($name in $exclude) {
        # Слеш (`\` или `/`) перед именем — иначе `svq3.c` матчит `rtpdec_svq3.c`.
        # 1) self-closing: <ClCompile Include="...\name" />
        $pattern1 = '(?m)^\s*<ClCompile\s+Include="[^"]*[\\/]' + [regex]::Escape($name) + '"\s*/>\s*\r?\n'
        $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern1, '')
        if ($newContent -ne $content) { $content = $newContent; $changed = $true }

        # 2) multiline block: <ClCompile Include="...\name"> ... </ClCompile>
        $pattern2 = '(?ms)^\s*<ClCompile\s+Include="[^"]*[\\/]' + [regex]::Escape($name) + '">.*?</ClCompile>\s*\r?\n'
        $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern2, '')
        if ($newContent -ne $content) { $content = $newContent; $changed = $true }

        # 3) NASM include: <NASM Include="...\name.asm" />
        $pattern3 = '(?m)^\s*<NASM\s+Include="[^"]*[\\/]' + [regex]::Escape([System.IO.Path]::GetFileNameWithoutExtension($name)) + '\.asm"\s*/>\s*\r?\n'
        $newContent = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern3, '')
        if ($newContent -ne $content) { $content = $newContent; $changed = $true }
    }

    if ($changed) {
        Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding UTF8
        $patched++
        Write-Host "patched: $($_.Name)"
    }
}

Write-Host "Patched $patched _files.props"
