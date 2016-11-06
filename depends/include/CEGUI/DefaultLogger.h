/***********************************************************************
    created:    25/1/2006
    author:     Andrew Zabolotny
    
    purpose:    Defines interface for the default Logger implementation
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#ifndef _CEGUIDefaultLogger_h_
#define _CEGUIDefaultLogger_h_

#include "CEGUI/Logger.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4275)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
/*!
\brief
    Default implementation for the Logger class.
    If you want to redirect CEGUI logs to some place other than a text file,
    implement your own Logger implementation and create a object of the
    Logger type before creating the CEGUI::System singleton.
*/
class CEGUIEXPORT DefaultLogger : public Logger
{
public:
    DefaultLogger(void);
    ~DefaultLogger(void);

    // overridden from Logger
    void logEvent(const String& message, LoggingLevel level = Standard);
    void setLogFilename(const String& filename, bool append = false);

protected:
    //! Stream used to implement the logger
    std::ofstream d_ostream;
    //! Used to build log entry strings. 
    std::ostringstream d_workstream;

    typedef std::pair<String, LoggingLevel> CacheItem;
    typedef std::vector<CacheItem
        CEGUI_VECTOR_ALLOC(CacheItem)> Cache;
    //! Used to cache log entries before log file is created. 
    Cache d_cache;
    //! true while log entries are beign cached (prior to logfile creation)
    bool d_caching;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

