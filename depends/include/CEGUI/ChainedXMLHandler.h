/***********************************************************************
    created:    Wed Aug 11 2010
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIChainedXMLHandler_h_
#define _CEGUIChainedXMLHandler_h_

#include "CEGUI/XMLHandler.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Abstract XMLHandler based class
class CEGUIEXPORT ChainedXMLHandler : public XMLHandler
{
public:
    ChainedXMLHandler();
    virtual ~ChainedXMLHandler();

    // XMLHandler overrides
    const String& getSchemaName() const;
    const String& getDefaultResourceGroup() const;
    void elementStart(const String& element, const XMLAttributes& attributes);
    void elementEnd(const String& element);

    //! returns whether this chained handler has completed.
    bool completed() const;

protected:
    //! Function that handles elements locally (used at end of handler chain)
    virtual void elementStartLocal(const String& element,
                                   const XMLAttributes& attributes) = 0;
    //! Function that handles elements locally (used at end of handler chain)
    virtual void elementEndLocal(const String& element) = 0;

    //! clean up any chained handler.
    void cleanupChainedHandler();

    //! chained xml handler object.
    ChainedXMLHandler* d_chainedHandler;
    //! is the chained handler completed.
    bool d_completed;
    //! should the chained handler be deleted by us?
    bool d_deleteChaniedHandler;
};


} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIChainedXMLHandler_h_

