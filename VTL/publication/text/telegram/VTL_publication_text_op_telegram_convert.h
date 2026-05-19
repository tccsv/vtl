#ifndef _VTL_PUBLICATION_TEXT_OP_TELEGRAM_CONVERT_H
#define _VTL_PUBLICATION_TEXT_OP_TELEGRAM_CONVERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// Telegram MD → внутренний формат.
// Проходит по отсортированным маркерам и нарезает текст на части с
// флагами BOLD/ITALIC/STRIKETHROUGH. Сами токены разметки (*, _, ~)
// из выходного текста выкидываются.
VTL_AppResult VTL_telegram_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_telegram_MarkerList* markers,
                                                VTL_publication_MarkedText** out);

// Внутренний формат → Telegram MD.
// Для каждой части смотрит разницу флагов с предыдущей и дописывает
// соответствующие закрывающие/открывающие токены.
// Символы '*' '_' '~' и backslash внутри текста экранируются обратным слешем.
VTL_AppResult VTL_telegram_SerializeFromMarkedText(const VTL_publication_MarkedText* src,
                                                    VTL_publication_Text** out);


#ifdef __cplusplus
}
#endif

#endif
