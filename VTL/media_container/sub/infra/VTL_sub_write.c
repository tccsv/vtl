#include <VTL/media_container/sub/infra/VTL_sub_write.h>
#include <VTL/media_container/sub/infra/VTL_sub_read.h>
#include <VTL/media_container/sub/VTL_sub_style.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VTL/media_container/sub/VTL_sub_data.h>

struct VTL_sub_WriteSink {
    VTL_sub_List* p_list;
    VTL_sub_WriteMeta meta;
    char* file_path;
    const VTL_sub_StyleParams* style_params;
};

VTL_AppResult VTL_sub_WriteOpenSink(const char* file_path, VTL_sub_Format format, VTL_sub_WriteSink** pp_sink, const VTL_sub_StyleParams* style_params) {
    if (!file_path || !pp_sink) return VTL_res_kNullArgument;
    *pp_sink = NULL;
    VTL_sub_WriteSink* sink = (VTL_sub_WriteSink*)malloc(sizeof(VTL_sub_WriteSink));
    if (!sink) return VTL_res_kAllocError;
    sink->p_list = NULL;
    VTL_sub_ListCreate(&sink->p_list);
    sink->meta.format = format;
    sink->file_path = strdup(file_path);
    if (!sink->file_path) { free(sink); return VTL_res_kAllocError; }
    sink->style_params = style_params;
    *pp_sink = sink;
    return VTL_res_kOk;
}

VTL_AppResult VTL_sub_WritePart(VTL_sub_WriteSink* p_sink, const VTL_sub_Entry* p_entry) {
    if (!p_sink || !p_entry) return VTL_res_kNullArgument;
    return VTL_sub_ListAddEntry(p_sink->p_list, p_entry);
}

VTL_AppResult VTL_sub_WriteCloseSink(VTL_sub_WriteSink** pp_sink) {
    if (!pp_sink || !*pp_sink) return VTL_res_kNullArgument;
    VTL_sub_WriteSink* sink = *pp_sink;
    if (sink->p_list && sink->file_path) {
        VTL_sub_SaveToFile(sink->file_path, sink->meta.format, sink->p_list, sink->style_params);
    }
    if (sink->p_list) VTL_sub_ListDestroy(&sink->p_list);
    if (sink->file_path) free(sink->file_path);
    free(sink);
    *pp_sink = NULL;
    return VTL_res_kOk;
} 