/***********************************************************************
    created:    Mon Jun 13 2005
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
#ifndef _CEGUIFalDimensions_h_
#define _CEGUIFalDimensions_h_

#include "./Enums.h"
#include "../String.h"
#include "../UDim.h"
#include "../Rect.h"
#include "../XMLSerializer.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Abstract interface for a generic 'dimension' class.
class CEGUIEXPORT BaseDim :
    public AllocatedObject<BaseDim>
{
public:
    BaseDim();

    virtual ~BaseDim();

    /*!
    \brief
        Return a value that represents this dimension as absolute pixels.

    \param wnd
        Window object that may be used by the specialised class to aid in
        calculating the final value.

    \return
        float value which represents, in pixels, the same value as this BaseDim.
    */
    virtual float getValue(const Window& wnd) const = 0;

    /*!
    \brief
        Return a value that represents this dimension as absolute pixels.

    \param wnd
        Window object that may be used by the specialised class to aid in
        calculating the final value (typically would be used to obtain
        window/widget dimensions).

    \param container
        Rect object which describes an area to be considered as the base area
        when calculating the final value.  Basically this means that relative
        values are calculated from the dimensions of this Rect.

    \return
        float value which represents, in pixels, the same value as this BaseDim.
    */
    virtual float getValue(const Window& wnd, const Rectf& container) const = 0;

    /*!
    \brief
        Create an exact copy of the specialised object and return it as a
        pointer to a BaseDim object.

        Since the system needs to be able to copy objects derived from BaseDim,
        but only has knowledge of the BaseDim interface, this clone method is
        provided to prevent slicing issues.
    */
    virtual BaseDim* clone() const = 0;

    /*!
    \brief
        Writes an xml representation of this BaseDim to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.
    */
    virtual void writeXMLToStream(XMLSerializer& xml_stream) const;

    //! perform any processing required due to the given font having changed.
    virtual bool handleFontRenderSizeChange(Window& window,
                                            const Font* font) const;
protected:
    //! Implementataion method to output real xml element name.
    virtual void writeXMLElementName_impl(XMLSerializer& xml_stream) const = 0;

    //! Implementataion method to create the element attributes
    virtual void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const = 0;
};

/*!
\brief
    Dimension type that represents the result of an operation performed on
    two other dimension values. Implements BaseDim interface.
*/
class CEGUIEXPORT OperatorDim : public BaseDim
{
public:
    OperatorDim();
    OperatorDim(DimensionOperator op);
    OperatorDim(DimensionOperator op, BaseDim* left, BaseDim* right);
    ~OperatorDim();

    //! set the left hand side operand (passed object is cloned)
    void setLeftOperand(const BaseDim* operand);

    //! return pointer to the left hand side operand
    BaseDim* getLeftOperand() const;

    //! set the right hand side operand (passed object is cloned)
    void setRightOperand(const BaseDim* operand);

    //! return pointer to the right hand side operand
    BaseDim* getRightOperand() const;

    //! Set the operation to be performed
    void setOperator(DimensionOperator op);

    //! Get the current operation that will be performed
    DimensionOperator getOperator() const;

    //! helper to set the next free operand, will throw after 2 are set
    void setNextOperand(const BaseDim* operand);

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;
    BaseDim* clone() const;

protected:
    float getValueImpl(const float lval, const float rval) const;
    // Implementation of the base class interface
    void writeXMLToStream(XMLSerializer& xml_stream) const;
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

    BaseDim* d_left;
    BaseDim* d_right;
    DimensionOperator d_op;
};

/*!
\brief
    Dimension type that represents an absolute pixel value.
    Implements BaseDim interface.
*/
class CEGUIEXPORT AbsoluteDim : public BaseDim
{
public:
    AbsoluteDim() {};
    AbsoluteDim(float val);

    //! Get the current value of the AbsoluteDim.
    float getBaseValue() const;

    //! Set the current value of the AbsoluteDim.
    void setBaseValue(float val);

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;
    BaseDim* clone() const;

protected:
    // Implementation of the base class interface
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

private:
    //! holds pixel value for the AbsoluteDim.
    float d_val;
};

