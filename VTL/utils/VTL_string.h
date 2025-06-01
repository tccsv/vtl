#ifndef _VTL_STRING_H
#define _VTL_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif


#define VTL_string_MaxLength 1024

typedef char VTL_char;

#define VTL_string_size (VTL_publication_string_max_length*sizeof(VTL_publication_char))

typedef VTL_char VTL_string[VTL_publication_string_MaxLength];


#ifdef __cplusplus
}
#endif


#endif
