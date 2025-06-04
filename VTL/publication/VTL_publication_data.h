#ifndef _VTL_PUBLICATION_DATA_H
#define _VTL_PUBLICATION_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/utils/VTL_file.h>


typedef struct _VTL_publication_image_Block
{
    VTL_Filename link;
} VTL_publication_image_Block;

typedef VTL_publication_image_Block VTL_publication_Image;

typedef struct _VTL_publication_ImageWithMarkedText
{
    VTL_publication_image_Block image;
    VTL_publication_marked_text_Block text;
} VTL_publication_ImageWithMarkedText;

typedef struct _VTL_publication_video_Block
{
    VTL_Filename link;
} VTL_publication_video_Block;

typedef VTL_publication_video_Block VTL_publication_Video;

typedef struct _VTL_publication_VideoWithMarkedText
{
    VTL_publication_video_Block video;
    VTL_publication_marked_text_Block text;
} VTL_publication_VideoWithMarkedText;



#ifdef __cplusplus
}
#endif


#endif