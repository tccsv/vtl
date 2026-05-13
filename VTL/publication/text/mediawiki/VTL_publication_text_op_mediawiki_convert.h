#ifndef _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_CONVERT_H
#define _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_CONVERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


VTL_AppResult VTL_mediawiki_SortMarkers(VTL_mediawiki_MarkerList* list);


VTL_AppResult VTL_mediawiki_MergeMarkers(const VTL_mediawiki_MarkerList* parts,
                                          size_t parts_count,
                                          VTL_mediawiki_MarkerList* out);


VTL_AppResult VTL_mediawiki_ConvertToMarkedText(const VTL_publication_Text* src,
                                                 const VTL_mediawiki_MarkerList* markers,
                                                 VTL_publication_MarkedText** out);


VTL_AppResult VTL_mediawiki_EmitFromMarkedText(const VTL_publication_MarkedText* src,
                                                VTL_publication_Text** out);


#ifdef __cplusplus
}
#endif

#endif
