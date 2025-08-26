#ifndef GALAXY_THREAD_H
#define GALAXY_THREAD_H

namespace galaxy
{
    namespace api
    {

        typedef void* ThreadEntryParam;

        typedef void (*ThreadEntryFunction)(ThreadEntryParam);

        class IGalaxyThread
        {
        public:

            virtual void Join() = 0;

            virtual bool Joinable() = 0;

            virtual void Detach() = 0;

            virtual ~IGalaxyThread() {};
        };

        class IGalaxyThreadFactory
        {
        public:

            virtual IGalaxyThread* SpawnThread(ThreadEntryFunction const entryPoint, ThreadEntryParam param) = 0;

            virtual ~IGalaxyThreadFactory() {};
        };

    }
}

#endif