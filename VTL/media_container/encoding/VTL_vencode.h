#ifndef VTL_VENCODE_H
#define VTL_VENCODE_H

#include <stddef.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>

/* Using VTL common result type */
#include <VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  VTL_vencode_codec_kH264,
  VTL_vencode_codec_kH265,
  VTL_vencode_codec_kAV1,
} VTL_vencode_Codec;

typedef enum {
  VTL_vencode_api_kCPU,
  VTL_vencode_api_kVulkan,
  VTL_vencode_api_kHybrid,
} VTL_vencode_Api;

typedef enum {
  VTL_VENCODE_FLAG_NONE = 0,
  VTL_VENCODE_FLAG_HYBRID_FORCE_GPU_PREPROC = 1 << 0,
  VTL_VENCODE_FLAG_TG_OPTIMAL = 1 << 1,
  VTL_VENCODE_FLAG_YT_OPTIMAL = 1 << 2,
  VTL_VENCODE_FLAG_BILI_BILI_OPTIMAL = 1 << 3,
  VTL_VENCODE_FLAG_VK_OPTIMAL = 1 << 4,
} VTL_vencode_Flags;

typedef enum {
  VTL_vencode_pixfmt_kYUV420P,
  VTL_vencode_pixfmt_kYUV444P,
} VTL_vencode_PixFmt;

typedef struct {
  int log_level;
  int thread_count; /* 0 for auto */
  VTL_vencode_Flags flags;
} VTL_vencode_Config;

typedef struct VTL_vencode_Context {
  AVFormatContext* oc;
  AVCodecContext*  enc;
  AVStream*        st;
  AVBufferRef*     hw_device;
  void*            sws_ctx;
  int              frame_count;
} VTL_vencode_Context;

VTL_AppResult VTL_vencode_Init(const VTL_vencode_Config* cfg);

VTL_vencode_Context* VTL_vencode_Open(VTL_vencode_Codec codec, 
                                     VTL_vencode_Api api, 
                                     const char* outfile);

VTL_AppResult VTL_vencode_EncodeFrame(VTL_vencode_Context* ctx,
                                     const uint8_t* data,
                                     size_t size,
                                     uint32_t w,
                                     uint32_t h,
                                     VTL_vencode_PixFmt fmt);

VTL_AppResult VTL_vencode_EncodeAVFrame(VTL_vencode_Context* ctx,
                                       AVFrame* frame);

VTL_AppResult VTL_vencode_Close(VTL_vencode_Context* ctx);

VTL_AppResult VTL_vencode_TranscodeFile(const char* input_file,
                                       const char* output_file,
                                       VTL_vencode_Codec codec,
                                       VTL_vencode_Api api);

VTL_AppResult VTL_vencode_PrepareForTG(const char* input_file,
                                      const char* output_file);

void VTL_vencode_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* VTL_VENCODE_H */
