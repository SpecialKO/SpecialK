/***********************************************************************
    created:    Mon Jul 18 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFalComponentBase_h_
#define _CEGUIFalComponentBase_h_

#include "CEGUI/falagard/Dimensions.h"
#include "CEGUI/Window.h"
#include "CEGUI/ColourRect.h"

namespace CEGUI
{
//! Common base class used for renderable components within an ImagerySection.
class CEGUIEXPORT FalagardComponentBase :
    public AllocatedObject<FalagardComponentBase>
{
public:
    FalagardComponentBase();
    virtual ~FalagardComponentBase();

    /*!
    \brief
        Render this component.  More correctly, the component is cached for
        rendering.

    \param srcWindow
        Window to use as the base for translating the component's ComponentArea
        into pixel values.

    \param modColours
        ColourRect describing colours that are to be modulated with the
        component's stored colour values to calculate a set of 'final' colour
        values to be used.  May be 0.
    */
    void render(Window& srcWindow, const CEGUI::ColourRect* modColours = 0,
                const Rectf* clipper = 0, bool clipToDisplay = false) const;

    /*!
    \brief
        Render this component.  More correctly, the component is cached for
        rendering.

    \param srcWindow
        Window to use as the base for translating the component's ComponentArea
        into pixel values.

    \param baseRect
        Rect to use as the base for translating the component's ComponentArea
        into pixel values.

    \param modColours
        ColourRect describing colours that are to be modulated with the
        component's stored colour values to calculate a set of 'final' colour
        values to be used.  May be 0.
    */
    void render(Window& srcWindow, const Rectf& baseRect,
                const CEGUI::ColourRect* modColours = 0,
                const Rectf* clipper = 0, bool clipToDisplay = false) const;

    /*!
    \brief
        Return the ComponentArea of this component.

    \return
        ComponentArea object describing the component's current target area.
    */
    const ComponentArea& getComponentArea() const;

    /*!
    \brief
        Set the conponent's ComponentArea.

    \param area
        ComponentArea object describing a new target area for the component.
    */
    void setComponentArea(const ComponentArea& area);

    /*!
    \brief
        Return the ColourRect used by this component.

    \return
        ColourRect object holding the colours currently in use by this
        component.
    */
    const ColourRect& getColours() const;

    /*!
    \brief
        Set the colours to be used by this component.

    \param cols
        ColourRect object describing the colours to be used by this component.
    */
    void setColours(const ColourRect& cols);

    /*!
    \brief
        Returns the name of the property from which the colour values will be obtained, if set.

    \return
        String object holding the name of the property.
    */
    const String& getColoursPropertySource() const;

    /*!
    \brief
        Set the name of the property where colour values will be obtained, if set.

    \param property
        String containing the name of the property.
    */
    void setColoursPropertySource(const String& property);

    //! perform any processing required due to the given font having changed.
    virtual bool handleFontRenderSizeChange(Window& window,
                                            const Font* font) const;

protected:
    /*!
    \brief
        Helper function to initialise a ColourRect with appropriate values
        according to the way the component is set up.

        This will try and get values from multiple places:
            - a property attached to \a wnd
            - or the integral d_colours value.
    */
    void initColoursRect(const Window& wnd,
                         const ColourRect* modCols,
                         ColourRect& cr) const;

    //! Function to do main render caching work.
    virtual void render_impl(Window& srcWindow, Rectf& destRect,
                             const CEGUI::ColourRect* modColours,
                             const Rectf* clipper, bool clipToDisplay) const = 0;

    /*!
    \brief
        Writes xml for the colours to a OutStream.
        Will prefer property colours before explicit.

    \note
        This is intended as a helper function for sub-classes when outputting
        xml to a stream.

    \return
        - true if xml element was written.
        - false if nothing was output due to the formatting not being set
          (sub-class may then choose to do something else.)
    */
    bool writeColoursXML(XMLSerializer& xml_stream) const;

    //! Destination area for this component.
    ComponentArea d_area;
    //! base colours to be applied when rendering the image component.
    ColourRect d_colours;
    //! name of property to fetch colours from.
    String d_colourPropertyName;
};

}

#endif

