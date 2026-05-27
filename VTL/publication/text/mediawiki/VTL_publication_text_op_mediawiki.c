#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki.h>

#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif


VTL_AppResult VTL_mediawiki_ParseTextSequential(const VTL_publication_Text* src,
                                                 VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_mediawiki_MarkerList markers;
    VTL_mediawiki_MarkerListInit(&markers);

    VTL_AppResult res = VTL_mediawiki_ParseDocumentSequential(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_mediawiki_ConvertToMarkedText(src, &markers, out);
    }
    VTL_mediawiki_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_mediawiki_ParseTextParallel(const VTL_publication_Text* src,
                                               VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_mediawiki_MarkerList markers;
    VTL_mediawiki_MarkerListInit(&markers);

    VTL_AppResult res = VTL_mediawiki_ParseDocumentParallel(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_mediawiki_ConvertToMarkedText(src, &markers, out);
    }
    VTL_mediawiki_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_mediawiki_EmitText(const VTL_publication_MarkedText* src,
                                      VTL_publication_Text** out)
{
    return VTL_mediawiki_EmitFromMarkedText(src, out);
}


static double VTL_mediawiki_now_seconds(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

double VTL_mediawiki_BenchSequential(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = VTL_mediawiki_now_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_mediawiki_MarkerList markers;
        VTL_mediawiki_MarkerListInit(&markers);
        VTL_mediawiki_ParseDocumentSequential(src, &markers);
        VTL_mediawiki_MarkerListFree(&markers);
    }
    return VTL_mediawiki_now_seconds() - start;
}

double VTL_mediawiki_BenchParallel(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = VTL_mediawiki_now_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_mediawiki_MarkerList markers;
        VTL_mediawiki_MarkerListInit(&markers);
        VTL_mediawiki_ParseDocumentParallel(src, &markers);
        VTL_mediawiki_MarkerListFree(&markers);
    }
    return VTL_mediawiki_now_seconds() - start;
}
