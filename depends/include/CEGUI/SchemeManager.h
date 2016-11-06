/***********************************************************************
    created:    Mon Jul 20 2009
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
#ifndef _CEGUISchemeManager_h_
#define _CEGUISchemeManager_h_

#include "CEGUI/Base.h"
#include "CEGUI/Singleton.h"
#include "CEGUI/NamedXMLResourceManager.h"
#include "CEGUI/Scheme.h"
#include "CEGUI/Scheme_xmlHandler.h"
#include "CEGUI/IteratorBase.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4275)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    A class that manages the creation of, access to, and destruction of GUI
    Scheme objects
*/
class CEGUIEXPORT SchemeManager :
        public Singleton<SchemeManager>,
        public NamedXMLResourceManager<Scheme, Scheme_xmlHandler>,
        public AllocatedObject<SchemeManager>
{
public:
    //! Constructor.
    SchemeManager();

    //! Destructor.
    ~SchemeManager();

    //! Definition of SchemeIterator type.
    typedef ConstMapIterator<ObjectRegistry> SchemeIterator;

    /*!
    \brief
        Return a SchemeManager::SchemeIterator object to iterate over the
        available schemes.
    */
    SchemeIterator  getIterator() const;
    
    /*!
    \brief
        If this is enabled, Schemas will immediately load their resources after they are created
        
    It's sometimes useful to turn this off when you want to load things more selectively.
    This is enabled by default.
    
    \param enabled
        If true, you will have to load resources from the Scheme yourself!
    
    \note
        Calling Scheme::loadResources after you create the Scheme is equivalent to this being enabled
        and creating the scheme.
    */
    void setAutoLoadResources(bool enabled);
    
    /*!
    \brief
        Checks whether resources are loaded immediately after schemes are created
        
    \see
        SchemeManager::setAutoLoadResources
    */
    bool getAutoLoadResources() const;

protected:
    // override from base
    void doPostObjectAdditionAction(Scheme& object);
    
    //! If true, Scheme::loadResources is called after "create" is called for it
    bool d_autoLoadResources;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUISchemeManager_h_
