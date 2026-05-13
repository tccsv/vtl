#ifndef _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_SCAN_H
#define _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_SCAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>


void* VTL_mediawiki_ScanBold(void* ctx);          // '''bold'''
void* VTL_mediawiki_ScanItalic(void* ctx);        // ''italic''
void* VTL_mediawiki_ScanStrike(void* ctx);
void* VTL_mediawiki_ScanInternalLink(void* ctx);  // [[Page]] / [[Page|Label]]
void* VTL_mediawiki_ScanExternalLink(void* ctx);  // [http://example.com label]
void* VTL_mediawiki_ScanTemplate(void* ctx);      // {{Template|arg}}
void* VTL_mediawiki_ScanNowiki(void* ctx);        // <nowiki>...</nowiki>
void* VTL_mediawiki_ScanRef(void* ctx);           // <ref>...</ref>
void* VTL_mediawiki_ScanComment(void* ctx);
void* VTL_mediawiki_ScanGenericTag(void* ctx);    // any other <tag>...</tag>


void* VTL_mediawiki_ScanHeading(void* ctx);
void* VTL_mediawiki_ScanList(void* ctx);
void* VTL_mediawiki_ScanHorizontalRule(void* ctx);


VTL_AppResult VTL_mediawiki_ParseDocumentSequential(const VTL_publication_Text* src,
                                                     VTL_mediawiki_MarkerList* out);

VTL_AppResult VTL_mediawiki_ParseDocumentParallel(const VTL_publication_Text* src,
                                                   VTL_mediawiki_MarkerList* out);


#ifdef __cplusplus
}
#endif

#endif
