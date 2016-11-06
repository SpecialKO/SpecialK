/***********************************************************************
    created:    Sun Feb 19 2006
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIIrrlichtMemoryFile_h_
#define _CEGUIIrrlichtMemoryFile_h_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "../../Base.h"
#include "../../String.h"
#include <irrlicht.h>

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Class to wrap a file access interface around a memory buffer to enable us to
    pass data that has been loaded via the CEGUI::ResourceProvider to irrlicht,
    via it's IReadFile based interfaces.
*/
class IrrlichtMemoryFile : public irr::io::IReadFile
{
public:
    IrrlichtMemoryFile(const String& filename, const unsigned char* memory,
                       uint32 size);
    virtual ~IrrlichtMemoryFile() {};

    // implement required interface from IReadFile
    irr::s32 read(void* buffer, irr::u32 sizeToRead);
    long getSize() const;
    long getPos() const;
    bool seek(long finalPos, bool relativeMovement = false);
#if CEGUI_IRR_SDK_VERSION >= 16
    const irr::io::path& getFileName() const;
#else
    const irr::c8* getFileName() const;
#endif

protected:
#if CEGUI_IRR_SDK_VERSION >= 16
    irr::io::path d_filename;
#else
    String d_filename;
#endif
    const unsigned char* d_buffer;
    uint32 d_size;
    uint32 d_position;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIIrrlichtMemoryFile_h_
