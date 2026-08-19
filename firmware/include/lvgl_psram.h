#pragma once

#include <stddef.h>
#include <stdlib.h>

#include <esp_heap_caps.h>

static inline void* missionControlLvglAlloc(
    size_t size)
{
    if (size >= 64UL * 1024UL)
    {
        void* externalMemory =
            heap_caps_malloc(
                size,
                MALLOC_CAP_SPIRAM |
                    MALLOC_CAP_8BIT);

        if (externalMemory != NULL)
        {
            return externalMemory;
        }
    }

    return malloc(size);
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
    const uint32_t capabilities =
        size >= 64UL * 1024UL
            ? MALLOC_CAP_SPIRAM |
                  MALLOC_CAP_8BIT
            : MALLOC_CAP_8BIT;

    return heap_caps_realloc(
        pointer,
        size,
        capabilities);
}
