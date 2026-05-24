#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_CONVERT_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_CONVERT_H

#include <VTL/publication/text/discord/VTL_publication_text_op_discord.h>

VTL_AppResult VTL_discord_ConvertToMarkedText(const VTL_publication_Text *src, const VTL_discord_MarkerList *markers, VTL_publication_MarkedText **out);
VTL_AppResult VTL_discord_ConvertFromMarkedText(const VTL_publication_MarkedText *src, VTL_publication_Text **out);

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_CONVERT_H */
