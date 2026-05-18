#include <VTL_vencode.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>

static VTL_vencode_Config g_vencode_cfg = {0};

VTL_AppResult VTL_vencode_Init(const VTL_vencode_Config* cfg)
{
  if (!cfg) return VTL_res_kInvalidParamErr;
  g_vencode_cfg = *cfg;
  av_log_set_level(cfg->log_level * 8);
  avformat_network_init();
  return VTL_res_kOk;
}

VTL_vencode_Flags VTL_vencode_GetFlags(void)
{
  return g_vencode_cfg.flags;
}

int VTL_vencode_GetThreadCount(void)
{
  return g_vencode_cfg.thread_count;
}

void VTL_vencode_Deinit(void)
{
  avformat_network_deinit();
}