/*!
\brief
    Dimension type that represents some dimension of an Image.
    Implements BaseDim interface.
*/
class CEGUIEXPORT ImageDimBase : public BaseDim
{
public:
    ImageDimBase() {};

    /*!
    \brief
        Constructor.

    \param dim
        DimensionType value indicating which dimension of an Image that
        this ImageDim is to represent.
    */
    ImageDimBase(DimensionType dim);

    /*!
    \brief
        Gets the source dimension type for this WidgetDim.

    \return
        DimensionType value indicating which dimension of the described
        image that this WidgetDim is to represent.
    */
    DimensionType getSourceDimension() const;

    /*!
    \brief
        Sets the source dimension type for this ImageDim.

    \param dim
        DimensionType value indicating which dimension of the described
        image that this ImageDim is to represent.
    */
    void setSourceDimension(DimensionType dim);

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;

protected:
    //! return the image instance to access
    virtual const Image* getSourceImage(const Window& wnd) const = 0;

    // Implementation of the base class interface
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

    //! the dimension of the image that we are to represent.
    DimensionType d_what;
};

//! ImageDimBase subclass that accesses an image by its name.
class CEGUIEXPORT ImageDim : public ImageDimBase
{
public:
    ImageDim() {};
    ImageDim(const String& image_name, DimensionType dim);

    //! return the name of the image accessed by this ImageDim.
    const String& getSourceImage() const;
    //! set the name of the image accessed by this ImageDim.
    void setSourceImage(const String& image_name);

    // Implementation of the base class interface
    BaseDim* clone() const;

protected:
    // Implementation / overrides of functions in superclasses
    const Image* getSourceImage(const Window& wnd) const;
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

    //! name of the Image.
    String d_imageName;
};

//! ImageDimBase subclass that accesses an image fetched via a property.
class CEGUIEXPORT ImagePropertyDim : public ImageDimBase
{
public:
    ImagePropertyDim() {};

    /*!
    \brief
        Constructor.

    \param property_name
        String holding the name of the property on the target that will be
        accessed to retrieve the name of the image to be accessed by the
        ImageDim.

    \param dim
        DimensionType value indicating which dimension of an Image that
        this ImageDim is to represent.
    */
    ImagePropertyDim(const String& property_name, DimensionType dim);

    //! return the name of the property accessed by this ImagePropertyDim.
    const String& getSourceProperty() const;
    //! set the name of the property accessed by this ImagePropertyDim.
    void setSourceProperty(const String& property_name);

    // Implementation of the base class interface
    BaseDim* clone() const;

protected:
    // Implementation / overrides of functions in superclasses
    const Image* getSourceImage(const Window& wnd) const;
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

    //! name of the property from which to fetch the image name.
    String d_propertyName;
};

/*!
\brief
    Dimension type that represents some dimension of a Window/widget.
    Implements BaseDim interface.

    When calculating the final pixel value for the dimension, a target widget
    name is built by appending the name specified in the WidgetDim to the name
    path of the window passed to getValue, we then use the window/widget with
    that name to obtain the value for the dimension.
*/
class CEGUIEXPORT WidgetDim : public BaseDim
{
public:
    WidgetDim() {};
    /*!
    \brief
        Constructor.

    \param name
        String object holding the name suffix for a window/widget.

    \param dim
        DimensionType value indicating which dimension of the described image that this ImageDim
        is to represent.
    */
    WidgetDim(const String& name, DimensionType dim);

    /*!
    \brief
        Get the name suffix to use for this WidgetDim.

    \return
        String object holding the name suffix for a window/widget.
    */
    const String& getWidgetName() const;

    /*!
    \brief
        Set the name suffix to use for this WidgetDim.

    \param name
        String object holding the name suffix for a window/widget.

    \return
        Nothing.
    */
    void setWidgetName(const String& name);

    /*!
    \brief
        Gets the source dimension type for this WidgetDim.

    \return
        DimensionType value indicating which dimension of the described image
        that this WidgetDim is to represent.
    */
    DimensionType getSourceDimension() const;

