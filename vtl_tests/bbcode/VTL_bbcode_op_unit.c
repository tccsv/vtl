#include <VTL/publication/text/VTL_publication_text_op.c>
#include <string.h>
VTL_publication_Text create_test_text(const char *str)
{
    VTL_publication_Text text;
    text.text = (VTL_publication_text_Symbol *)str;
    text.length = strlen(str);
    return text;
}

bool VTL_tests_bbcode_ToBBTestPlainText()
{
    VTL_publication_marked_text_Part part = {
        .text = "Простой текст",
        .length = strlen("Простой текст"),
        .type = 0};

    VTL_publication_MarkedText marked_text = {
        .parts = &part,
        .length = 1};

    VTL_publication_Text *result = NULL;
    VTL_AppResult res = VTL_publication_marked_text_TransformToBB(&result, &marked_text);

    if (res != VTL_res_kOk)
    {
        return false;
    }
    if (result == NULL)
    {
        return false;
    }

    bool content_ok = strcmp(result->text, "Простой текст") == 0;
    if (!content_ok)
    {
        if (result)
        {
            free(result->text);
            free(result);
        }
        return false;
    }

    if (result)
    {
        free(result->text);
        free(result);
    }

    return true;
}

bool VTL_tests_bbcode_ToBBTestRedundantTags()
{
    VTL_publication_marked_text_Part parts[2] = {
        {.text = "Часть1", .length = strlen("Часть1"), .type = VTL_TEXT_MODIFICATION_BOLD},
        {.text = "Часть2", .length = strlen("Часть2"), .type = VTL_TEXT_MODIFICATION_BOLD}};

    VTL_publication_MarkedText marked_text = {
        .parts = parts,
        .length = 2};

    VTL_publication_Text *result = NULL;
    VTL_AppResult res = VTL_publication_marked_text_TransformToBB(&result, &marked_text);

    if (res != VTL_res_kOk)
    {
        return false;
    }
    if (result == NULL)
    {
        return false;
    }

    bool content_ok = strcmp(result->text, "[b]Часть1Часть2[/b]") == 0;
    if (!content_ok)
    {
        if (result)
        {
            free(result->text);
            free(result);
        }
        return false;
    }

    if (result)
    {
        free(result->text);
        free(result);
    }

    return true;
}

bool VTL_tests_bbcode_ToBBTestMixedFormatting()
{
    VTL_publication_marked_text_Part parts[3] = {
        {.text = "Жирный ", .length = strlen("Жирный "), .type = VTL_TEXT_MODIFICATION_BOLD},
        {.text = "Курсив ", .length = strlen("Курсив "), .type = VTL_TEXT_MODIFICATION_ITALIC},
        {.text = "Обычный", .length = strlen("Обычный"), .type = 0}};

    VTL_publication_MarkedText marked_text = {
        .parts = parts,
        .length = 3};

    VTL_publication_Text *result = NULL;
    VTL_AppResult res = VTL_publication_marked_text_TransformToBB(&result, &marked_text);

    if (res != VTL_res_kOk)
    {
        return false;
    }
    if (result == NULL)
    {
        return false;
    }

    bool content_ok = strcmp(result->text, "[b]Жирный [/b][i]Курсив [/i]Обычный") == 0;
    if (!content_ok)
    {
        if (result)
        {
            free(result->text);
            free(result);
        }
        return false;
    }

    if (result)
    {
        free(result->text);
        free(result);
    }

    return true;
}

