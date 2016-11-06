/***********************************************************************
    created:    27/3/2005
    author:     Tomas Lindquist Olsen (based on code by Paul D Turner)
    
    purpose:    Interface to base class for PopupMenu widget
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
#ifndef _CEGUIPopupMenu_h_
#define _CEGUIPopupMenu_h_

#include "../Base.h"
#include "../Window.h"
#include "./MenuBase.h"


#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Base class for popup menus
*/
class CEGUIEXPORT PopupMenu : public MenuBase
{
public:
    /************************************************************************
        Constants
    *************************************************************************/
    static const String EventNamespace;     //!< Namespace for global events
    static const String WidgetTypeName;             //!< Window factory name

    /*************************************************************************
        Accessor type functions
    *************************************************************************/
    /*!
    \brief
        Get the fade in time for this popup menu.

    \return
        The time in seconds that it takes for the popup to fade in.
        0 if fading is disabled.
    */
    float getFadeInTime(void) const      {return d_fadeInTime;}


    /*!
    \brief
        Get the fade out time for this popup menu.

    \return
        The time in seconds that it takes for the popup to fade out.
        0 if fading is disabled.
    */
    float getFadeOutTime(void) const     {return d_fadeOutTime;}


    /*!
    \brief
        Find out if this popup menu is open or closed;
    */
    bool isPopupMenuOpen(void) const     {return d_isOpen;}


    /*************************************************************************
        Manipulators
    *************************************************************************/
    /*!
    \brief
        Set the fade in time for this popup menu.

    \param fadetime
        The time in seconds that it takes for the popup to fade in.
        If this parameter is zero, fading is disabled.
    */
    void    setFadeInTime(float fadetime)         {d_fadeInTime=fadetime;}


    /*!
    \brief
        Set the fade out time for this popup menu.

    \param fadetime
        The time in seconds that it takes for the popup to fade out.
        If this parameter is zero, fading is disabled.
    */
    void    setFadeOutTime(float fadetime)        {d_fadeOutTime=fadetime;}


    /*!
    \brief
        Tells the popup menu to open.

    \param notify
        true if the parent menu item (if any) is to handle the opening. false if not.
    */
    void    openPopupMenu(bool notify=true);


    /*!
    \brief
        Tells the popup menu to close.

    \param notify
        true if the parent menu item (if any) is to handle the closing. false if not.
    */
    void    closePopupMenu(bool notify=true);


    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructor for PopupMenu objects
    */
    PopupMenu(const String& type, const String& name);


    /*!
    \brief
        Destructor for PopupMenu objects
    */
    virtual ~PopupMenu(void);


protected:
    /*************************************************************************
        Implementation Functions
    *************************************************************************/
    /*!
    \brief
    Perform actual update processing for this Window.

    \param elapsed
    float value indicating the number of seconds elapsed since the last update call.

    \return
    Nothing.
    */
    virtual void    updateSelf(float elapsed);


    /*!
    \brief
        Setup size and position for the item widgets attached to this Listbox

    \return
        Nothing.
    */
    virtual void    layoutItemWidgets(void);


    /*!
    \brief
        Resizes the popup menu to exactly fit the content that is attached to it.

    \return
        Nothing.
    */
    virtual Sizef getContentSize(void) const;

    /*************************************************************************
        Overridden event handlers
    *************************************************************************/
    virtual void onAlphaChanged(WindowEventArgs& e);
	virtual void onDestructionStarted(WindowEventArgs& e);
	virtual void onShown(WindowEventArgs& e);
	virtual void onHidden(WindowEventArgs& e);
	virtual void onMouseButtonDown(MouseEventArgs& e);
	virtual void onMouseButtonUp(MouseEventArgs& e);


    /*************************************************************************
        Implementation Data
    *************************************************************************/
    float d_origAlpha;      //!< The original alpha of this window.
    float d_fadeElapsed;    //!< The time in seconds this popup menu has been fading.
    float d_fadeOutTime;    //!< The time in seconds it takes for this popup menu to fade out.
    float d_fadeInTime;     //!< The time in seconds it takes for this popup menu to fade in.
    bool d_fading;          //!< true if this popup menu is fading in/out. false if not
    bool d_fadingOut;       //!< true if this popup menu is fading out. false if fading in.
    bool d_isOpen;          //!< true if this popup menu is open. false if not.


private:
    /*************************************************************************
    Private methods
    *************************************************************************/
    void    addPopupMenuProperties(void);
};


} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#    pragma warning(pop)
#endif


#endif    // end of guard _CEGUIPopupMenu_h_
