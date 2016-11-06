/***********************************************************************
    created:    Fri Oct 07 2011
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFactoryRegisterer_h_
#define _CEGUIFactoryRegisterer_h_

#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Base class encapsulating a type name and common parts of factory
    registration.
*/
class CEGUIEXPORT FactoryRegisterer :
    public AllocatedObject<FactoryRegisterer>
{
public:
    //! Destructor.
    virtual ~FactoryRegisterer();

    /*!
    \brief
        Perform registration (addition) of the factory for whichever
        type this class registers a factory for.
    */
    void registerFactory() const;

    /*!
    \brief
        Perform unregistration (removal) of the factory for whichever
        type this class registers a factory for.
    */
    virtual void unregisterFactory() const = 0;

    //! describes the type this class registers a factory for.
    const String d_type;

protected:
    //! Constructor.
    FactoryRegisterer(const String& type);

    //! Function to do the actual addition of a factory to the CEGUI system.
    virtual void doFactoryAdd() const = 0;

    //! Function to check if factory for our type is already registered.
    virtual bool isAlreadyRegistered() const = 0;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIFactoryRegisterer_h_

