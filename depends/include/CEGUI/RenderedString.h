/***********************************************************************
    created:    25/05/2009
    author:     Paul Turner
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
#ifndef _CEGUIRenderedString_h_
#define _CEGUIRenderedString_h_

#include "CEGUI/Size.h"
#include "CEGUI/Rect.h"
#include <vector>
#include <utility>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Class representing a rendered string of entities.

    Here 'string' does not refer solely to a text string, rather a string of
    any renderable items.
*/
class CEGUIEXPORT RenderedString :
    public AllocatedObject<RenderedString>
{
public:
    //! Constructor.
    RenderedString();

    //! Destructor.
    virtual ~RenderedString();

    /*!
    \brief
        Draw the string to a GeometryBuffer.

    \param line
        The line of the RenderedString to draw.

    \param buffer
        GeometryBuffer object that is to receive the geometry resulting from the
        draw operations.

    \param position
        Vector2 describing the position where the RenderedString is to be drawn.
        Note that this is not the final onscreen position, but the position as
        offset from the top-left corner of the entity represented by the
        GeometryBuffer.

    \param mod_colours
        Pointer to a ColourRect describing colour values that are to be
        modulated with the any stored colour values to calculate the final
        colour values to be used.  This may be 0 if no modulated colours are
        required.  NB: Each specific component will decide if and how it will
        apply the modulated colours.

    \param clip_rect
        Pointer to a Rect object that describes a clipping rectangle that should
        be used when drawing the RenderedString.  This may be 0 if no clipping
        is required.

    \param space_extra
        float value indicating additional padding value to be applied to space
        characters in the string.

    \exception InvalidRequestException
        thrown if \a line is out of range.
    */
    void draw(const Window* ref_wnd,
              const size_t line, GeometryBuffer& buffer,
              const Vector2f& position, const ColourRect* mod_colours,
              const Rectf* clip_rect, const float space_extra) const;

    /*!
    \brief
        Return the pixel size of a specified line for the RenderedString.

    \param line
        The line number whose size is to be returned.

    \return
        Size object describing the size of the rendered output of the specified
        line of this RenderedString, in pixels.

    \exception InvalidRequestException
        thrown if \a line is out of range.
    */
    Sizef getPixelSize(const Window* ref_wnd, const size_t line) const;

    //! Return the maximum horizontal extent of all lines, in pixels.
    float getHorizontalExtent(const Window* ref_wnd) const;

    //! Return the sum vertical extent of all lines, in pixels.
    float getVerticalExtent(const Window* ref_wnd) const;

    //! append \a component to the list of components drawn for this string.
    void appendComponent(const RenderedStringComponent& component);

    //! clear the list of components drawn for this string.
    void clearComponents();

    //! return the number of components that make up this string.
    size_t getComponentCount() const;

    /*!
    \brief
        split the string in line \a line as close to \a split_point as possible.

        The RenderedString \a left will receive the left portion of the split,
        while the right portion of the split will remain in this RenderedString.

    \param line
        The line number on which the split is to occur.

    \param split_point
        float value specifying the pixel location where the split should occur.
        The actual split will occur as close to this point as possible, though
        preferring a shorter 'left' portion when the split can not be made
        exactly at the requested point.

    \param left
        RenderedString object that will receieve the left portion of the split.
        Any existing content in the RenderedString is replaced.

    \exception InvalidRequestException
        thrown if \a line is out of range.
    */
    void split(const Window* ref_wnd,
               const size_t line, float split_point, RenderedString& left);

    //! return the total number of spacing characters in the specified line.
    size_t getSpaceCount(const size_t line) const;

    //! linebreak the rendered string at the present position.
    void appendLineBreak();

    //! return number of lines in this string.
    size_t getLineCount() const;

    //! set selection highlight
    void setSelection(const Window* ref_wnd, float start, float end);

    //! Copy constructor.
    RenderedString(const RenderedString& other);
    //! Assignment.
    RenderedString& operator=(const RenderedString& rhs);

protected:
    //! Collection type used to hold the string components.
    typedef std::vector<RenderedStringComponent*
        CEGUI_VECTOR_ALLOC(RenderedStringComponent*)> ComponentList;
    //! RenderedStringComponent objects that comprise this RenderedString.
    ComponentList d_components;
    //! track info for a line.  first is componetn idx, second is component count.
    typedef std::pair<size_t, size_t> LineInfo;
    //! Collection type used to hold details about the lines.
    typedef std::vector<LineInfo
        CEGUI_VECTOR_ALLOC(LineInfo)> LineList;
    //! lines that make up this string.
    LineList d_lines;
    //! Make this object's component list a clone of \a list.
    void cloneComponentList(const ComponentList& list);
    //! Free components in the given ComponentList and clear the list.
    static void clearComponentList(ComponentList& list);
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif // end of guard _CEGUIRenderedString_h_
