/***********************************************************************
    created:    Fri Jul 8 2005
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _FalTabControl_h_
#define _FalTabControl_h_

#include "CEGUI/WindowRendererSets/Core/Module.h"
#include "CEGUI/widgets/TabControl.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        TabControl class for the FalagardBase module.

        This class requires LookNFeel to be assigned.  The LookNFeel should provide the following:

        States:
            - Enabled
            - Disabled

        Child Widgets:
            TabPane based widget with name suffix "__auto_TabPane__"
            optional: DefaultWindow to contain tab buttons with name suffix "__auto_TabPane__Buttons"

        Property initialiser definitions:
            - TabButtonType - specifies a TabButton based widget type to be
              created each time a new tab button is required.

    \note
        The current TabControl base class enforces a strict layout, so while
        imagery can be customised as desired, the general layout of the
        component widgets is, at least for the time being, fixed.
    */
    class COREWRSET_API FalagardTabControl : public TabControlWindowRenderer
    {
    public:
        static const String TypeName;     //! type name for this widget.

        /*!
        \brief
            Constructor
        */
        FalagardTabControl(const String& type);

        const String& getTabButtonType() const;
        void setTabButtonType(const String& type);

        void render();

    protected:
        // overridden from TabControl base class.
        TabButton* createTabButton(const String& name) const;

        // data fields
        String  d_tabButtonType;
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _FalTabControl_h_
