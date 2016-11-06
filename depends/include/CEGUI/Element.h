/***********************************************************************
    created:    18/8/2011
    author:     Martin Preisler

    purpose:    Defines a class representing an item in a graph
                (deals with relative positions, relative dimensions, ...)
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

#ifndef _CEGUIElement_h_
#define _CEGUIElement_h_

#include "CEGUI/Base.h"
#include "CEGUI/PropertySet.h"
#include "CEGUI/EventSet.h"
#include "CEGUI/EventArgs.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{

/*!
\brief Enumerated type used when specifying horizontal alignments for Element

\see VerticalAlignment
 */
enum HorizontalAlignment
{
    /**
     * Element's position specifies an offset of it's left edge from the left
     * edge of it's parent.
     */
    HA_LEFT,
    /**
     * Element's position specifies an offset of it's horizontal centre from the
     * horizontal centre of it's parent.
     */
    HA_CENTRE,
    /**
     * Element's position specifies an offset of it's right edge from the right
     * edge of it's parent.
     */
    HA_RIGHT
};

template<>
class PropertyHelper<HorizontalAlignment>
{
public:
    typedef HorizontalAlignment return_type;
    typedef return_type safe_method_return_type;
    typedef HorizontalAlignment pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("HorizontalAlignment");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "Centre")
        {
            return HA_CENTRE;
        }
        else if (str == "Right")
        {
            return HA_RIGHT;
        }
        else
        {
            return HA_LEFT;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == HA_CENTRE)
        {
            return "Centre";
        }
        else if (val == HA_RIGHT)
        {
            return "Right";
        }
        else if (val == HA_LEFT)
        {
            return "Left";
        }
        else
        {
            assert(false && "Invalid horizontal alignment");
            return "Centre";
        }
    }
};

/*!
\brief Enumerated type used when specifying vertical alignments for Element

\see HorizontalAlignment
 */
enum VerticalAlignment
{
    /**
     * Element's position specifies an offset of it's top edge from the top edge
     * of it's parent.
     */
    VA_TOP,
    /**
     * Element's position specifies an offset of it's vertical centre from the
     * vertical centre of it's parent.
     */
    VA_CENTRE,
    /**
     * Element's position specifies an offset of it's bottom edge from the
     * bottom edge of it's parent.
     */
    VA_BOTTOM
};

template<>
class PropertyHelper<CEGUI::VerticalAlignment>
{
public:
    typedef VerticalAlignment return_type;
    typedef return_type safe_method_return_type;
    typedef VerticalAlignment pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("VerticalAlignment");

        return type;
    }

    static return_type fromString(const String& str)
    {
      if (str == "Centre")
      {
          return VA_CENTRE;
      }
      else if (str == "Bottom")
      {
          return VA_BOTTOM;
      }
      else
      {
          return VA_TOP;
      }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == VA_CENTRE)
        {
            return "Centre";
        }
        else if (val == VA_BOTTOM)
        {
            return "Bottom";
        }
        else if (val == VA_TOP)
        {
            return "Top";
        }
        else
        {
            assert(false && "Invalid vertical alignment");
            return "Centre";
        }
    }
};

/*!
\brief
    EventArgs based class that is used for objects passed to handlers triggered
    for events concerning some Element object.

\see CEGUI::Element
*/
class CEGUIEXPORT ElementEventArgs : public EventArgs
{
public:
    ElementEventArgs(Element* element):
        element(element)
    {}

    //! pointer to an Element object of relevance to the event.
    Element* element;
};

