#ifndef GALAXY_I_ALLOCATOR_H
#define GALAXY_I_ALLOCATOR_H

#include "stdint.h"
#include <cstddef>

namespace galaxy
{
    namespace api
    {
        typedef void* (*GalaxyMalloc)(uint32_t size, const char* typeName);

        typedef void* (*GalaxyRealloc)(void* ptr, uint32_t newSize, const char* typeName);

        typedef void (*GalaxyFree)(void* ptr);

        struct GalaxyAllocator
        {
            GalaxyAllocator()
                : galaxyMalloc(NULL)
                , galaxyRealloc(NULL)
                , galaxyFree(NULL)
            {}

            GalaxyAllocator(GalaxyMalloc _galaxyMalloc, GalaxyRealloc _galaxyRealloc, GalaxyFree _galaxyFree)
                : galaxyMalloc(_galaxyMalloc)
                , galaxyRealloc(_galaxyRealloc)
                , galaxyFree(_galaxyFree)
            {}

            GalaxyMalloc galaxyMalloc;
            GalaxyRealloc galaxyRealloc;
            GalaxyFree galaxyFree;
        };

    }
}

#endif