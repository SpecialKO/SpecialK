/***********************************************************************
    created:    21/2/2004
    author:     Paul D Turner

    purpose:    Defines abstract base class for WindowFactory objects
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIWindowFactory_h_
#define _CEGUIWindowFactory_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/Window.h"

/*!
\brief
    Declares a window factory class.

\param T
    The window class name.

\note
    The class that will be generated is is named &lt;classname&gt;Factory.
    A statement like this:
        CEGUI_DECLARE_WINDOW_FACTORY(MyWidget);
    Would generate a factory class named MyWidgetFactory.

    The factory is automatically instantiated and for the example it would
    be available as:
        WindowFactory* wf = &(getMyWidgetFactory());
    or
        WindowFactory* wf = &CEGUI_WINDOW_FACTORY(MyWidget);
*/
#define CEGUI_DECLARE_WINDOW_FACTORY( T )\
    class T ## Factory : public WindowFactory\
    {\
    public:\
        T ## Factory() : WindowFactory( T::WidgetTypeName ) {}\
        Window* createWindow(const String& name)\
        {\
            return CEGUI_NEW_AO T(d_type, name);\
        }\
        void destroyWindow(Window* window)\
        {\
            CEGUI_DELETE_AO window;\
        }\
    };\
    T ## Factory& get ## T ## Factory();

/*!
\brief
    Generates code for the constructor for the instance of the window factory
    generated from the class name \a T
*/
#define CEGUI_DEFINE_WINDOW_FACTORY( T )\
    T ## Factory& get ## T ## Factory()\
    {\
        static T ## Factory s_factory;\
        return s_factory;\
    }

/*!
\brief
    Helper macro that return the real factory class name from a given class
    name \a T
*/
#define CEGUI_WINDOW_FACTORY( T ) (get ## T ## Factory())

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Abstract class that defines the required interface for all WindowFactory
    objects.

    A WindowFactory is used to create and destroy windows of a specific type.
    For every type of Window object wihin the system (widgets, dialogs, movable
    windows etc) there must be an associated WindowFactory registered with the
    WindowFactoryManager so that the system knows how to create and destroy
    those types of Window base object.

\note
    The use if of the CEGUI_DECLARE_WINDOW_FACTORY, CEGUI_DEFINE_WINDOW_FACTORY
    and CEGUI_WINDOW_FACTORY macros is deprecated in favour of the
    template class TplWindowFactory and templatised
    WindowFactoryManager::addFactory function, whereby you no longer need to
    directly create any supporting structure for your new window type, and can
    simply do:
    \code
    CEGUI::WindowFactoryManager::addFactory<TplWindowFactory<MyWidget> >();
    \endcode
*/
class CEGUIEXPORT WindowFactory :
    public AllocatedObject<WindowFactory>
{
public:
    /*!
    \brief
        Create a new Window object of whatever type this WindowFactory produces.

    \param name
        A unique name that is to be assigned to the newly created Window object

    \return
        Pointer to the new Window object.
    */
    virtual Window* createWindow(const String& name) = 0;

    /*!
    \brief
        Destroys the given Window object.

    \param window
        Pointer to the Window object to be destroyed.

    \return
        Nothing.
    */
    virtual void destroyWindow(Window* window) = 0;

    /*!
    \brief
        Get the string that describes the type of Window object this
        WindowFactory produces.

    \return
        String object that contains the unique Window object type produced by
        this WindowFactory
    */
    const String& getTypeName() const
    { return d_type; }

    //! Destructor.
    virtual ~WindowFactory()
    {}

protected:
    //! Constructor
    WindowFactory(const String& type) :
        d_type(type)
    {}

protected:
    //! String holding the type of object created by this factory.
    String  d_type;
};

} // End of  CEGUI namespace section

#endif // end of guard _CEGUIWindowFactory_h_