/*!
\brief A positioned and sized rectangular node in a tree graph

This class implements positioning, alignment, sizing including minimum and
maximum size constraining. In its bare essense it's an unnamed rectangular node
that may contain other unnamed rectangular nodes.

Unless you are implementing new CEGUI functionality you do NOT want to use this
class directly. You most likely want to use CEGUI::Window.

\see CEGUI::Window

\internal
    Currently only CEGUI::Window uses this but in the future Falagard might use
    it for all widget parts, this would unify much of currently repeated code.

\internal
    Methods retrieving Element (like getParentElement) have Element suffix so
    that deriving classes can easily make their getParent and return the proper
    casted type (Window* for example).
*/
class CEGUIEXPORT Element :
    public PropertySet,
    public EventSet,
    public AllocatedObject<Element>
{
public:
    //! Namespace for global events
    static const String EventNamespace;

    /** Event fired when the Element size has changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose size was changed.
     */
    static const String EventSized;
    /** Event fired when the parent of this Element has been re-sized.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element pointing to the <em>parent element</em> that
     * was resized, not the element whose parent was resized.
     */
    static const String EventParentSized;
    /** Event fired when the Element position has changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose position was changed.
     */
    static const String EventMoved;
    /** Event fired when the horizontal alignment for the element is changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose horizontal alignment
     * setting was changed.
     */
    static const String EventHorizontalAlignmentChanged;
    /** Event fired when the vertical alignment for the element is changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose vertical alignment
     * setting was changed.
     */
    static const String EventVerticalAlignmentChanged;
    /** Event fired when the rotation factor(s) for the element are changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose rotation was changed.
     */
    static const String EventRotated;
    /** Event fired when a child Element has been added.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the child element that was added.
     */
    static const String EventChildAdded;
    /** Event fired when a child element has been removed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the child element that was removed.
     */
    static const String EventChildRemoved;
    /** Event fired when the z-order of the element has changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose z order position has
     * changed.
     */
    static const String EventZOrderChanged;
    /** Event fired when the non-client setting for the Element is changed.
     * Handlers are passed a const ElementEventArgs reference with
     * ElementEventArgs::element set to the Element whose non-client setting was
     * changed.
     */
    static const String EventNonClientChanged;

    /*!
    \brief A tiny wrapper to hide some of the dirty work of rect caching

    This is used internally by CEGUI::Element and other classes, it is passed
    to the user in several methods. In those circumstances you most likely
    want the result of either the "get()" or "getFresh(..)" methods.
    */
    class CachedRectf
    {
    public:
        /*!
        \brief Function to generate fresh data that might later be cached

        If the bool is true all PixelAlignment settings will be overridden
        and no pixel alignment will take place.
        */
        typedef Rectf (Element::*DataGenerator)(bool) const;

        CachedRectf(Element const* element, DataGenerator generator):
            d_element(element),
            d_generator(generator),
            // we don't have to initialise d_cachedData here, it will get
            // regenerated and reset anyways
            d_cacheValid(false)
        {}

        /*!
        \brief Retrieves cached Rectf or generated a fresh one and caches it
        */
        inline const Rectf& get() const
        {
            if (!d_cacheValid)
            {
                regenerateCache();
            }

            return d_cachedData;
        }

        /*!
        \brief Skips all caching and calls the generator

        This method will cache the result if cache is invalid and
        alignment is not being skipped.
        */
        inline Rectf getFresh(bool skipAllPixelAlignment = false) const
        {
            // if the cache is not valid we will use this chance to regenerate it
            // of course this is only applicable if we are allowed to use pixel alignment where applicable
            if (!d_cacheValid && !skipAllPixelAlignment)
            {
                return get();
            }

            return CEGUI_CALL_MEMBER_FN(*d_element, d_generator)(skipAllPixelAlignment);
        }

        /*!
        \brief Invalidates the cached Rectf causing it to be regenerated

        The regeneration will not happen immediately, it will happen when user
        requests the data.
        */
        inline void invalidateCache() const
        {
            d_cacheValid = false;
        }

        inline bool isCacheValid() const
        {
            return d_cacheValid;
        }

        inline void regenerateCache() const
        {
            // false, since when we are caching we don't want to skip anything, we want everything to act
            // exactly as it was setup
            d_cachedData = CEGUI_CALL_MEMBER_FN(*d_element, d_generator)(false);

            d_cacheValid = true;
        }

    private:
        Element const* d_element;
        const DataGenerator d_generator;

        mutable Rectf d_cachedData;
        mutable bool  d_cacheValid;
    };

    /*!
    \brief Constructor
    */
    Element();

    /*!
    \brief Destructor
    */
    virtual ~Element();

    /*!
    \brief Retrieves parent of this element
   
    \returns
        pointer to parent or 0, 0 means that this Element is a root of
        the subtree it represents
    */
    inline Element* getParentElement() const
    {
        return d_parent;
    }

    /*!
    \brief
        Set the Element area.

        Sets the area occupied by this Element. The defined area is offset from
        one of the corners and edges of this Element's parent element (depending on alignments)
        or from the top-left corner of the display if this element has no parent
        (i.e. if it is the root element).

    \param pos
        UVector2 describing the new position of the element area. Meaning of
        position depends on currently set alignments. By default it is the
        offset from the top-left corner of widget's parent.

    \param size
        UVector2 describing the new size of the element area.

    \see UDim
    */
    virtual void setArea(const UVector2& pos, const USize& size);

    //! \overload
    inline void setArea(const UDim& xpos, const UDim& ypos,
                        const UDim& width, const UDim& height)
    {
        setArea(UVector2(xpos, ypos), USize(width, height));
    }

    //! \overload
    inline void setArea(const URect& area)
    {
        setArea(area.d_min, area.getSize());
    }

    /*!
    \brief
        Return the element's area.

        Sets the area occupied by this Element. The defined area is offset from
        one of the corners and edges of this Element's parent element (depending on alignments)
        or from the top-left corner of the display if this element has no parent
        (i.e. it is the root element).

    \return
        URect describing the rectangle of the element area.

    \see UDim
    */
    inline const URect& getArea() const
    {
        return d_area;
    }

    /*!
    \brief
        Set the element's position.

        Sets the position of the area occupied by this element. The position is offset from
        one of the corners and edges of this Element's parent element (depending on alignments)
        or from the top-left corner of the display if this element has no parent
        (i.e. it is the root element).

    \param pos
        UVector2 describing the new position of the element area.

    \see UDim
    \see Element::setArea(const UVector2& pos, const USize& size)
    */
    inline void setPosition(const UVector2& pos)
    {
        setArea_impl(pos, d_area.getSize());
    }

    //! \overload
    inline void setXPosition(const UDim& pos)
    {
        setPosition(UVector2(pos, getYPosition()));
    }

    //! \overload
    inline void setYPosition(const UDim& pos)
    {
        setPosition(UVector2(getXPosition(), pos));
    }

    /*!
    \brief
        Get the element's position.

        Sets the position of the area occupied by this element. The position is offset from
        one of the corners of this Element's parent element (depending on alignments)
        or from the top-left corner of the display if this element has no parent
        (i.e. it is the root element).

    \return
        UVector2 describing the position of the element area.

    \see UDim
    */
    inline const UVector2& getPosition() const
    {
        return d_area.getPosition();
    }

    //! \overload
    inline const UDim& getXPosition() const
    {
        return getPosition().d_x;
    }

    //! \overload
    inline const UDim& getYPosition() const
    {
        return getPosition().d_y;
    }

    /*!
    \brief
        Set the horizontal alignment.

        Modifies the horizontal alignment for the element. This setting affects
        how the element's position is interpreted relative to its parent.

    \param alignment
        One of the HorizontalAlignment enumerated values.
     */
    virtual void setHorizontalAlignment(const HorizontalAlignment alignment);

    /*!
    \brief
        Get the horizontal alignment.

        Returns the horizontal alignment for the element. This setting affects
        how the element's position is interpreted relative to its parent.

    \return
        One of the HorizontalAlignment enumerated values.
     */
    inline HorizontalAlignment getHorizontalAlignment() const
    {
        return d_horizontalAlignment;
    }

    /*!
    \brief
        Set the vertical alignment.

        Modifies the vertical alignment for the element. This setting affects
        how the element's position is interpreted relative to its parent.

    \param alignment
        One of the VerticalAlignment enumerated values.
     */
    virtual void setVerticalAlignment(const VerticalAlignment alignment);

    /*!
    \brief
        Get the vertical alignment.

        Returns the vertical alignment for the element.  This setting affects how
        the element's position is interpreted relative to its parent.

    \return
        One of the VerticalAlignment enumerated values.
     */
    inline VerticalAlignment getVerticalAlignment() const
    {
        return d_verticalAlignment;
    }

    /*!
    \brief
        Set the element's size.

        Sets the size of the area occupied by this element.

    \param size
        USize describing the new size of the element's area.

    \see UDim
    */
    inline void setSize(const USize& size)
    {
        setArea(d_area.getPosition(), size);
    }

    //! \overload
    inline void setWidth(const UDim& width)
    {
        setSize(USize(width, getSize().d_height));
    }

    //! \overload
    inline void setHeight(const UDim& height)
    {
        setSize(USize(getSize().d_width, height));
    }

    /*!
    \brief
        Get the element's size.

        Gets the size of the area occupied by this element.

    \return
        USize describing the size of the element's area.

    \see UDim
    */
    inline USize getSize() const
    {
        return d_area.getSize();
    }

    //! \overload
    inline UDim getWidth() const
    {
        return getSize().d_width;
    }

    //! \overload
    inline UDim getHeight() const
    {
        return getSize().d_height;
    }

    /*!
    \brief
        Set the element's minimum size.

        Sets the minimum size that this element's area may occupy (whether size
        changes occur by user interaction, general system operation, or by
        direct setting by client code).

    \note
        The scale component of UDim takes display size as the base.
        It is not dependent on parent element's size!

    \param size
        USize describing the new minimum size of the element's area.

    \see Element::setSize
    */
    void setMinSize(const USize& size);

    /*!
    \brief
        Get the element's minimum size.

        Gets the minimum size that this element's area may occupy (whether size
        changes occur by user interaction, general system operation, or by
        direct setting by client code).

    \return
        UVector2 describing the minimum size of the element's area.

    \see Element::setMinSize
    */
    inline const USize& getMinSize() const
    {
        return d_minSize;
    }

    /*!
    \brief
        Set the element's maximum size.

        Sets the maximum size that this element area may occupy (whether size
        changes occur by user interaction, general system operation, or by
        direct setting by client code).

    \note
        The scale component of UDim takes display size as the base.
        It is not dependent on parent element's size!

    \param size
        USize describing the new maximum size of the element's area.  Note that
        zero is used to indicate that the Element's maximum area size will be
        unbounded.

    \see Element::setSize
    */
    void setMaxSize(const USize& size);

    /*!
    \brief
        Get the element's maximum size.

        Gets the maximum size that this element area may occupy (whether size
        changes occur by user interaction, general system operation, or by
        direct setting by client code).

    \return
        UVector2 describing the maximum size of the element's area.

    \see Element::setMaxSize
     */
    inline const USize& getMaxSize() const
    {
        return d_maxSize;
    }

    /*!
    \brief Sets current aspect mode and recalculates the area rect

    \param mode the new aspect mode to set

    \see CEGUI::AspectMode
    \see CEGUI::setAspectRatio
    */
    void setAspectMode(const AspectMode mode);

    /*!
    \brief
        Retrieves currently used aspect mode

    \see Element::setAspectMode
    */
    inline AspectMode getAspectMode() const
    {
        return d_aspectMode;
    }

    /*!
    \brief
        Sets target aspect ratio

    \param ratio
        The desired ratio as width / height. For example 4.0f / 3.0f,
        16.0f / 9.0.f, ...

    \note
        This is ignored if AspectMode is AM_IGNORE.

    \see Element::setAspectMode
    */
    void setAspectRatio(const float ratio);

    /*!
    \brief
        Retrieves target aspect ratio

    \see Element::setAspectRatio
    */
    inline float getAspectRatio() const
    {
        return d_aspectRatio;
    }

    /*!
    \brief
        Sets whether this Element is pixel aligned (both position and size, basically the 4 "corners").
        
    \par Impact on the element tree
        Lets say we have Element A with child Element B, A is pixel aligned
        and it's position is 99.5, 99.5 px in screenspace. This gives us
        100, 100 px pixel aligned position.

        B's position is always relative to the pixel-aligned position of its
        parent. Say B isn't pixel-aligned and it's position is 0.5, 0.5 px.
        Its final position will be 100.5, 100.5 px in screenspace, not 100, 100 px!

        If it were pixel-aligned the final position would be 101, 101 px.

    \par Why you should pixel-align widgets
        Pixel aligning is enabled by default and for most widgets it makes
        a lot of sense and just looks better. Especially with text. However for
        HUD or decorative elements pixel aligning might make transitions less
        fluid. Feel free to experiment with the setting.
    */
    void setPixelAligned(const bool setting);

    /*!
    \brief
        Checks whether this Element is pixel aligned
        
    \see
        Element::setPixelAligned
    */
    inline bool isPixelAligned() const
    {
        return d_pixelAligned;
    }

    /*!
    \brief
        Return the element's absolute (or screen, depending on the type of the element) position in pixels.

    \return
        Vector2f object describing this element's absolute position in pixels.
    */
    inline const Vector2f& getPixelPosition() const
    {
        return getUnclippedOuterRect().get().d_min;
    }

    /*!
    \brief
        Return the element's size in pixels.

    \return
        Size object describing this element's size in pixels.
    */
    inline const Sizef& getPixelSize() const
    {
        return d_pixelSize;
    }

    /*!
    \brief Calculates this element's pixel size

    \param skipAllPixelAlignment 
        Should all pixel-alignment be skipped when calculating the pixel size?

    If you want to get the pixel size you most probably want to use the
    Element::getPixelSize method. This method skips caching and might
    impact performance!
    */
    Sizef calculatePixelSize(bool skipAllPixelAlignment = false) const;

    /*!
    \brief Return the pixel size of the parent element.

    If this element doesn't have any parent, the display size will be returned.
    This method returns a valid Sizef object in all cases.

    \return
        Size object that describes the pixel dimensions of this Element's parent
    */
    Sizef getParentPixelSize(bool skipAllPixelAlignment = false) const;

    /*!
    \brief sets rotation of this widget

    \param rotation
        A Quaternion describing the rotation

    \par Euler angles
        CEGUI used Euler angles previously. While these are easy to use and seem
        intuitive they cause Gimbal locks when animating and are overall the worse
        solution than using Quaternions. You can still use Euler angles, see
        the CEGUI::Quaternion class for more info about that.
    */
    void setRotation(const Quaternion& rotation);

    /*!
    \brief retrieves rotation of this widget

    \see Element::setRotation
    */
    inline const Quaternion& getRotation() const
    {
        return d_rotation;
    }

    /*!
    \brief
        Add the specified Element as a child of this Element.

    If the Element \a element is already attached to a different Element,
    it is detached before being added to this Element.

    \param element
        Pointer to the Element object to be added.

    \exception InvalidRequestException
        thrown if Element \a element is NULL.

    \exception InvalidRequestException
        thrown if Element \a element is "this" element

    \exception InvalidRequestException
        thrown if Element \a element is an ancestor of this Element, to prevent
        cyclic Element structures.
    */
    void addChild(Element* element);

    /*!
    \brief
        Remove the Element Element's child list.
        
    \exception InvalidRequestException
        thrown if Element \a element is NULL.

    \see
        Element::addChild
    */
    void removeChild(Element* element);

    /*!
    \brief
        return a pointer to the child element that is attached to 'this' at the
        given index.

    \param idx
        Index of the child element who's pointer should be returned.  This value
        is not bounds checked, client code should ensure that this is less than
        the value returned by getChildCount().

    \return
        Pointer to the child element currently attached at index position \a idx
    */
    inline Element* getChildElementAtIdx(size_t idx) const
    {
        return d_children[idx];
    }

    /*!
    \brief Returns number of child elements attached to this Element
    */
    inline size_t getChildCount() const
    {
        return d_children.size();
    }

    /*!
    \brief Checks whether given element is attached to this Element
    */
    bool isChild(const Element* element) const;

    /*!
    \brief Checks whether the specified Element is an ancestor of this Element

    \param element
        Pointer to the Element object to look for.

    This element itself is not its own ancestor!

    \return
        - true if \a element was found to be an ancestor (parent, or parent of
          parent, etc) of this Element.
        - false if \a element is not an ancestor of this element.
    */
    bool isAncestor(const Element* element) const;

    /*!
    \brief Set whether the Element is non-client.

    A non-client element is clipped, positioned and sized according to the
    parent element's full area as opposed to just the inner rect area used
    for normal client element.

    \param setting
        - true if the element should be clipped, positioned and sized according
        to the full area rectangle of it's parent.
        - false if the element should be clipped, positioned and sized according
        to the inner rect area of it's parent.
    */
    void setNonClient(const bool setting);

    /*!
    \brief Checks whether this element was set to be non client

    \see Element::setNonClient
    */
    inline bool isNonClient() const
    {
        return d_nonClient;
    }

    /*!
    \brief Return a Rect that describes the unclipped outer rect area of the Element

    The unclipped outer rectangle is the entire area of the element, including
    frames and other outside decorations.

    \note
        Unclipped in this context means not limited by any ancestor Element's area.

    \note
        If you take position of the result rectangle it is the same as pixel
        position of the Element in screenspace.
    */
    inline const CachedRectf& getUnclippedOuterRect() const
    {
        return d_unclippedOuterRect;
    }

    /*!
    \brief Return a Rect that describes the unclipped inner rect area of the Element

    The inner rectangle is typically an area that excludes some frame or other decorations
    that should not be touched by rendering of client clipped child elements.

    \note
        Unclipped in this context means not limited by any ancestor Element's area.

    \return
        Rect object that describes, in unclipped screen pixel co-ordinates, the
        element object's inner rect area.
    */
    inline const CachedRectf& getUnclippedInnerRect() const
    {
        return d_unclippedInnerRect;
    }

    /*!
    \brief Return a Rect that describes the unclipped area covered by the Element.

    This function can return either the inner or outer area dependant upon
    the boolean values passed in.

    \param inner
        - true if the inner rect area should be returned.
        - false if the outer rect area should be returned.

    \see Element::getUnclippedOuterRect
    \see Element::getUnclippedInnerRect
    */
    inline const CachedRectf& getUnclippedRect(const bool inner) const
    {
        return inner ? getUnclippedInnerRect() : getUnclippedOuterRect();
    }

    /*!
    \brief Return a Rect that is used by client child elements as content area

    Client content area is used for relative sizing, positioning and clipping
    of child elements that are client (their NonClient property is "false").

    \see Element::getChildContentArea
    */
    virtual const CachedRectf& getClientChildContentArea() const;

    /*!
    \brief Return a Rect that is used by client child elements as content area

    Client content area is used for relative sizing, positioning and clipping
    of child elements that are non-client (their NonClient property is "true").

    \see Element::getChildContentArea
    */
    virtual const CachedRectf& getNonClientChildContentArea() const;

    /*!
    \brief Return a Rect that is used to position and size child elements

    It is used as the reference area for positioning and its size is used for
    the scale components of position and size.
 
    \note
        By and large the area returned here will be the same as the unclipped
        inner rect (for client content) or the unclipped outer rect (for non
        client content), although certain advanced uses will require
        alternative Rects to be returned.

    \note
        The behaviour of this function is modified by overriding the
        protected Element::getClientChildContentArea and/or
        Element::getNonClientChildContentArea functions.

    \param non_client
        - true to return the non-client child content area.
        - false to return the client child content area (default).
    */
    inline const CachedRectf& getChildContentArea(const bool non_client = false) const
    {
        return non_client ? getNonClientChildContentArea() : getClientChildContentArea();
    }

    /*!
    \brief Inform the element and (optionally) all children that screen area has changed

    \note
        This will cause recomputation and recaching of various rectangles used.
        Such an action, especially if applied recursively, will impact performance
        before everything is cached again.

    \param recursive
        - true to recursively call notifyScreenAreaChanged on attached child
          Element objects.
        - false to just process \e this Element.
    */
    virtual void notifyScreenAreaChanged(bool recursive = true);

    /*!
    \brief Return the size of the root container (such as screen size).

    This is size of the hypothetical parent of the root element that has no
    parent element. Display size is usually used.

    The value is significant and is used to size and position the root if
    it is using scale UDim component in position and/or size.
    */
    virtual const Sizef& getRootContainerSize() const;

protected:
    /*!
    \brief
        Add standard CEGUI::Element properties.
    */
    void addElementProperties();

    /*!
    \brief
        Implementation method to modify element area while correctly applying
        min / max size processing, and firing any appropriate events.

    \note
        This is the implementation function for setting size and position.
        In order to simplify area management, from this point on, all
        modifications to element size and position (area rect) should come
        through here.

    \param pos
        UVector2 object describing the new area position.

    \param size
        USize object describing the new area size.

    \param topLeftSizing
        - true to indicate the the operation is a sizing operation on the top
          and/or left edges of the area, and so element movement should be
          inhibited if size is at max or min.
        - false to indicate the operation is not a strict sizing operation on
          the top and/or left edges and that the element position may change as
          required

    \param fireEvents
        - true if events should be fired as normal.
        - false to inhibit firing of events (required, for example, if you need
          to call this from the onSize/onMove handlers).
     */
    virtual void setArea_impl(const UVector2& pos, const USize& size,
                              bool topLeftSizing = false, bool fireEvents = true);

    //! helper to return whether the inner rect size has changed
    inline bool isInnerRectSizeChanged() const
    {
        const Sizef old_sz(d_unclippedInnerRect.get().getSize());
        d_unclippedInnerRect.invalidateCache();
        return old_sz != d_unclippedInnerRect.get().getSize();
    }

    /*!
    \brief
        Set the parent element for this element object.

    \param parent
        Pointer to a Element object that is to be assigned as the parent to this
        Element.

    \return
        Nothing
    */
    virtual void setParent(Element* parent);

    /*!
    \brief
        Add given element to child list at an appropriate position
    */
    virtual void addChild_impl(Element* element);

    /*!
    \brief
        Remove given element from child list
    */
    virtual void removeChild_impl(Element* element);

    //! Default implementation of function to return Element's outer rect area.
    virtual Rectf getUnclippedOuterRect_impl(bool skipAllPixelAlignment) const;
    //! Default implementation of function to return Element's inner rect area.
    virtual Rectf getUnclippedInnerRect_impl(bool skipAllPixelAlignment) const;

    //! helper to fire events based on changes to area rect
    void fireAreaChangeEvents(const bool moved, const bool sized);
    void notifyChildrenOfSizeChange(const bool non_client,
                                    const bool client);

    /*************************************************************************
        Event trigger methods
    *************************************************************************/
    /*!
    \brief
        Handler called when the element's size changes.

    \param e
        ElementEventArgs object whose 'element' pointer field is set to the element
        that triggered the event.
    */
    virtual void onSized(ElementEventArgs& e);

    /*!
    \brief
        Handler called when this element's parent element has been resized.  If
        this element is the root / GUI Sheet element, this call will be made when
        the display size changes.

    \param e
        ElementEventArgs object whose 'element' pointer field is set the the
        element that caused the event; this is typically either this element's
        parent element, or NULL to indicate the screen size has changed.
    */
    virtual void onParentSized(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the element's position changes.

    \param e
        ElementEventArgs object whose 'element' pointer field is set to the element
        that triggered the event.
    */
    virtual void onMoved(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the horizontal alignment setting for the element is
        changed.

    \param e
        ElementEventArgs object initialised as follows:
        - element field is set to point to the element object who's alignment has
          changed (typically 'this').
    */
    virtual void onHorizontalAlignmentChanged(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the vertical alignment setting for the element is
        changed.

    \param e
        ElementEventArgs object initialised as follows:
        - element field is set to point to the element object who's alignment has
          changed (typically 'this').
    */
    virtual void onVerticalAlignmentChanged(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the element's rotation is changed.

    \param e
        ElementEventArgs object whose 'element' pointer field is set to the element
        that triggered the event.
    */
    virtual void onRotated(ElementEventArgs& e);

    /*!
    \brief
        Handler called when a child element is added to this element.

    \param e
        ElementEventArgs object whose 'element' pointer field is set to the element
        that has been added.
    */
    virtual void onChildAdded(ElementEventArgs& e);

    /*!
    \brief
        Handler called when a child element is removed from this element.

    \param e
        ElementEventArgs object whose 'element' pointer field is set the element
        that has been removed.
    */
    virtual void onChildRemoved(ElementEventArgs& e);

    /*!
    \brief
        Handler called when the element's non-client setting, affecting it's
        position and size relative to it's parent is changed.

    \param e
        ElementEventArgs object whose 'element' pointer field is set to the element
        that triggered the event. For this event the trigger element is always
        'this'.
    */
    virtual void onNonClientChanged(ElementEventArgs& e);

    /*************************************************************************
        Implementation Data
    *************************************************************************/
    //! definition of type used for the list of attached child elements.
    typedef std::vector<Element*
        CEGUI_VECTOR_ALLOC(Element*)> ChildList;

    //! The list of child element objects attached to this.
    ChildList d_children;
    //! Holds pointer to the parent element.
    Element* d_parent;

    //! true if element is in non-client (outside InnerRect) area of parent.
    bool d_nonClient;

    //! This element objects area as defined by a URect.
    URect d_area;
    //! Specifies the base for horizontal alignment.
    HorizontalAlignment d_horizontalAlignment;
    //! Specifies the base for vertical alignment.
    VerticalAlignment d_verticalAlignment;
    //! current minimum size for the element.
    USize d_minSize;
    //! current maximum size for the element.
    USize d_maxSize;
    //! How to satisfy current aspect ratio
    AspectMode d_aspectMode;
    //! The target aspect ratio
    float d_aspectRatio;
    //! If true, the position and size are pixel aligned
    bool d_pixelAligned;
    //! Current constrained pixel size of the element.
    Sizef d_pixelSize;
    //! Rotation of this element (relative to the parent)
    Quaternion d_rotation;

    //! outer area rect in screen pixels
    CachedRectf d_unclippedOuterRect;
    //! inner area rect in screen pixels
    CachedRectf d_unclippedInnerRect;

private:
    /*************************************************************************
        May not copy or assign Element objects
    *************************************************************************/
    Element(const Element&);

    Element& operator=(const Element&) {return *this;}
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIElement_h_
