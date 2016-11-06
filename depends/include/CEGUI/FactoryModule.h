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
#ifndef _CEGUIFactoryModule_h_
#define _CEGUIFactoryModule_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include <vector>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class FactoryRegisterer;

class CEGUIEXPORT FactoryModule :
    public AllocatedObject<FactoryModule>
{
public:
    //! Destructor.
    virtual ~FactoryModule();
    //! Register the factory for objects of the specified type.
    void registerFactory(const String& type_name);
    //! Register factories for all object types in the module.
    uint registerAllFactories();
    //! Unregister the factory for objects of the specified type.
    void unregisterFactory(const String& type_name);
    //! Unregister factories for all object types in the module.
    uint unregisterAllFactories();

protected:
    //! Collection type that holds pointers to the factory registerer objects.
    typedef std::vector<FactoryRegisterer*
        CEGUI_VECTOR_ALLOC(FactoryRegisterer*)> FactoryRegistry;
    //! The collection of factorty registerer object pointers.
    FactoryRegistry d_registry;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIFactoryModule_h_

