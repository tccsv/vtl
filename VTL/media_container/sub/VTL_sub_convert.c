#include <VTL/media_container/sub/VTL_sub_convert.h>
#include <VTL/media_container/sub/VTL_sub_style.h>
#include <VTL/media_container/sub/VTL_sub_parse.h>
#include <VTL/media_container/sub/infra/VTL_sub_read.h>
#include <VTL/media_container/sub/infra/VTL_sub_write.h>
#include <stdio.h>
#include <VTL/media_container/sub/VTL_sub_data.h>

// Форматтер времени
static VTL_AppResult VTL_sub_ConvertFormatTime(char* buf, size_t bufsz, double t, VTL_sub_Format format) {
    return VTL_sub_FormatSubTime(t, buf, bufsz, format);
}

// Преобразование цвета
// color_to_str: преобразует ARGB (0xAARRGGBB) в формат ASS (&HAABBGGRR)
// Для данной libass: Alpha в ASS инвертирована по сравнению с ARGB.
// ASS Alpha 0x00 = непрозрачный (соответствует ARGB Alpha 0xFF)
// ASS Alpha 0xFF = прозрачный   (соответствует ARGB Alpha 0x00)
static VTL_AppResult VTL_sub_ConvertColorToStr(uint32_t argb, char* buf, size_t bufsz, VTL_sub_Format format) {
    if (format == VTL_sub_format_kASS) {
        return VTL_sub_ArgbToAssStr(argb, buf, bufsz);
    } else {
        if (!buf || bufsz < 8) return VTL_res_kArgumentError;
        int n = snprintf(buf, bufsz, "#%02X%02X%02X", (argb>>16)&0xFF, (argb>>8)&0xFF, (argb)&0xFF);
        if (n < 0 || (size_t)n >= bufsz) return VTL_res_kSubtitleTextOverflow;
        return VTL_res_kOk;
    }
}

// Конвертирует файл субтитров из входного файла в выходной, применяя параметры оформления.
// Если style_params равен NULL, используются стили по умолчанию.
VTL_AppResult VTL_sub_ConvertWithStyle(const char* input_file, VTL_sub_Format input_format, 
                                       const char* output_file, VTL_sub_Format output_format, 
                                       const VTL_sub_StyleParams* style_params) {
    VTL_sub_ReadSource* src = NULL;
    VTL_AppResult res = VTL_res_kOk;
    res = VTL_sub_ReadOpenSource(input_file, &src);
    if (res != VTL_res_kOk) {
        return VTL_res_convert_kOpenInputFileError;
    }
    VTL_sub_ReadMeta meta;
    res = VTL_sub_ReadMetaData(src, &meta);
    if (res != VTL_res_kOk) {
        VTL_sub_ReadCloseSource(&src);
        return VTL_res_convert_kReadMetaError;
    }

    VTL_sub_WriteMeta wmeta = { .format = output_format };
    VTL_sub_WriteSink* sink = NULL;
    res = VTL_sub_WriteOpenSink(output_file, output_format, &sink, style_params);
    if (res != VTL_res_kOk) {
        VTL_sub_ReadCloseSource(&src);
        return VTL_res_convert_kOpenOutputFileError;
    }

    VTL_sub_Entry entry;
    while (VTL_sub_ReadPart(src, &entry) == VTL_res_kOk) {
        if (style_params && (output_format == VTL_sub_format_kSRT || output_format == VTL_sub_format_kVTT)) {
            VTL_sub_Entry styled_entry = entry;
            char styled_text[2048];
            if (output_format == VTL_sub_format_kSRT) {
                char html_color[16];
                snprintf(html_color, sizeof(html_color), "#%02X%02X%02X",
                    (style_params->primary_color>>16)&0xFF,
                    (style_params->primary_color>>8)&0xFF,
                    (style_params->primary_color)&0xFF);
                snprintf(styled_text, sizeof(styled_text),
                    "<font color=\"%s\" size=\"%d\" face=\"%s\">%s</font>",
                    html_color,
                    style_params->fontsize > 0 ? style_params->fontsize : 24,
                    style_params->fontname ? style_params->fontname : "Arial",
                    entry.text ? entry.text : "");
            } else if (output_format == VTL_sub_format_kVTT) {
                snprintf(styled_text, sizeof(styled_text), "%s", entry.text ? entry.text : "");
            }
            if (styled_entry.text) free(styled_entry.text);
            styled_entry.text = (char*)malloc(strlen(styled_text) + 1);
            if (styled_entry.text) strcpy(styled_entry.text, styled_text);
            if (!styled_entry.text) {
                VTL_sub_ReadCloseSource(&src);
                VTL_sub_WriteCloseSink(&sink);
                return VTL_res_convert_kAllocError;
            }
            res = VTL_sub_WritePart(sink, &styled_entry);
            if (res != VTL_res_kOk) {
                VTL_sub_ReadCloseSource(&src);
                VTL_sub_WriteCloseSink(&sink);
                return VTL_res_convert_kWritePartError;
            }
        } else {
            res = VTL_sub_WritePart(sink, &entry);
            if (res != VTL_res_kOk) {
                VTL_sub_ReadCloseSource(&src);
                VTL_sub_WriteCloseSink(&sink);
                return VTL_res_convert_kWritePartError;
            }
        }
        if (entry.text) free(entry.text);
        if (entry.style) free(entry.style);
    }
    if (src) VTL_sub_ReadCloseSource(&src);
    if (sink) VTL_sub_WriteCloseSink(&sink);
    return VTL_res_kOk;
}