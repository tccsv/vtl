
#ifndef VTL_VENCODE_VK_HELPER_H
#define VTL_VENCODE_VK_HELPER_H

#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>          
#include <VTL_vencode.h>               

#define ARRAY_LEN(a) (sizeof(a)/sizeof((a)[0]))

#define VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME "VK_KHR_video_encode_queue"
#define VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME "VK_KHR_video_encode_h264"
#define VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME "VK_KHR_video_encode_h265"
#define VK_KHR_VIDEO_ENCODE_AV1_EXTENSION_NAME "VK_KHR_video_encode_av1"
#define VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME "VK_KHR_video_decode_queue"
#define VK_QUEUE_VIDEO_ENCODE_BIT_KHR 0x00000040
#define VK_QUEUE_VIDEO_DECODE_BIT_KHR 0x00000020

typedef enum {
    VK_VIDEO_DIR_ENCODE,
    VK_VIDEO_DIR_DECODE
} vk_video_direction;

static VkBool32 has_ext(const char *name,
                        uint32_t       count,
                        const VkExtensionProperties *list)
{
    for (uint32_t i = 0; i < count; ++i)
        if (!strcmp(list[i].extensionName, name))
            return VK_TRUE;
    return VK_FALSE;
}

static VkBool32 has_queue_family(VkQueueFlagBits flag,
                                 VkPhysicalDevice pdev)
{
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdev, &n, NULL);
    VkQueueFamilyProperties *props = malloc(n * sizeof(*props));
    if (!props) return VK_FALSE;
    vkGetPhysicalDeviceQueueFamilyProperties(pdev, &n, props);

    VkBool32 found = VK_FALSE;
    for (uint32_t i = 0; i < n; ++i)
        if (props[i].queueFlags & flag) {
            found = VK_TRUE;
            break;
        }
    free(props);
    return found;
}


static VTL_AppResult vk_check_video_caps(VkPhysicalDevice pdev,
                                       VTL_vencode_Codec codec,
                                       vk_video_direction dir)
{
    uint32_t ext_cnt = 0;
    vkEnumerateDeviceExtensionProperties(pdev, NULL, &ext_cnt, NULL);
    VkExtensionProperties *ext =
        malloc(ext_cnt * sizeof(*ext));
    if (!ext) return VTL_res_kInvalidParamErr;
    vkEnumerateDeviceExtensionProperties(pdev, NULL, &ext_cnt, ext);

    const char *encode_ext = VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME;
    const char *decode_ext = VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME;

    VkBool32 ok = VK_FALSE;
    if (dir == VK_VIDEO_DIR_ENCODE) {          
        ok  = has_ext(encode_ext, ext_cnt, ext);
        ok &= has_queue_family(VK_QUEUE_VIDEO_ENCODE_BIT_KHR, pdev);
    } else {                                     
        ok  = has_ext(decode_ext, ext_cnt, ext);
        ok &= has_queue_family(VK_QUEUE_VIDEO_DECODE_BIT_KHR, pdev);
    }

    const char *profile_ext = NULL;
    switch (codec) {
    case VTL_vencode_codec_kH264: profile_ext = VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME; break;
    case VTL_vencode_codec_kH265: profile_ext = VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME; break;
    case VTL_vencode_codec_kAV1:  profile_ext = VK_KHR_VIDEO_ENCODE_AV1_EXTENSION_NAME;  break;
    }
    if (ok && profile_ext)
        ok = has_ext(profile_ext, ext_cnt, ext);

    free(ext);
    return ok ? VTL_res_kOk : VTL_res_ffmpeg_kInitError;
}

#endif
