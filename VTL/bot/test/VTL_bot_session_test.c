/* Тесты сессии бота: дефолты, разбор имён платформ/форматов, выбор флагов.
 * Чистая логика — без сети и Telegram. */
#include <VTL/bot/session/VTL_bot_session.h>
#include <VTL/test/VTL_test_data.h>

#include <string.h>

#define CHAT "12345"

TEST(new_session_has_defaults)
{
    VTL_bot_SessionTable t;
    VTL_bot_session_TableInit(&t);

    VTL_bot_Session* s = VTL_bot_session_GetOrCreate(&t, CHAT);
    VTL_ASSERT(s != NULL, "session created");
    VTL_ASSERT((s->platforms & VTL_CONTENT_PLATFORM_TG) != 0, "default has TG");
    VTL_ASSERT((s->platforms & VTL_CONTENT_PLATFORM_W) != 0, "default has W");
    VTL_ASSERT(s->markup == VTL_markup_type_kTelegramMD, "default markup TelegramMD");
    VTL_ASSERT(strcmp(s->text_file, "text.md") == 0, "default file text.md");
}

TEST(get_or_create_is_stable_per_chat)
{
    VTL_bot_SessionTable t;
    VTL_bot_session_TableInit(&t);

    VTL_bot_Session* a1 = VTL_bot_session_GetOrCreate(&t, "100");
    VTL_bot_Session* a2 = VTL_bot_session_GetOrCreate(&t, "100");
    VTL_bot_Session* b  = VTL_bot_session_GetOrCreate(&t, "200");
    VTL_ASSERT(a1 == a2, "same chat returns same session");
    VTL_ASSERT(a1 != b, "different chat returns different session");
}

TEST(parse_platform_known_and_unknown)
{
    VTL_content_platform_flags bit = 0;
    VTL_ASSERT(VTL_bot_session_ParsePlatform("tg", &bit) == 1, "tg parses");
    VTL_ASSERT(bit == VTL_CONTENT_PLATFORM_TG, "tg -> TG bit");
    VTL_ASSERT(VTL_bot_session_ParsePlatform("reddit", &bit) == 1, "reddit parses");
    VTL_ASSERT(bit == VTL_CONTENT_PLATFORM_R, "reddit -> R bit");
    VTL_ASSERT(VTL_bot_session_ParsePlatform("WIKI", &bit) == 1, "case-insensitive");
    VTL_ASSERT(bit == VTL_CONTENT_PLATFORM_W, "wiki -> W bit");
    VTL_ASSERT(VTL_bot_session_ParsePlatform("nope", &bit) == 0, "unknown rejected");
}

TEST(parse_format_known_and_unknown)
{
    VTL_publication_marked_text_MarkupType m = VTL_markup_type_kStandartMD;
    VTL_ASSERT(VTL_bot_session_ParseFormat("asciidoc", &m) == 1, "asciidoc parses");
    VTL_ASSERT(m == VTL_markup_type_kAsciiDoc, "asciidoc -> enum");
    VTL_ASSERT(VTL_bot_session_ParseFormat("mediawiki", &m) == 1, "mediawiki parses");
    VTL_ASSERT(m == VTL_markup_type_kMediaWiki, "mediawiki -> enum");
    VTL_ASSERT(VTL_bot_session_ParseFormat("html", &m) == 1, "html parses");
    VTL_ASSERT(m == VTL_markup_type_kHTML, "html -> enum");
    VTL_ASSERT(VTL_bot_session_ParseFormat("xml", &m) == 0, "unknown rejected");
}

TEST(describe_platforms_lists_selected)
{
    char out[128];
    VTL_bot_session_DescribePlatforms(
        VTL_CONTENT_PLATFORM_TG | VTL_CONTENT_PLATFORM_W, out, sizeof(out));
    VTL_ASSERT(strstr(out, "TG") != NULL, "lists TG");
    VTL_ASSERT(strstr(out, "W") != NULL, "lists W");

    VTL_bot_session_DescribePlatforms(0, out, sizeof(out));
    VTL_ASSERT(strcmp(out, "—") == 0, "empty selection shows dash");
}

TEST(format_name_round_trip)
{
    VTL_ASSERT(strcmp(VTL_bot_session_FormatName(VTL_markup_type_kAsciiDoc),
                      "AsciiDoc") == 0, "asciidoc name");
    VTL_ASSERT(strcmp(VTL_bot_session_FormatName(VTL_markup_type_kMediaWiki),
                      "MediaWiki") == 0, "mediawiki name");
}

int main(void)
{
    VTL_RUN_TEST(new_session_has_defaults);
    VTL_RUN_TEST(get_or_create_is_stable_per_chat);
    VTL_RUN_TEST(parse_platform_known_and_unknown);
    VTL_RUN_TEST(parse_format_known_and_unknown);
    VTL_RUN_TEST(describe_platforms_lists_selected);
    VTL_RUN_TEST(format_name_round_trip);
    return VTL_test_result();
}
