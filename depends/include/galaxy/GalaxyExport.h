 #ifndef GALAXY_EXPORT_H
 #define GALAXY_EXPORT_H

 #if defined(_WIN32) || defined(_XBOX_ONE) || defined(__ORBIS__) || defined(__PROSPERO__)
     #if defined(GALAXY_EXPORT)
         #define GALAXY_DLL_EXPORT __declspec(dllexport)
     #elif defined(GALAXY_NODLL)
         #define GALAXY_DLL_EXPORT
     #else
         #define GALAXY_DLL_EXPORT __declspec(dllimport)
     #endif
 #else
     #define GALAXY_DLL_EXPORT
 #endif

 #if defined(_WIN32) && !defined(__ORBIS__) && !defined(_XBOX_ONE) && !defined(__PROSPERO__)
     #define GALAXY_CALLTYPE __cdecl
 #else
     #define GALAXY_CALLTYPE
 #endif

 #endif