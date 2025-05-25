#ifndef _VTL_STRING_H
#define _VTL_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif


#define VTL_publication_string_max_length 1024

typedef char VTL_publication_char;

#define VTL_publication_string_size (VTL_publication_string_max_length*sizeof(VTL_publication_char))

typedef VTL_publication_char VTL_publication_string[VTL_publication_string_max_length];


#ifdef __cplusplus
}
#endif


#endif