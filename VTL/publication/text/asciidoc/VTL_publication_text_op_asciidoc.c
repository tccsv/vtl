#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_compat.h>


VTL_AppResult VTL_asciidoc_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_asciidoc_MarkerList markers;
    VTL_asciidoc_MarkerListInit(&markers);

    VTL_AppResult res = VTL_asciidoc_ParseDocumentSequential(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_asciidoc_ConvertToMarkedText(src, &markers, out);
    }
    VTL_asciidoc_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_asciidoc_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_asciidoc_MarkerList markers;
    VTL_asciidoc_MarkerListInit(&markers);

    VTL_AppResult res = VTL_asciidoc_ParseDocumentParallel(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_asciidoc_ConvertToMarkedText(src, &markers, out);
    }
    VTL_asciidoc_MarkerListFree(&markers);
    return res;
}


// CLOCK_MONOTONIC даёт wall-clock — это единственный правильный таймер для параллельного кода
// clock() показывает CPU-time и при N потоках выглядел бы как N-кратный рост, а не как ускорение
static double VTL_asciidoc_now_seconds(void)
{
    return vtl_monotonic_seconds();
}

double VTL_asciidoc_BenchSequential(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = VTL_asciidoc_now_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_asciidoc_MarkerList markers;
        VTL_asciidoc_MarkerListInit(&markers);
        VTL_asciidoc_ParseDocumentSequential(src, &markers);
        VTL_asciidoc_MarkerListFree(&markers);
    }
    return VTL_asciidoc_now_seconds() - start;
}

double VTL_asciidoc_BenchParallel(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = VTL_asciidoc_now_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_asciidoc_MarkerList markers;
        VTL_asciidoc_MarkerListInit(&markers);
        VTL_asciidoc_ParseDocumentParallel(src, &markers);
        VTL_asciidoc_MarkerListFree(&markers);
    }
    return VTL_asciidoc_now_seconds() - start;
}