    /*!
    \brief
        Sets the source dimension type for this WidgetDim.

    \param dim
        DimensionType value indicating which dimension of the described image
        that this WidgetDim is to represent.
    */
    void setSourceDimension(DimensionType dim);

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;
    BaseDim* clone() const;

protected:
    // Implementation of the base class interface
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

private:
    //! Holds target window name suffix.
    String d_widgetName;
    //! the dimension of the target window that we are to represent.
    DimensionType d_what;
};

/*!
\brief
    Dimension type that represents an Unified dimension.
    Implements BaseDim interface.
*/
class CEGUIEXPORT UnifiedDim : public BaseDim
{
public:
    UnifiedDim(){};
    /*!
    \brief
        Constructor.

    \param value
        UDim holding the value to assign to this UnifiedDim.

    \param dim
        DimensionType value indicating what this UnifiedDim is to represent.  This is required
        because we need to know what part of the base Window that the UDim scale component is
        to operate against.
    */
    UnifiedDim(const UDim& value, DimensionType dim);

    //! Get the current value of the UnifiedDim.
    const UDim& getBaseValue() const;

    //! Set the current value of the UnifiedDim.
    void setBaseValue(const UDim& val);

    /*!
    \brief
        Gets the source dimension type for this WidgetDim.

    \return
        DimensionType value indicating which dimension should be used as the
        scale reference / base value when calculating the pixel value of this
        dimension.
    */
    DimensionType getSourceDimension() const;

    /*!
    \brief
        Sets the source dimension type for this WidgetDim.

    \param dim
        DimensionType value indicating which dimension should be used as the
        scale reference / base value when calculating the pixel value of this
        dimension.
    */
    void setSourceDimension(DimensionType dim);

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;
    BaseDim* clone() const;

protected:
    // Implementation of the base class interface
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

private:
    //! The UDim value.
    UDim d_value;
    //! what to use as the base / reference for scale.
    DimensionType d_what;
};

/*!
\brief
    Dimension type that represents some metric of a Font.
    Implements BaseDim interface.
*/
class CEGUIEXPORT FontDim : public BaseDim
{
public:
    FontDim() {};
    /*!
    \brief
        Constructor.

    \param name
        String holding the name suffix of the window to be accessed to obtain
        the font and / or text strings to be used when these items are not
        explicitly given.

    \param font
        String holding the name of the font to use for this dimension.  If the
        string is empty, the font assigned to the window passed to getValue will
        be used.

    \param text
        String holding the text to be measured for horizontal extent. If this
        is empty, the text from the window passed to getValue will be used.

    \param metric
        One of the FontMetricType values indicating what we should represent.

    \param padding
        constant pixel padding value to be added.
    */
    FontDim(const String& name, const String& font, const String& text,
            FontMetricType metric, float padding = 0);

    //! Get the current name of the FontDim.
    const String& getName() const;

    //! Set the current name of the FontDim.
    void setName(const String& name);

    //! Get the current font of the FontDim.
    const String& getFont() const;

    //! Set the current font of the FontDim.
    void setFont(const String& font);

    //! Get the current text of the FontDim.
    const String& getText() const;

    //! Set the current text of the FontDim.
    void setText(const String& text);

    //! Get the current metric of the FontDim.
    FontMetricType getMetric() const;

    //! Set the current metric of the FontDim.
    void setMetric(FontMetricType metric);

    //! Get the current padding of the FontDim.
    float getPadding() const;

    //! Set the current padding of the FontDim.
    void setPadding(float padding);

    // overridden from BaseDim.
    bool handleFontRenderSizeChange(Window& window,
                                    const Font* font) const;

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;
    BaseDim* clone() const;

protected:
    // Implementation of the base class interface
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

    const Font* getFontObject(const Window& window) const;

private:
    //! Name of Font.  If empty font will be taken from Window.
    String d_font;
    //! String to measure for extents, if empty will use window text.
    String d_text;
    //! String to hold the name of the window to use for fetching missing font and/or text.
    String d_childName;
    //! what metric we represent.
    FontMetricType d_metric;
    //! padding value to be added.
    float d_padding;
};

