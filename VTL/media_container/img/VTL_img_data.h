#ifndef _VTL_IMG_DATA_H
#define _VTL_IMG_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>

// Определение флага для сохранения ссылки на кадр
#define AV_BUFFERSRC_FLAG_KEEP_REF 0x0001

// Определение кодов результата
#define VTL_res_kOk 0
#define VTL_res_video_fs_r_kMissingFileErr -2

typedef enum {
    VTL_IMG_SUCCESS = 0,
    VTL_IMG_ERROR_INVALID_PARAM = -1,
    VTL_IMG_ERROR_FILE_NOT_FOUND = -2,
    VTL_IMG_ERROR_FILE_ACCESS = -3,
    VTL_IMG_ERROR_FORMAT_NOT_SUPPORTED = -4,
    VTL_IMG_ERROR_DECODE = -5,
    VTL_IMG_ERROR_ENCODE = -6,
    VTL_IMG_ERROR_MEMORY = -7,
    VTL_IMG_ERROR_FILTER = -8
} VTL_AppResult;

typedef struct {
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVFilterGraph* filter_graph;
    AVFilterContext* buffersrc_ctx;
    AVFilterContext* buffersink_ctx;
    struct SwsContext* sws_ctx;
    AVFrame* current_frame;
} VTL_ImageContext;

typedef struct {
    const char* name;
    const char* description;
    const char* filter_desc;
    int (*apply)(AVFrame* frame);
} VTL_ImageFilter;

typedef VTL_ImageContext VTL_img_data_t;

// Функции для работы с изображениями
VTL_ImageContext* VTL_img_context_Init(void);
void VTL_img_context_Cleanup(VTL_ImageContext* ctx);
VTL_AppResult VTL_img_LoadImage(const char* path, VTL_ImageContext* ctx);
VTL_AppResult VTL_img_SaveImage(const char* path, VTL_ImageContext* ctx);
VTL_AppResult VTL_img_ApplyFilter(VTL_ImageContext* ctx, const VTL_ImageFilter* filter);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_DATA_H