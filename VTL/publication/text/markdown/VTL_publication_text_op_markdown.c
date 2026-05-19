#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown.h>
#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_threads.h>


// Sequential: парс в одном потоке + конвертация
VTL_AppResult VTL_markdown_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_markdown_MarkerList markers;
    VTL_markdown_MarkerListInit(&markers);

    VTL_AppResult res = VTL_markdown_ParseDocumentSequential(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_markdown_ConvertToMarkedText(src, &markers, out);
    }
    VTL_markdown_MarkerListFree(&markers);
    return res;
}

// Parallel: все сканеры в отдельных потоках, потом merge + конвертация
VTL_AppResult VTL_markdown_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_markdown_MarkerList markers;
    VTL_markdown_MarkerListInit(&markers);

    VTL_AppResult res = VTL_markdown_ParseDocumentParallel(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_markdown_ConvertToMarkedText(src, &markers, out);
    }
    VTL_markdown_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_markdown_SerializeText(const VTL_publication_MarkedText* src,
                                         VTL_publication_Text** out)
{
    return VTL_markdown_SerializeFromMarkedText(src, out);
}


// ============================================================
// бенчмарки
// измеряется только парсинг (без конвертации в MarkedText), потому что
// именно парсинг — единственное, что распараллелено
// ============================================================

double VTL_markdown_BenchSequential(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = vtl_md_monotonic_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_markdown_MarkerList markers;
        VTL_markdown_MarkerListInit(&markers);
        VTL_markdown_ParseDocumentSequential(src, &markers);
        VTL_markdown_MarkerListFree(&markers);
    }
    return vtl_md_monotonic_seconds() - start;
}

double VTL_markdown_BenchParallel(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = vtl_md_monotonic_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_markdown_MarkerList markers;
        VTL_markdown_MarkerListInit(&markers);
        VTL_markdown_ParseDocumentParallel(src, &markers);
        VTL_markdown_MarkerListFree(&markers);
    }
    return vtl_md_monotonic_seconds() - start;
}
