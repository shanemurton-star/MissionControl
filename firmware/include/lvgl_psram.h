#pragma once

#include <stddef.h>
#include <stdlib.h>

#include <esp_heap_caps.h>

static inline void* missionControlLvglAlloc(
    size_t size)
{
    // LVGL consists mostly of many small object/style allocations. Reserving
    // PSRAM only for 64 KB blocks left nearly the entire UI in scarce internal
    // RAM and starved LwIP DNS/TLS after display startup. The RGB DMA draw
    // buffer remains a separate static internal allocation; regular LVGL heap
    // objects are safe in byte-addressable PSRAM.
    void* externalMemory = heap_caps_malloc(
        size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (externalMemory != NULL) return externalMemory;

    return heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

static inline void missionControlLvglFree(
    void* pointer)
{
    free(pointer);
}

static inline void* missionControlLvglRealloc(
    void* pointer,
    size_t size)
{
    void* resized = heap_caps_realloc(
        pointer,
        size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (resized != NULL || size == 0) return resized;

    return heap_caps_realloc(pointer, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
