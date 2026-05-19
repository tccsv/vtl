#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_threads.h>


// Sequential: парс в одном потоке + конвертация
VTL_AppResult VTL_telegram_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_telegram_MarkerList markers;
    VTL_telegram_MarkerListInit(&markers);

    VTL_AppResult res = VTL_telegram_ParseDocumentSequential(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_telegram_ConvertToMarkedText(src, &markers, out);
    }
    VTL_telegram_MarkerListFree(&markers);
    return res;
}

// Parallel: все сканеры в отдельных потоках, потом merge + конвертация
VTL_AppResult VTL_telegram_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_telegram_MarkerList markers;
    VTL_telegram_MarkerListInit(&markers);

    VTL_AppResult res = VTL_telegram_ParseDocumentParallel(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_telegram_ConvertToMarkedText(src, &markers, out);
    }
    VTL_telegram_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_telegram_SerializeText(const VTL_publication_MarkedText* src,
                                         VTL_publication_Text** out)
{
    return VTL_telegram_SerializeFromMarkedText(src, out);
}


// ============================================================
// бенчмарки
// измеряется только парсинг (без конвертации в MarkedText), потому что
// именно парсинг — единственное, что распараллелено
// ============================================================

double VTL_telegram_BenchSequential(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = vtl_tg_monotonic_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_telegram_MarkerList markers;
        VTL_telegram_MarkerListInit(&markers);
        VTL_telegram_ParseDocumentSequential(src, &markers);
        VTL_telegram_MarkerListFree(&markers);
    }
    return vtl_tg_monotonic_seconds() - start;
}

double VTL_telegram_BenchParallel(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = vtl_tg_monotonic_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_telegram_MarkerList markers;
        VTL_telegram_MarkerListInit(&markers);
        VTL_telegram_ParseDocumentParallel(src, &markers);
        VTL_telegram_MarkerListFree(&markers);
    }
    return vtl_tg_monotonic_seconds() - start;
}
