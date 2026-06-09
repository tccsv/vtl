/* Тесты разбора и состояния VK-бота. */
#include <VTL/bots/vkbot/session/VTL_vk_session.h>
#include <VTL/bots/vkbot/command/VTL_vk_command.h>
#include <VTL/test/VTL_test_data.h>

#include <string.h>

TEST(targets_parse)
{
    VTL_content_platform_flags b = 0;
    VTL_ASSERT(VTL_vk_ParseTarget("vk", &b) == 1, "vk parses");
    VTL_ASSERT(b == VTL_CONTENT_PLATFORM_VK, "vk -> VK bit");
    VTL_ASSERT(VTL_vk_ParseTarget("Reddit", &b) == 1, "case-insensitive");
    VTL_ASSERT(b == VTL_CONTENT_PLATFORM_R, "reddit -> R bit");
    VTL_ASSERT(VTL_vk_ParseTarget("zzz", &b) == 0, "unknown rejected");
}

TEST(markup_parse)
{
    VTL_publication_marked_text_MarkupType m = VTL_markup_type_kStandartMD;
    VTL_ASSERT(VTL_vk_ParseMarkup("html", &m) == 1 && m == VTL_markup_type_kHTML, "html");
    VTL_ASSERT(VTL_vk_ParseMarkup("asciidoc", &m) == 1 && m == VTL_markup_type_kAsciiDoc, "asciidoc");
    VTL_ASSERT(VTL_vk_ParseMarkup("nope", &m) == 0, "unknown rejected");
}

TEST(markup_label)
{
    VTL_ASSERT(strcmp(VTL_vk_MarkupLabel(VTL_markup_type_kMediaWiki), "MediaWiki") == 0, "label mw");
}

TEST(targets_label)
{
    char out[128];
    VTL_vk_TargetsLabel(VTL_CONTENT_PLATFORM_VK | VTL_CONTENT_PLATFORM_W, out, sizeof(out));
    VTL_ASSERT(strstr(out, "VK") && strstr(out, "W"), "lists both");
    VTL_vk_TargetsLabel(0, out, sizeof(out));
    VTL_ASSERT(strcmp(out, "ничего") == 0, "empty label");
}

TEST(dialog_defaults_and_identity)
{
    VTL_vk_Hub hub;
    VTL_vk_hub_Reset(&hub, NULL, NULL, NULL);
    VTL_vk_Dialog* a = VTL_vk_hub_Dialog(&hub, 42);
    VTL_ASSERT(a != NULL, "dialog created");
    VTL_ASSERT((a->targets & VTL_CONTENT_PLATFORM_VK) != 0, "default target VK");
    VTL_ASSERT(a->markup == VTL_markup_type_kStandartMD, "default markup md");
    VTL_ASSERT(strcmp(a->source, "text.md") == 0, "default source");
    VTL_ASSERT(VTL_vk_hub_Dialog(&hub, 42) == a, "same peer -> same dialog");
    VTL_ASSERT(VTL_vk_hub_Dialog(&hub, 99) != a, "other peer -> other dialog");
}

/* Перехват ответов бота. */
typedef struct vk_sink { char last[512]; int count; } vk_sink;
static void vk_sink_emit(void* ud, long long peer_id, const char* text)
{
    vk_sink* s = (vk_sink*)ud;
    (void)peer_id;
    snprintf(s->last, sizeof(s->last), "%s", text);
    ++s->count;
}

/* Фейковый публикатор. */
static int                                    vk_pub_calls;
static VTL_content_platform_flags             vk_pub_flags;
static VTL_publication_marked_text_MarkupType vk_pub_markup;
static VTL_AppResult vk_pub_text(const VTL_Filename file,
                                 const VTL_content_platform_flags flags,
                                 const VTL_publication_marked_text_MarkupType markup)
{
    (void)file;
    ++vk_pub_calls;
    vk_pub_flags  = flags;
    vk_pub_markup = markup;
    return VTL_res_kOk;
}

