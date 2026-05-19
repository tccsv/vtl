#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_data.h>

#include <stdlib.h>


// стартовая ёмкость списка маркеров. дальше растём удвоением
#define VTL_TELEGRAM_INITIAL_CAPACITY 64


VTL_AppResult VTL_telegram_MarkerListInit(VTL_telegram_MarkerList* list)
{
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
    return VTL_res_kOk;
}

VTL_AppResult VTL_telegram_MarkerListReserve(VTL_telegram_MarkerList* list, size_t capacity)
{
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    if (capacity <= list->capacity) {
        return VTL_res_kOk;
    }
    VTL_telegram_Marker* new_items = (VTL_telegram_Marker*)realloc(
        list->items, capacity * sizeof(VTL_telegram_Marker));
    if (!new_items) {
        return VTL_res_kMemAllocErr;
    }
    list->items = new_items;
    list->capacity = capacity;
    return VTL_res_kOk;
}

VTL_AppResult VTL_telegram_MarkerListPush(VTL_telegram_MarkerList* list,
                                          VTL_telegram_Marker marker)
{
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    // удвоение даёт амортизированный O(1) на вставку
    if (list->length >= list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : VTL_TELEGRAM_INITIAL_CAPACITY;
        VTL_AppResult res = VTL_telegram_MarkerListReserve(list, new_cap);
        if (res != VTL_res_kOk) {
            return res;
        }
    }
    list->items[list->length++] = marker;
    return VTL_res_kOk;
}

void VTL_telegram_MarkerListClear(VTL_telegram_MarkerList* list)
{
    if (list) {
        list->length = 0;
    }
}

void VTL_telegram_MarkerListFree(VTL_telegram_MarkerList* list)
{
    if (!list) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
}
