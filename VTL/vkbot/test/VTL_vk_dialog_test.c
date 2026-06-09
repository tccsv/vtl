/* Тесты разбора и состояния VK-бота: площадки, разметка, диалоги. Без сети. */
#include <VTL/vkbot/VTL_vk_dialog.h>
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

int main(void)
{
    VTL_RUN_TEST(targets_parse);
    VTL_RUN_TEST(markup_parse);
    VTL_RUN_TEST(markup_label);
    VTL_RUN_TEST(targets_label);
    VTL_RUN_TEST(dialog_defaults_and_identity);
    return VTL_test_result();
}