/* /targets -> /markup -> /source -> /post. */
TEST(vk_sequence_select_then_post)
{
    const char* path = "vtl_vk_seq_text.md";
    FILE* f = fopen(path, "wb");
    VTL_ASSERT(f != NULL, "temp source file created");
    if (f) { fputs("# hi\n", f); fclose(f); }

    vk_sink sink = { {0}, 0 };
    VTL_vk_Publishers pub = { vk_pub_text, NULL };
    vk_pub_calls = 0;

    VTL_vk_Hub hub;
    VTL_vk_hub_Reset(&hub, &pub, vk_sink_emit, &sink);

    const long long peer = 555;
    VTL_vk_Dialog* d = VTL_vk_hub_Dialog(&hub, peer);

    VTL_ASSERT(d->targets == VTL_CONTENT_PLATFORM_VK, "step0: default target VK");
    VTL_ASSERT(d->markup == VTL_markup_type_kStandartMD, "step0: default markup md");

    VTL_vk_hub_Handle(&hub, peer, "/targets vk w");
    VTL_ASSERT((d->targets & VTL_CONTENT_PLATFORM_VK) &&
               (d->targets & VTL_CONTENT_PLATFORM_W), "step1: targets VK+W");
    VTL_ASSERT(strstr(sink.last, "Ок, площадки") != NULL, "step1: reply confirms targets");

    VTL_vk_hub_Handle(&hub, peer, "/markup asciidoc");
    VTL_ASSERT(d->markup == VTL_markup_type_kAsciiDoc, "step2: markup AsciiDoc");

    {
        char msg[128];
        snprintf(msg, sizeof(msg), "/source %s", path);
        VTL_vk_hub_Handle(&hub, peer, msg);
    }
    VTL_ASSERT(strcmp(d->source, path) == 0, "step3: source path stored");

    VTL_vk_hub_Handle(&hub, peer, "/post");
    VTL_ASSERT(vk_pub_calls == 1, "step4: publisher called once");
    VTL_ASSERT((vk_pub_flags & VTL_CONTENT_PLATFORM_VK) &&
               (vk_pub_flags & VTL_CONTENT_PLATFORM_W), "step4: flags from /targets");
    VTL_ASSERT(vk_pub_markup == VTL_markup_type_kAsciiDoc, "step4: markup from /markup");
    VTL_ASSERT(strstr(sink.last, "опубликовано") != NULL, "step4: reply says published");

    remove(path);
}

/* Файл не существует — /post не зовёт публикатор. */
TEST(vk_sequence_post_without_file)
{
    vk_sink sink = { {0}, 0 };
    VTL_vk_Publishers pub = { vk_pub_text, NULL };
    vk_pub_calls = 0;

    VTL_vk_Hub hub;
    VTL_vk_hub_Reset(&hub, &pub, vk_sink_emit, &sink);

    const long long peer = 777;

    VTL_vk_hub_Handle(&hub, peer, "/targets vk");
    VTL_vk_hub_Handle(&hub, peer, "/source no_such_file_42.md");
    VTL_vk_hub_Handle(&hub, peer, "/post");

    VTL_ASSERT(vk_pub_calls == 0, "publisher NOT called on missing file");
    VTL_ASSERT(strstr(sink.last, "Нет файла") != NULL, "reply reports missing file");
}

/* Изоляция диалогов. */
TEST(vk_sequence_peers_isolated)
{
    VTL_vk_Hub hub;
    VTL_vk_hub_Reset(&hub, NULL, NULL, NULL);

    VTL_vk_hub_Handle(&hub, 1, "/markup html");
    VTL_vk_hub_Handle(&hub, 2, "/markup bb");

    VTL_ASSERT(VTL_vk_hub_Dialog(&hub, 1)->markup == VTL_markup_type_kHTML, "peer1 -> HTML");
    VTL_ASSERT(VTL_vk_hub_Dialog(&hub, 2)->markup == VTL_markup_type_kBB, "peer2 -> BB");
}

/* Токен: задать -> HasToken, сброс -> нет. */
TEST(token_set_clear)
{
    VTL_vk_Dialog d;
    memset(&d, 0, sizeof(d));
    VTL_ASSERT(VTL_vk_session_HasToken(&d) == 0, "no token by default");
    VTL_vk_session_SetToken(&d, "abc123");
    VTL_ASSERT(VTL_vk_session_HasToken(&d) == 1, "token set -> has");
    VTL_ASSERT(strcmp(d.token, "abc123") == 0, "token stored");
    VTL_vk_session_ClearToken(&d);
    VTL_ASSERT(VTL_vk_session_HasToken(&d) == 0, "token cleared -> none");
}

/* Справки непустые и содержат ключевые имена. */
TEST(helps_list_known)
{
    VTL_ASSERT(strstr(VTL_vk_TargetsHelp(), "vk") != NULL, "targets help lists vk");
    VTL_ASSERT(strstr(VTL_vk_TargetsHelp(), "tg") != NULL, "targets help lists tg");
    VTL_ASSERT(strstr(VTL_vk_MarkupsHelp(), "asciidoc") != NULL, "markups help lists asciidoc");
    VTL_ASSERT(strstr(VTL_vk_MarkupsHelp(), "mediawiki") != NULL, "markups help lists mediawiki");
}

int main(void)
{
    VTL_RUN_TEST(targets_parse);
    VTL_RUN_TEST(markup_parse);
    VTL_RUN_TEST(markup_label);
    VTL_RUN_TEST(targets_label);
    VTL_RUN_TEST(dialog_defaults_and_identity);
    VTL_RUN_TEST(vk_sequence_select_then_post);
    VTL_RUN_TEST(vk_sequence_post_without_file);
    VTL_RUN_TEST(vk_sequence_peers_isolated);
    VTL_RUN_TEST(token_set_clear);
    VTL_RUN_TEST(helps_list_known);
    return VTL_test_result();
}
