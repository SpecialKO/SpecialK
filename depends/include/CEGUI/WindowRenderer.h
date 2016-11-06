/***********************************************************************
    created:    Jan 11 2006
    author:     Tomas Lindquist Olsen

    purpose:    Defines interface for the WindowRenderer base class
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
#ifndef _CEGUIWindowRenderer_h_
#define _CEGUIWindowRenderer_h_

#include "CEGUI/Window.h"
#include "CEGUI/Property.h"
#include <vector>
#include <utility>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Base-class for the assignable WindowRenderer object
*/
class CEGUIEXPORT WindowRenderer :
    public AllocatedObject<WindowRenderer>
{
public:
    /*************************************************************************
        Constructor / Destructor
    **************************************************************************/
    /*!
    \brief
        Constructor

    \param name
        Factory type name

    \param class_name
        The name of a widget class that is to be the minimum requirement for
        this window renderer.
    */
    WindowRenderer(const String& name, const String& class_name="Window");

    /*!
    \brief
        Destructor
    */
    virtual ~WindowRenderer();

    /*************************************************************************
        Public interface
    **************************************************************************/
    /*!
    \brief
        Populate render cache.

        This method must be implemented by all window renderers and should
        perform the rendering operations needed for this widget.
        Normally using the Falagard API...
    */
    virtual void render() = 0;

    /*!
    \brief
        Returns the factory type name of this window renderer.
    */
    const String& getName() const   {return d_name;}

    /*!
    \brief
        Get the window this windowrenderer is attached to.
    */
    Window* getWindow() const       {return d_window;}

    /*!
    \brief
        Get the "minimum" Window class this renderer requires
    */
    const String& getClass() const  {return d_class;}

    /*!
    \brief
        Get the Look'N'Feel assigned to our window
    */
    const WidgetLookFeel& getLookNFeel() const;

    /*!
    \brief
        Get unclipped inner rectangle that our window should return from its
        member function with the same name.
    */
    virtual Rectf getUnclippedInnerRect() const;

    /*!
    \brief
        Method called to perform extended laying out of the window's attached
        child windows.
    */
    virtual void performChildWindowLayout() {}

    /*!
    \brief
        update the RenderingContext as needed for our window.  This is normally
        invoked via our window's member function with the same name.
    */
    virtual void getRenderingContext(RenderingContext& ctx) const;

    //! perform any time based updates for this WindowRenderer.
    virtual void update(float /*elapsed*/) {}

    /*!
    \brief
        Perform any updates needed because the given font's render size has
        changed.

    /note
        This base implementation deals with updates needed for various
        definitions in the assigned widget look.  If you override, you should
        generally always call this base class implementation.

    \param font
        Pointer to the Font whose render size has changed.

    \return
        - true if some action was taken.
        - false if no action was taken (i.e font is not used here).
    */
    virtual bool handleFontRenderSizeChange(const Font* const font);

protected:
    /*************************************************************************
        Implementation methods
    **************************************************************************/
    /*!
    \brief
        Register a property class that will be properly managed by this window
        renderer.

    \param property
        Pointer to a static Property object that will be added to the target
        window.

    \param ban_from_xml
        - true if this property should be added to the 'ban' list so that it is
          not written in XML output.
        - false if this property is not banned and should appear in XML output.
    */
    void registerProperty(Property* property, const bool ban_from_xml);

    /*!
    \brief
        Register a property class that will be properly managed by this window
        renderer.

    \param property
        Pointer to a static Property object that will be added to the target
        window.
    */
    void registerProperty(Property* property);

    /*!
    \brief
        Handler called when this windowrenderer is attached to a window
    */
    virtual void onAttach();

    /*!
    \brief
        Handler called when this windowrenderer is detached from its window
    */
    virtual void onDetach();

    /*!
    \brief
        Handler called when a Look'N'Feel is assigned to our window.
    */
    virtual void onLookNFeelAssigned() {}

    /*!
    \brief
        Handler called when a Look'N'Feel is removed/unassigned from our window.
    */
    virtual void onLookNFeelUnassigned() {}

    /*************************************************************************
        Implementation data
    **************************************************************************/
    Window* d_window;       //!< Pointer to the window this windowrenderer is assigned to.
    const String d_name;    //!< Name of the factory type used to create this window renderer.
    const String d_class;   //!< Name of the widget class that is the "minimum" requirement.

    //! type used for entries in the PropertyList.
    typedef std::pair<Property*, bool> PropertyEntry;
    //! type to use for the property list.
    typedef std::vector<PropertyEntry
        CEGUI_VECTOR_ALLOC(PropertyEntry)> PropertyList;
    PropertyList d_properties;  //!< The list of properties that this windowrenderer will be handling.

    // Window is friend so it can manipulate our 'd_window' member directly.
    // We don't want users fiddling with this so no public interface.
    friend class Window;

private:
    WindowRenderer& operator=(const WindowRenderer&) { return *this; }
};

/*!
\brief
    Base-class for WindowRendererFactory
*/
class CEGUIEXPORT WindowRendererFactory
{
public:
    /*!
    \brief
        Contructor

    \param name
        Type name for this window renderer factory
    */
    WindowRendererFactory(const String& name) : d_factoryName(name) {}

    /*!
    \brief
        Destructor
    */
    virtual ~WindowRendererFactory() {}

    /*!
    \brief
        Returns the type name of this window renderer factory.
    */
    const String& getName() const {return d_factoryName;}

    /*!
    \brief
        Creates and returns a new window renderer object.
    */
    virtual WindowRenderer* create() = 0;

    /*!
    \brief
        Destroys a window renderer object previously created by us.
    */
    virtual void destroy(WindowRenderer* wr) = 0;

protected:
    String d_factoryName;   //!< Our factory type name.
};

} // End of CEGUI namespace

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif // _CEGUIWindowRenderer_h_