/*!
\brief
    Dimension type that represents the value of a Window property.
    Implements BaseDim interface.
*/
class CEGUIEXPORT PropertyDim : public BaseDim
{
public:
    PropertyDim() {};
    /*!
    \brief
        Constructor.

    \param name
        String holding the name suffix of the window on which the property
        is to be accessed.

    \param property
        String object holding the name of the property this PropertyDim
        represents the value of.  The property named should represent either
        a UDim value or a simple float value - dependning upon what \a type
        is specified as.

    \param type
        DimensionType value indicating what dimension named property
        represents.  The possible DimensionType values are as follows:
        - DT_INVALID the property should represent a simple float value.
        - DT_WIDTH the property should represent a UDim value where the
        scale is relative to the targetted Window's width.
        - DT_HEIGHT the property should represent a UDim value where the
        scale is relative to the targetted Window's height.
        - All other values will cause an InvalidRequestException exception
        to be thrown.
    */
    PropertyDim(const String& name, const String& property, DimensionType type);
    /*!
    \brief
        Get the name suffix to use for this WidgetDim.

    \return
        String object holding the name suffix for a window/widget.
    */
    const String& getWidgetName() const;

    /*!
    \brief
        Set the name suffix to use for this WidgetDim.

    \param name
        String object holding the name suffix for a window/widget.

    \return
        Nothing.
    */
    void setWidgetName(const String& name);

    /*!
    \brief
        Get the name of the property to use for this WidgetDim.

    \return
        String object holding the name of the property.
    */
    const String& getPropertyName() const;

    /*!
    \brief
        Set the name of the property to use for this WidgetDim.

    \param property
        String object holding the name of the property.

    \return
        Nothing.
    */
    void setPropertyName(const String& property);

    /*!
    \brief
        Gets the source dimension type for this WidgetDim.

    \return
        DimensionType value indicating which dimension of the target window to
        use as the reference / base value when accessing a property that
        represents a unified dimension:
            - DT_INVALID if the property does not represent a unified dim.
            - DT_WIDTH to use target width as reference value.
            - DT_HEIGHT to use target hight as reference value.
    */
    DimensionType getSourceDimension() const;

    /*!
    \brief
        Sets the source dimension type for this WidgetDim.

    \param dim
        DimensionType value indicating which dimension of the target window to
        use as the reference / base value when accessing a property that
        represents a unified dimension:
            - DT_INVALID if the property does not represent a unified dim.
            - DT_WIDTH to use target width as reference value.
            - DT_HEIGHT to use target hight as reference value.
    */
    void setSourceDimension(DimensionType dim);

    // Implementation of the base class interface
    float getValue(const Window& wnd) const;
    float getValue(const Window& wnd, const Rectf& container) const;
    BaseDim* clone() const;

protected:
    // Implementation of the base class interface
    void writeXMLElementName_impl(XMLSerializer& xml_stream) const;
    void writeXMLElementAttributes_impl(XMLSerializer& xml_stream) const;

private:
    //! Propery that this object represents.
    String d_property;
    //! String to hold the name of the child to access the property form.
    String d_childName;
    //! String to hold the type of dimension
    DimensionType d_type;
};

/*!
\brief
    Class representing some kind of dimension.

    The key thing to understand about Dimension is that it contains not just a
    dimensional value, but also a record of what the dimension value is supposed
    to represent. (e.g. a co-ordinate on the x axis, or the height of something).
*/
class CEGUIEXPORT Dimension :
    public AllocatedObject<Dimension>
{
public:
    Dimension();
    ~Dimension();
    Dimension(const Dimension& other);
    Dimension& operator=(const Dimension& other);

    /*!
    \brief
        Constructor

    \param dim
        object based on subclass of BaseDim which holds the dimensional value.

    \param type
        DimensionType value indicating what dimension this object is to represent.
    */
    Dimension(const BaseDim& dim, DimensionType type);

    /*!
    \brief
        return the BaseDim object currently used as the value for this Dimension.

    \return
        const reference to the BaseDim sub-class object which contains the value
        for this Dimension.
    */
    const BaseDim& getBaseDimension() const;

    /*!
    \brief
        set the current value for this Dimension.

    \param dim
        object based on a subclass of BaseDim which holds the dimensional value.
    */
    void setBaseDimension(const BaseDim& dim);

    /*!
    \brief
        Return a DimensionType value indicating what this Dimension represents.

    \return
        one of the DimensionType enumerated values.
    */
    DimensionType getDimensionType() const;

    /*!
    \brief
        Sets what this Dimension represents.

    \param type
        one of the DimensionType enumerated values.
    */
    void setDimensionType(DimensionType type);

    /*!
    \brief
        Writes an xml representation of this Dimension to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.
    */
    void writeXMLToStream(XMLSerializer& xml_stream) const;

    //! perform any processing required due to the given font having changed.
    bool handleFontRenderSizeChange(Window& window,
                                    const Font* font) const;

private:
    //! Pointer to the value for this Dimension.
    BaseDim* d_value;
    //! What we represent.
    DimensionType d_type;
};

