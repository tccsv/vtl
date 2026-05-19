#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/asciidoc/infra/VTL_publication_text_op_asciidoc_load.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_compat.h>

#include <stdio.h>
#include <stdlib.h>


// читает файл целиком в malloc-буфер
static VTL_AppResult VTL_asciidoc_load_file(const char* path, char** out_buf, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    if (!f) return VTL_res_kFileOpenErr;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return VTL_res_kFileReadErr; }
    long size = ftell(f);
    if (size < 0)                  { fclose(f); return VTL_res_kFileReadErr; }
    rewind(f);

    char* buf = (char*)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return VTL_res_kMemAllocErr; }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) { free(buf); return VTL_res_kFileReadErr; }
    buf[size] = '\0';

    *out_buf = buf;
    *out_len = (size_t)size;
    return VTL_res_kOk;
}

VTL_AppResult VTL_asciidoc_ParseFile(const char* path,
                                      VTL_publication_MarkedText** out)
{
    if (!path || !out) return VTL_res_kInvalidParamErr;

    char* buf = NULL;
    size_t len = 0;
    VTL_AppResult res = VTL_asciidoc_load_file(path, &buf, &len);
    if (res != VTL_res_kOk) return res;

    VTL_publication_Text src = { buf, len };
    res = VTL_asciidoc_ParseTextParallel(&src, out);
    free(buf);
    return res;
}


VTL_AppResult VTL_asciidoc_ParseFileBatchSequential(const char* const* paths,
                                                     size_t paths_count,
                                                     VTL_publication_MarkedText** out_marked)
{
    if (!paths || !out_marked) return VTL_res_kInvalidParamErr;

    for (size_t i = 0; i < paths_count; ++i) {
        out_marked[i] = NULL;
        VTL_AppResult res = VTL_asciidoc_ParseFile(paths[i], &out_marked[i]);
        if (res != VTL_res_kOk) return res;
    }
    return VTL_res_kOk;
}


// один файл = один воркер. внутри файла — sequential
// иначе на N файлов поднимется N*15 потоков и кэш CPU начнёт буксовать
typedef struct _VTL_asciidoc_BatchWorkerCtx
{
    const char* path;
    VTL_publication_MarkedText** slot;
    VTL_AppResult result;
} VTL_asciidoc_BatchWorkerCtx;

static void* VTL_asciidoc_batch_worker(void* arg)
{
    VTL_asciidoc_BatchWorkerCtx* ctx = (VTL_asciidoc_BatchWorkerCtx*)arg;

    char* buf = NULL;
    size_t len = 0;
    ctx->result = VTL_asciidoc_load_file(ctx->path, &buf, &len);
    if (ctx->result != VTL_res_kOk) return NULL;

    VTL_publication_Text src = { buf, len };
    ctx->result = VTL_asciidoc_ParseTextSequential(&src, ctx->slot);
    free(buf);
    return NULL;
}

VTL_AppResult VTL_asciidoc_ParseFileBatchParallel(const char* const* paths,
                                                   size_t paths_count,
                                                   VTL_publication_MarkedText** out_marked)
{
    if (!paths || !out_marked) return VTL_res_kInvalidParamErr;
    if (paths_count == 0) return VTL_res_kOk;

    vtl_thread_t* threads = (vtl_thread_t*)malloc(paths_count * sizeof(vtl_thread_t));
    VTL_asciidoc_BatchWorkerCtx* ctxs = (VTL_asciidoc_BatchWorkerCtx*)malloc(
        paths_count * sizeof(VTL_asciidoc_BatchWorkerCtx));
    if (!threads || !ctxs) {
        free(threads); free(ctxs);
        return VTL_res_kMemAllocErr;
    }

    for (size_t i = 0; i < paths_count; ++i) {
        out_marked[i] = NULL;
        ctxs[i].path = paths[i];
        ctxs[i].slot = &out_marked[i];
        ctxs[i].result = VTL_res_kOk;
    }

    // поток на файл
    size_t spawned = 0;
    for (size_t i = 0; i < paths_count; ++i) {
        if (vtl_thread_create(&threads[i],
                              VTL_asciidoc_batch_worker, &ctxs[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < paths_count; ++i) {
        VTL_asciidoc_batch_worker(&ctxs[i]);
    }
    for (size_t i = 0; i < spawned; ++i) {
        vtl_thread_join(threads[i]);
    }

    // вернём последний неуспех если что-то упало
    VTL_AppResult final_res = VTL_res_kOk;
    for (size_t i = 0; i < paths_count; ++i) {
        if (ctxs[i].result != VTL_res_kOk) {
            final_res = ctxs[i].result;
        }
    }

    free(threads);
    free(ctxs);
    return final_res;
}
