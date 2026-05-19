#include <VTL/media_container/img/VTL_img_utils.h>
#include <sys/stat.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static const struct {
    const char* format;
    const char* description;
} supported_formats[] = {
    {"png", "Portable Network Graphics"},
    {"jpg", "JPEG Image"},
    {"jpeg", "JPEG Image"},
    {"bmp", "Bitmap Image"},
    {"tiff", "Tagged Image File Format"},
    {"webp", "WebP Image"},
    {NULL, NULL}
};

int VTL_img_CheckFileExists(const char* path)
{
    if (!path) return 0;
    return access(path, F_OK) == 0;
}

int VTL_img_GetFileSize(const char* path)
{
    if (!path) return -1;
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return st.st_size;
}

int VTL_img_IsFormatSupported(const char* format)
{
    if (!format) return 0;
    
    const char* ext = strrchr(format, '.');
    if (!ext) return 0;
    ext++;     

    for (int i = 0; supported_formats[i].format != NULL; i++) {
        if (strcasecmp(ext, supported_formats[i].format) == 0) {
            return 1;
        }
    }
    
    return 0;
}

const char* VTL_img_GetFormatDescription(const char* format)
{
    if (!format) return NULL;
    
    const char* ext = strrchr(format, '.');
    if (!ext) return "Unknown format";
    ext++;     
    for (int i = 0; supported_formats[i].format != NULL; i++) {
        if (strcasecmp(ext, supported_formats[i].format) == 0) {
            return supported_formats[i].description;
        }
    }
    
    return "Unknown format";
} 