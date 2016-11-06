/***********************************************************************
    created:    21/2/2005
    author:     Paul D Turner
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
#ifndef _CEGUITooltip_h_
#define _CEGUITooltip_h_

#include "../Window.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Base class for Tooltip window renderer objects.
    */
    class CEGUIEXPORT TooltipWindowRenderer : public WindowRenderer
    {
    public:
        /*!
        \brief
            Constructor
        */
        TooltipWindowRenderer(const String& name);

        /*!
        \brief
            Return the size of the area that will be occupied by the tooltip text, given
            any current formatting options.

        \return
            Size object describing the size of the rendered tooltip text in pixels.
        */
        virtual Sizef getTextSize() const = 0;
    };

    /*!
    \brief
        Base class for Tooltip widgets.
        
        The Tooltip class shows a simple pop-up window around the mouse position
        with some text information.  The tool-tip fades in when the user hovers
        with the mouse over a window which has tool-tip text set, and then fades
        out after some pre-set time.

    \note
        For Tooltip to work properly, you must specify a default tool-tip widget
        type via System::setTooltip, or by setting a custom tool-tip object for
        your Window(s).  Additionally, you need to ensure that time pulses are
        properly passed to the system via System::injectTimePulse.
    */
    class CEGUIEXPORT Tooltip : public Window
    {
    public:
        /*************************************************************************
            Constants
        *************************************************************************/
        static const String WidgetTypeName;                 //!< Window factory name
        static const String EventNamespace;                 //!< Namespace for global events
        /** Event fired when the hover timeout for the tool tip gets changed.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the Tooltip whose hover timeout has
         * been changed.
         */
        static const String EventHoverTimeChanged;
        /** Event fired when the display timeout for the tool tip gets changed.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the Tooltip whose display timeout has
         * been changed.
         */
        static const String EventDisplayTimeChanged;
        /** Event fired when the fade timeout for the tooltip gets changed.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the Tooltip whose fade timeout has
         * been changed.
         */
        static const String EventFadeTimeChanged;
        /** Event fired when the tooltip is about to get activated.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the Tooltip that is about to become
         * active.
         */
        static const String EventTooltipActive;
        /** Event fired when the tooltip has been deactivated.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the Tooltip that has become inactive.
         */
        static const String EventTooltipInactive;
        /** Event fired when the tooltip changes target window but stays active.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the Tooltip that has transitioned.
         */
        static const String EventTooltipTransition;

        /************************************************************************
            Object Construction and Destruction
        ************************************************************************/
        /*!
        \brief
            Constructor for the Tooltip base class constructor
         */
        Tooltip(const String& type, const String& name);

        /*!
        \brief
            Destructor for the Tooltip base class.
         */
        ~Tooltip(void);

        /************************************************************************
            Public interface
        ************************************************************************/
        /*!
        \brief
            Sets the target window for the tooltip.  This used internally to manage tooltips, you
            should not have to call this yourself.

        \param wnd
            Window object that the tooltip should be associated with (for now).

        \return
            Nothing.
         */
        void setTargetWindow(Window* wnd);

        /*!
        \brief
            return the current target window for this Tooltip.

        \return
            Pointer to the target window for this Tooltip or 0 for none.
        */
        const Window* getTargetWindow();

        /*!
        \brief
            Resets the timer on the tooltip when in the Active / Inactive states.  This is used internally
            to control the tooltip, it is not normally necessary to call this method yourself.

        \return
            Nothing.
         */
        void resetTimer(void);

        /*!
        \brief
            Return the number of seconds the mouse should hover stationary over the target window before
            the tooltip gets activated.

        \return
            float value representing a number of seconds.
         */
        float getHoverTime(void) const;

        /*!
        \brief
            Set the number of seconds the tooltip should be displayed for before it automatically
            de-activates itself.  0 indicates that the tooltip should never timesout and auto-deactivate.

        \param seconds
            float value representing a number of seconds.

        \return
            Nothing.
         */
        void setDisplayTime(float seconds);

        /*!
        \brief
            Set the number of seconds the mouse should hover stationary over the target window before
            the tooltip gets activated.

        \param seconds
            float value representing a number of seconds.

        \return
            Nothing.
         */
        void setHoverTime(float seconds);

        /*!
        \brief
            Return the number of seconds the tooltip should be displayed for before it automatically
            de-activates itself.  0 indicates that the tooltip never timesout and auto-deactivates.

        \return
            float value representing a number of seconds.
         */
        float getDisplayTime(void) const;

        /*!
        \brief
            Causes the tooltip to position itself appropriately.

        \return
            Nothing.
        */
        void positionSelf(void);

        /*!
        \brief
            Causes the tooltip to resize itself appropriately.

        \return
            Nothing.
        */
        void sizeSelf(void);

        /*!
        \brief
            Return the size of the area that will be occupied by the tooltip text, given
            any current formatting options.

        \return
            Size object describing the size of the rendered tooltip text in pixels.
        */
        Sizef getTextSize() const;

        /*!
        \brief
            Return the size of the area that will be occupied by the tooltip text, given
            any current formatting options.

        \return
            Size object describing the size of the rendered tooltip text in pixels.
        */
        virtual Sizef getTextSize_impl() const;

    protected:
        /*************************************************************************
            Implementation Methods
        *************************************************************************/
        // methods to perform processing for each of the widget states
        void doActiveState(float elapsed);
        void doInactiveState(float elapsed);

        // methods to switch widget states
        void switchToInactiveState(void);
        void switchToActiveState(void);

        // validate window renderer
        virtual bool validateWindowRenderer(const WindowRenderer* renderer) const;

        /*************************************************************************
            Event triggers
        *************************************************************************/
        /*!
        \brief
            Event trigger method called when the hover timeout gets changed.

        \param e
            WindowEventArgs object.
        */
        virtual void onHoverTimeChanged(WindowEventArgs& e);

        /*!
        \brief
            Event trigger method called when the display timeout gets changed.

        \param e
            WindowEventArgs object.
        */
        virtual void onDisplayTimeChanged(WindowEventArgs& e);

        /*!
        \brief
            Event trigger method called just before the tooltip becomes active.

        \param e
            WindowEventArgs object.
        */
        virtual void onTooltipActive(WindowEventArgs& e);

        /*!
        \brief
            Event trigger method called just after the tooltip is deactivated.

        \param e
            WindowEventArgs object.
        */
        virtual void onTooltipInactive(WindowEventArgs& e);

        /*!
        \brief
            Event trigger method called just after the tooltip changed target window but remained active.

        \param e
            WindowEventArgs object.
        */
        virtual void onTooltipTransition(WindowEventArgs& e);


        /************************************************************************
            Overridden from Window.
        ************************************************************************/
        void updateSelf(float elapsed);
        void onHidden(WindowEventArgs& e);
        void onMouseEnters(MouseEventArgs& e);
        void onTextChanged(WindowEventArgs& e);

        /************************************************************************
            Data fields
        ************************************************************************/
        bool        d_active;       //!< true if the tooltip is active
        float       d_elapsed;      //!< Used to track state change timings
        const Window* d_target;     //!< Current target Window for this Tooltip.
        float       d_hoverTime;    //!< tool-tip hover time (seconds mouse must stay stationary before tip shows).
        float       d_displayTime;  //!< tool-tip display time (seconds that tip is showsn for).
        float       d_fadeTime;     //!< tool-tip fade time (seconds it takes for tip to fade in and/or out).
        //! are in positionSelf function? (to avoid infinite recursion issues)
        bool d_inPositionSelf;

    private:
        /*************************************************************************
            Private methods
        *************************************************************************/
        void addTooltipProperties(void);
    };
} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUITooltip_h_