/*!
\brief
    Class that represents a target area for a widget or imagery component.

    This is essentially a Rect built out of Dimension objects.  Of note is that
    what would normally be the 'right' and 'bottom' edges may alternatively
    represent width and height depending upon what the assigned Dimension(s)
    represent.
*/
class CEGUIEXPORT ComponentArea :
    public AllocatedObject<ComponentArea>
{
public:
    ComponentArea();

    /*!
    \brief
        Return a Rect describing the absolute pixel area represented by this
        ComponentArea.

    \param wnd
        Window object to be used when calculating final pixel area.

    \return
        Rectf object describing the pixels area represented by this ComponentArea
        when using \a wnd as a reference for calculating the final pixel
        dimensions.
    */
    Rectf getPixelRect(const Window& wnd) const;

    /*!
    \brief
        Return a Rectd describing the absolute pixel area represented by this
        ComponentArea.

    \param wnd
        Window object to be used when calculating final pixel area.

    \param container
        Rect object to be used as a base or container when converting relative
        dimensions.

    \return
        Rectf object describing the pixels area represented by this
        ComponentArea when using \a wnd and \a container as a reference for
        calculating the final pixel dimensions.
    */
    Rectf getPixelRect(const Window& wnd, const Rectf& container) const;

    /*!
    \brief
        Writes an xml representation of this ComponentArea to \a out_stream.

    \param xml_stream
        XMLSerializer where xml data should be output.
    */
    void writeXMLToStream(XMLSerializer& xml_stream) const;

    /*!
    \brief
        Return whether this ComponentArea fetches it's area via a property on
        the target window.

    \return
        - true if the area comes via a Propery.
        - false if the area is not sourced from a Property.
    */
    bool isAreaFetchedFromProperty() const;

    /*!
    \brief
        Return the name of the property that will be used to determine the pixel
        area for this ComponentArea.

    \return
        String object holding the name of a Propery.
    */
    const String& getAreaPropertySource() const;

    /*!
    \brief
        Set the name of the property that will be used to determine the pixel
        area for this ComponentArea.

    \note
        Calling this will replace any existing souce, such as a named area.

    \param property
        String object holding the name of a Propery.  The property should access
        a URect type property.
    */
    void setAreaPropertySource(const String& property);

    /*!
    \brief
        Return the name of the look to which the NamedArea, which is defined as source, belongs to.

    \return
        A string containing the name of the look.
    */
    const String& getNamedAreaSourceLook() const;

    //! Set the named area source of the ComponentArea.
    void setNamedAreaSouce(const String& widget_look, const String& area_name);

    /*!
    \brief
        Return whether this ComponentArea fetches it's area via a named area
        defined.

    \return
        - true if the area comes via a named area defined in a WidgetLook.
        - false if the area is not sourced from a named area.
    */
    bool isAreaFetchedFromNamedArea() const;

    //! perform any processing required due to the given font having changed.
    bool handleFontRenderSizeChange(Window& window, const Font* font) const;

    //! Left edge of the area.
    Dimension d_left;
    //! Top edge of the area.
    Dimension d_top;
    //! Either the right edge or the width of the area.
    Dimension d_right_or_width;
    //! Either the bototm edge or the height of the area.
    Dimension d_bottom_or_height;

private:
    //! name of property or named area: must access a URect style value.
    String d_namedSource;
    //! name of widget look holding the named area to fetch
    String d_namedAreaSourceLook;
};

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIFalDimensions_h_

