#include <VTL/publication/text/bbcode/VTL_publication_text_op_bbcode.c>
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

    VTL_publication_Text *bbcode_result = NULL;
    VTL_AppResult transformation_result = VTL_publication_marked_text_TransformToBB(&bbcode_result, &marked_text);

    if (transformation_result != VTL_res_kOk)
    {
        return false;
    }
    if (bbcode_result == NULL)
    {
        return false;
    }

    bool content_ok = strcmp(bbcode_result->text, "Простой текст") == 0;
    if (!content_ok)
    {
        if (bbcode_result)
        {
            free(bbcode_result->text);
            free(bbcode_result);
        }
        return false;
    }

    if (bbcode_result)
    {
        free(bbcode_result->text);
        free(bbcode_result);
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

    VTL_publication_Text *bbcode_result = NULL;
    VTL_AppResult transformation_result = VTL_publication_marked_text_TransformToBB(&bbcode_result, &marked_text);

    if (transformation_result != VTL_res_kOk)
    {
        return false;
    }
    if (bbcode_result == NULL)
    {
        return false;
    }

    bool content_ok = strcmp(bbcode_result->text, "[b]Часть1Часть2[/b]") == 0;
    if (!content_ok)
    {
        if (bbcode_result)
        {
            free(bbcode_result->text);
            free(bbcode_result);
        }
        return false;
    }

    if (bbcode_result)
    {
        free(bbcode_result->text);
        free(bbcode_result);
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

    VTL_publication_Text *bbcode_result = NULL;
    VTL_AppResult transformation_result = VTL_publication_marked_text_TransformToBB(&bbcode_result, &marked_text);

    if (transformation_result != VTL_res_kOk)
    {
        return false;
    }
    if (bbcode_result == NULL)
    {
        return false;
    }

    bool content_ok = strcmp(bbcode_result->text, "[b]Жирный [/b][i]Курсив [/i]Обычный") == 0;
    if (!content_ok)
    {
        if (bbcode_result)
        {
            free(bbcode_result->text);
            free(bbcode_result);
        }
        return false;
    }

    if (bbcode_result)
    {
        free(bbcode_result->text);
        free(bbcode_result);
    }

    return true;
}

bool VTL_tests_bbcode_FromBBTestMultipleTags()
{
    VTL_publication_MarkedText *marked_result = NULL;
    VTL_publication_Text text = create_test_text("[b]Bold[/b] [i]Italic[/i] [s]Strike[/s]");

    VTL_AppResult init_result = VTL_publication_text_InitFromBB(&marked_result, &text);
    bool passed = init_result == VTL_res_kOk;

    if (passed)
    {
        passed = (marked_result != NULL && marked_result->length == 5);
    }

    if (passed)
    {
        passed = (marked_result->parts[0].length == 4 &&
                  memcmp(marked_result->parts[0].text, "Bold", 4) == 0 &&
                  marked_result->parts[0].type == VTL_TEXT_MODIFICATION_BOLD);
    }

    if (passed)
    {
        passed = (marked_result->parts[1].length == 1 &&
                  marked_result->parts[1].text[0] == ' ' &&
                  marked_result->parts[1].type == 0);
    }

    if (passed)
    {
        passed = (marked_result->parts[2].length == 6 &&
                  memcmp(marked_result->parts[2].text, "Italic", 6) == 0 &&
                  marked_result->parts[2].type == VTL_TEXT_MODIFICATION_ITALIC);
    }

    if (passed)
    {
        passed = (marked_result->parts[3].length == 1 &&
                  marked_result->parts[3].text[0] == ' ' &&
                  marked_result->parts[3].type == 0);
    }

    if (passed)
    {
        passed = (marked_result->parts[4].length == 6 &&
                  memcmp(marked_result->parts[4].text, "Strike", 6) == 0 &&
                  marked_result->parts[4].type == VTL_TEXT_MODIFICATION_STRIKETHROUGH);
    }
    if (marked_result)
        free(marked_result->parts);
    free(marked_result);

    return passed;
}

bool VTL_tests_bbcode_FromBBTestMultipleTagsTestUnclosedTags()
{
    VTL_publication_MarkedText *marked_result = NULL;
    VTL_publication_Text text = create_test_text("[b]Bold [i]Italic");

    VTL_AppResult init_result = VTL_publication_text_InitFromBB(&marked_result, &text);
    bool passed = init_result == VTL_res_kOk;

    if (passed)
    {
        passed = (marked_result != NULL && marked_result->length == 2);
    }

    if (passed)
    {
        passed = (marked_result->parts[0].length == 5 &&
                  memcmp(marked_result->parts[0].text, "Bold ", 5) == 0 &&
                  marked_result->parts[0].type == VTL_TEXT_MODIFICATION_BOLD);
    }

    if (passed)
    {
        passed = (marked_result->parts[1].length == 6 &&
                  memcmp(marked_result->parts[1].text, "Italic", 6) == 0 &&
                  marked_result->parts[1].type == (VTL_TEXT_MODIFICATION_BOLD | VTL_TEXT_MODIFICATION_ITALIC));
    }

    if (marked_result)
        free(marked_result->parts);
    free(marked_result);

    return passed;
}

bool VTL_tests_bbcode_FromBBTestSingleTag()
{
    VTL_publication_MarkedText *marked_result = NULL;
    VTL_publication_Text text = create_test_text("[b]Bold text[/b]");

    VTL_AppResult init_result = VTL_publication_text_InitFromBB(&marked_result, &text);

    bool passed = init_result == VTL_res_kOk;

    if (passed)
    {
        passed = (marked_result != NULL && marked_result->length == 1);
    }

    if (passed)
    {
        passed = (marked_result->parts[0].length == 9 &&
                  memcmp(marked_result->parts[0].text, "Bold text", 9) == 0 &&
                  marked_result->parts[0].type == VTL_TEXT_MODIFICATION_BOLD);
    }

    if (marked_result)
        free(marked_result->parts);
    free(marked_result);

    return passed;
}

bool VTL_tests_bbcode_FromBBTestEmptyString()
{
    VTL_publication_MarkedText *marked_result = NULL;
    VTL_publication_Text text = create_test_text("");

    VTL_AppResult init_result = VTL_publication_text_InitFromBB(&marked_result, &text);

    bool passed = init_result == VTL_res_kOk;

    if (passed)
    {
        passed = (marked_result != NULL && marked_result->length == 0);
    }

    if (marked_result)
        free(marked_result->parts);
    free(marked_result);

    return passed;
}

int main(void)
{
    int passed_tests = 0;
    const int total_tests = 7;

    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_ToBBTestPlainText(), 
                                          "ToBB: Тест простого текста не пройден\n") ? 1 : 0;
    
    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_ToBBTestRedundantTags(),
                                          "ToBB: Тест с избыточными тегами не пройден\n") ? 1 : 0;
    
    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_ToBBTestMixedFormatting(),
                                          "ToBB: Тест смешанного форматирования не пройден\n") ? 1 : 0;
    
    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestMultipleTags(),
                                          "FromBB: Тест множественных тегов не пройден\n") ? 1 : 0;
    
    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestMultipleTagsTestUnclosedTags(),
                                          "FromBB: Тест незакрытых тегов не пройден\n") ? 1 : 0;
    
    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestSingleTag(),
                                          "FromBB: Тест одним тегом не пройден\n") ? 1 : 0;
    
    passed_tests += VTL_test_CheckCondition(VTL_tests_bbcode_FromBBTestEmptyString(),
                                          "FromBB: Тест с пустой строкой не пройден\n") ? 1 : 0;

    // Вывод сводной информации
    if (passed_tests == total_tests)
    {
        VTL_Print("Все тесты успешно пройдены (%d/%d)\n", passed_tests, total_tests);
        return 0;
    }
    else
    {
        VTL_Print("Пройдено %d из %d тестов\n", passed_tests, total_tests);
        return 1;
    }
}