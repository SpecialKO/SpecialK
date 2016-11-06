/***********************************************************************
    created:    Wed Dec 23 2009
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
#ifndef _CEGUIRenderEffectManager_h_
#define _CEGUIRenderEffectManager_h_

#include "CEGUI/Singleton.h"
#include "CEGUI/IteratorBase.h"
#include "CEGUI/Logger.h"
#include "CEGUI/Exceptions.h"
#include "CEGUI/RenderEffectFactory.h"
#include <map>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Singleton class that manages creation and destruction of RenderEffect based
    objects.
*/
class CEGUIEXPORT RenderEffectManager :
    public Singleton<RenderEffectManager>,
    public AllocatedObject<RenderEffectManager>
{
private:
    //! Collection type used for the render effect registry
    typedef std::map<String, RenderEffectFactory*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, RenderEffectFactory*)> RenderEffectRegistry;

    //! Collection type to track which effects we created with which factories
    typedef std::map<RenderEffect*, RenderEffectFactory*, std::less<RenderEffect*>
        CEGUI_MAP_ALLOC(RenderEffect*, RenderEffectFactory*)> EffectCreatorMap;

    //! Collection of registered render effects
    RenderEffectRegistry d_effectRegistry;
    //! Collection of effect instances we created (and the factory used)
    EffectCreatorMap d_effects;

public:
    //! Iterator type that iterates over entries in the RenderEffectRegistry
    typedef ConstMapIterator<RenderEffectRegistry> RenderEffectIterator;

    //! Constructor for RenderEffectManager objects.
    RenderEffectManager();

    //! Destructor for RenderEffectManager objects.
    ~RenderEffectManager();

    /*!
    \brief
        Register a RenderEffect type with the system and associate it with the
        identifier \a name.

        This registers a RenderEffect based class, such that instances of that
        class can subsequently be created by requesting an effect using the
        specified identifier.

    \tparam T
        The RenderEffect based class to be instantiated when an effect is
        requested using the identifier \a name.

    \param name
        String object describing the identifier that the RenderEffect based
        class will be registered under.

    \exception AlreadyExistsException
        thrown if a RenderEffect is already registered using \a name.
    */
    template <typename T>
    void addEffect(const String& name);

    /*!
    \brief
        Remove / unregister any RenderEffect using the specified identifier.

    \param name
        String object describing the identifier of the RenderEffect that is to
        be removed / unregistered.  If no such RenderEffect is present, no
        action is taken.

    \note
        You should avoid removing RenderEffect types that are still in use.
        Internally a factory system is employed for the creation and deletion
        of RenderEffect objects; if an effect - and therefore it's factory - is
        removed while instances are still active, it will not be possible to
        safely delete those RenderEffect object instances.
    */
    void removeEffect(const String& name);

    /*!
    \brief
        Return whether a RenderEffect has been registered under the specified
        name.

    \param name
        String object describing the identifier of a RenderEffect to test for.

    \return
        - true if a RenderEffect with the specified name is registered.
        - false if no RenderEffect with the specified name is registered.
    */
    bool isEffectAvailable(const String& name) const;

    /*!
    \brief
        Create an instance of the RenderEffect based class identified by the
        specified name.

    \param name
        String object describing the identifier of the RenderEffect based
        class that is to be created.

    \param window
        Pointer to a Window object.  Exactly how or if this is used will
        depend upon the specific effect being created.

    \return
        Reference to the newly created RenderEffect.

    \exception UnknownObjectException
        thrown if no RenderEffect class has been registered using the
        identifier \a name.
    */
    RenderEffect& create(const String& name, Window* window);

    /*!
    \brief
        Destroy the given RenderEffect object.

    \note
        This function will only destroy objects that were created via the
        RenderEffectManager.  Attempts to destroy objects created by other
        means will result in an InvalidRequestException.  This option was
        chosen over silently ignoring the request in order to aid application
        developers in thier debugging.

    \param effect
        Reference to the RenderEffect object that is to be destroyed.

    \exception InvalidRequestException
        thrown if \a effect was not created by the RenderEffectManager.
    */
    void destroy(RenderEffect& effect);
};

//---------------------------------------------------------------------------//
template <typename T>
void RenderEffectManager::addEffect(const String& name)
{
    if (isEffectAvailable(name))
        CEGUI_THROW(AlreadyExistsException(
            "A RenderEffect is already registered under the name '" +
            name + "'"));

    // create an instance of a factory to create effects of type T
    d_effectRegistry[name] = CEGUI_NEW_AO TplRenderEffectFactory<T>;

    Logger::getSingleton().logEvent(
        "Registered RenderEffect named '" + name + "'");
}

//---------------------------------------------------------------------------//


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIRenderEffectManager_h_

