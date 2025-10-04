#ifndef GALAXY_INIT_OPTIONS_H
#define GALAXY_INIT_OPTIONS_H

#include "galaxy/GalaxyAllocator.h"
#include "galaxy/GalaxyThread.h"

#include "stdint.h"
#include <cstddef>

namespace galaxy
{
    namespace api
    {
        struct InitOptions_1_121_2
        {
            InitOptions_1_121_2(
                const char* _clientID,
                const char* _clientSecret,
                const char* _configFilePath = ".",
                GalaxyAllocator* _galaxyAllocator = NULL,
                const char* _storagePath = NULL,
                const char* _host = NULL,
                uint16_t _port = 0,
                IGalaxyThreadFactory* _galaxyThreadFactory = NULL)
                : clientID(_clientID)
                , clientSecret(_clientSecret)
                , configFilePath(_configFilePath)
                , storagePath(_storagePath)
                , galaxyAllocator(_galaxyAllocator)
                , galaxyThreadFactory(_galaxyThreadFactory)
                , host(_host)
                , port(_port)
            {
            }

            const char* clientID;
            const char* clientSecret;
            const char* configFilePath;
            const char* storagePath;
            GalaxyAllocator* galaxyAllocator;
            IGalaxyThreadFactory* galaxyThreadFactory;
            const char* host;
            uint16_t port;
        };

    }
}

#endif