bool VTL_tests_bbcode_FromBBTestMultipleTags()
{
    VTL_publication_MarkedText *result = NULL;
    VTL_publication_Text text = create_test_text("[b]Bold[/b] [i]Italic[/i] [s]Strike[/s]");

    VTL_AppResult res = VTL_publication_text_InitFromBB(&result, &text);
    bool passed = true;

    if (passed)
    {
        passed = (result != NULL && result->length == 5);
    }

    if (passed)
    {
        passed = (result->parts[0].length == 4 &&
                  memcmp(result->parts[0].text, "Bold", 4) == 0 &&
                  result->parts[0].type == VTL_TEXT_MODIFICATION_BOLD);
    }

    if (passed)
    {
        passed = (result->parts[1].length == 1 &&
                  result->parts[1].text[0] == ' ' &&
                  result->parts[1].type == 0);
    }

    if (passed)
    {
        passed = (result->parts[2].length == 6 &&
                  memcmp(result->parts[2].text, "Italic", 6) == 0 &&
                  result->parts[2].type == VTL_TEXT_MODIFICATION_ITALIC);
    }

    if (passed)
    {
        passed = (result->parts[3].length == 1 &&
                  result->parts[3].text[0] == ' ' &&
                  result->parts[3].type == 0);
    }

    if (passed)
    {
        passed = (result->parts[4].length == 6 &&
                  memcmp(result->parts[4].text, "Strike", 6) == 0 &&
                  result->parts[4].type == VTL_TEXT_MODIFICATION_STRIKETHROUGH);
    }
    if (result)
        free(result->parts);
    free(result);

    return passed;
}

bool VTL_tests_bbcode_FromBBTestMultipleTagsTestUnclosedTags()
{
    VTL_publication_MarkedText *result = NULL;
    VTL_publication_Text text = create_test_text("[b]Bold [i]Italic");

    VTL_AppResult res = VTL_publication_text_InitFromBB(&result, &text);
    bool passed = true;

    if (passed)
    {
        passed = (result != NULL && result->length == 2);
    }

    if (passed)
    {
        // Check first part (bold)
        passed = (result->parts[0].length == 5 &&
                  memcmp(result->parts[0].text, "Bold ", 5) == 0 &&
                  result->parts[0].type == VTL_TEXT_MODIFICATION_BOLD);
    }

    if (passed)
    {
        // Check second part (bold + italic)
        passed = (result->parts[1].length == 6 &&
                  memcmp(result->parts[1].text, "Italic", 6) == 0 &&
                  result->parts[1].type == (VTL_TEXT_MODIFICATION_BOLD | VTL_TEXT_MODIFICATION_ITALIC));
    }

    if (result)
        free(result->parts);
    free(result);

    return passed;
}

bool VTL_tests_bbcode_FromBBTestSingleTag()
{
    VTL_publication_MarkedText *result = NULL;
    VTL_publication_Text text = create_test_text("[b]Bold text[/b]");

    VTL_AppResult res = VTL_publication_text_InitFromBB(&result, &text);
    bool passed = true;

    if (passed)
    {
        passed = (res == VTL_res_kOk);
    }

    if (passed)
    {
        passed = (result != NULL && result->length == 1);
    }

    if (passed)
    {
        passed = (result->parts[0].length == 9 &&
                  memcmp(result->parts[0].text, "Bold text", 9) == 0 &&
                  result->parts[0].type == VTL_TEXT_MODIFICATION_BOLD);
    }

    if (result)
        free(result->parts);
    free(result);

    return passed;
}

bool VTL_tests_bbcode_FromBBTestEmptyString()
{
    // Инициализация переменных
    VTL_publication_MarkedText *result = NULL;
    VTL_publication_Text text = create_test_text("");

    // Выполнение основной операции
    VTL_AppResult res = VTL_publication_text_InitFromBB(&result, &text);

    // Проверка первого условия
    bool passed = (res == VTL_res_kOk);

    // Если первое условие прошло успешно, проверяем второе
    if (passed)
    {
        passed = (result != NULL && result->length == 0);
    }

    // Освобождение памяти
    if (result)
        free(result->parts);
    free(result);

    return passed;
}

int main(void)
{
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_ToBBTestPlainText(), "ToBB: Тест простого текста не пройден\n"))
        return 1;
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_ToBBTestRedundantTags(), "ToBB: Тест с избыточными тегами не пройден\n"))
        return 1;
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_ToBBTestMixedFormatting(), "ToBB: Тест смешанного форматирования не пройден\n"))
        return 1;
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestMultipleTags(), "FromBB: Тест множественных тегов не пройден\n"))
        return 1;
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestMultipleTagsTestUnclosedTags(), "FromBB: Тест незакрытых тегов не пройден\n"))
        return 1;
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestSingleTag(), "FromBB: Тест одним тегом не пройден\n"))
        return 1;
    if (!VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestEmptyString(), "FromBB: Тест с пустой строкой не пройден\n"))
        return 1;

    VTL_Print("All tests PASSED\n");
    return 0;
}