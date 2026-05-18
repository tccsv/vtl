#include <VTL_vencode.h>

extern VTL_vencode_Context* VTL_vencode_OpenCPU(VTL_vencode_Codec, const char*);
extern VTL_vencode_Context* VTL_vencode_OpenVulkan(VTL_vencode_Codec, const char*);

extern VTL_AppResult VTL_vencode_EncodeFrameCPU(VTL_vencode_Context*, const uint8_t*, size_t, uint32_t, uint32_t);
extern VTL_AppResult VTL_vencode_EncodeFrameVulkan(VTL_vencode_Context*, const uint8_t*, size_t, uint32_t, uint32_t);

extern VTL_AppResult VTL_vencode_CloseCPU(VTL_vencode_Context*);
extern VTL_AppResult VTL_vencode_CloseVulkan(VTL_vencode_Context*);

VTL_vencode_Context* VTL_vencode_Open(VTL_vencode_Codec codec, VTL_vencode_Api api, const char* outfile)
{
  if (api == VTL_vencode_api_kCPU || api == VTL_vencode_api_kHybrid) {
      return VTL_vencode_OpenCPU(codec, outfile);
  } else if (api == VTL_vencode_api_kVulkan) {
      return VTL_vencode_OpenVulkan(codec, outfile);
  }
  return NULL;
}
VTL_AppResult VTL_vencode_EncodeFrame(VTL_vencode_Context* ctx,
                                     const uint8_t* data,
                                     size_t size,
                                     uint32_t w,
                                     uint32_t h, VTL_vencode_PixFmt fmt)
{
  if (!ctx) return VTL_res_kNullArgument;

  /* Hybrid and CPU use CPU encoding paths, Vulkan uses GPU path */
  return (ctx->enc->pix_fmt == AV_PIX_FMT_VULKAN)
           ? VTL_vencode_EncodeFrameVulkan(ctx, data, size, w, h)
           : VTL_vencode_EncodeFrameCPU(ctx, data, size, w, h);
}

VTL_AppResult VTL_vencode_EncodeAVFrame(VTL_vencode_Context* ctx,
                                       AVFrame* frame)
{
  if (!ctx || !frame) return VTL_res_kNullArgument;

  return (ctx->enc->pix_fmt == AV_PIX_FMT_VULKAN)
           ? VTL_vencode_EncodeAVFrameVulkan(ctx, frame)
           : VTL_vencode_EncodeAVFrameCPU(ctx, frame);
}

VTL_AppResult VTL_vencode_Close(VTL_vencode_Context* ctx)
{
  if (!ctx) return VTL_res_kOk;

  return (ctx->enc->pix_fmt == AV_PIX_FMT_VULKAN)
           ? VTL_vencode_CloseVulkan(ctx)
           : VTL_vencode_CloseCPU(ctx);
}

VTL_AppResult VTL_vencode_PrepareForTG(const char* input_file, const char* output_file)
{
  VTL_vencode_Config cfg = { .log_level = 1, .flags = VTL_VENCODE_FLAG_TG_OPTIMAL };
  VTL_vencode_Init(&cfg);
  
  VTL_AppResult res = VTL_vencode_TranscodeFile(input_file, output_file, VTL_vencode_codec_kH264, VTL_vencode_api_kCPU);
  
  return res;
}

VTL_AppResult VTL_vencode_PrepareForYT(const char* input_file, const char* output_file)
{
  VTL_vencode_Config cfg = { .log_level = 1, .flags = VTL_VENCODE_FLAG_YT_OPTIMAL };
  VTL_vencode_Init(&cfg);
  
  VTL_AppResult res = VTL_vencode_TranscodeFile(input_file, output_file, VTL_vencode_codec_kH265, VTL_vencode_api_kCPU);
  
  return res;
}

VTL_AppResult VTL_vencode_PrepareForBiliBili(const char* input_file, const char* output_file)
{
  VTL_vencode_Config cfg = { .log_level = 1, .flags = VTL_VENCODE_FLAG_BILI_BILI_OPTIMAL };
  VTL_vencode_Init(&cfg);
  
  VTL_AppResult res = VTL_vencode_TranscodeFile(input_file, output_file, VTL_vencode_codec_kH265, VTL_vencode_api_kCPU);
  
  return res;
}

VTL_AppResult VTL_vencode_PrepareForVK(const char* input_file, const char* output_file)
{
  VTL_vencode_Config cfg = { .log_level = 1, .flags = VTL_VENCODE_FLAG_VK_OPTIMAL };
  VTL_vencode_Init(&cfg);
  
  VTL_AppResult res = VTL_vencode_TranscodeFile(input_file, output_file, VTL_vencode_codec_kH264, VTL_vencode_api_kCPU);
  
  return res;
}