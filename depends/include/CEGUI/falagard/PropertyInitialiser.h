/***********************************************************************
    created:    Mon Jun 13 2005
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
#ifndef _CEGUIFalPropertyInitialiser_h_
#define _CEGUIFalPropertyInitialiser_h_

#include "../PropertySet.h"


// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Class that holds information about a property and it's required initial value.
    */
    class CEGUIEXPORT PropertyInitialiser :
        public AllocatedObject<PropertyInitialiser>
    {
    public:
        PropertyInitialiser() {};
        /*!
        \brief
            Constructor

        \param property
            String holding the name of the property targetted by this PropertyInitialiser.

        \param value
            String holding the value to be set by this PropertyInitialiser.
        */
        PropertyInitialiser(const String& property, const String& value);

        /*!
        \brief
            Apply this property initialiser to the specified target CEGUI::PropertySet object.

        \param target
            CEGUI::PropertySet object to be initialised by this PropertyInitialiser.

        \return
            Nothing.
        */
        void apply(PropertySet& target) const;
        /*!
        \brief
            Sets the name of the property targetted by this PropertyInitialiser.

        \param name
            String object holding the name of the target property.

        \return
            Nothing.
        */
        void setTargetPropertyName(const String& name);

        /*!
        \brief
            Return the name of the property targetted by this PropertyInitialiser.

        \return
            String object holding the name of the target property.
        */
        const String& getTargetPropertyName() const;

        /*!
        \brief
            Sets the value string to be set on the property targetted by this PropertyInitialiser.

        \return
            String object holding the value string.

        \return
            Nothing.
        */
        void setInitialiserValue(const String& value);

        /*!
        \brief
            Return the value string to be set on the property targetted by this PropertyInitialiser.

        \return
            String object holding the value string.
        */
        const String& getInitialiserValue() const;

        /*!
        \brief
            Writes an xml representation of this PropertyInitialiser to \a out_stream.

        \param xml_stream
            Stream where xml data should be output.


        \return
            Nothing.
        */
        void writeXMLToStream(XMLSerializer& xml_stream) const;

    private:
        CEGUI::String   d_propertyName;     //!< Name of a property to be set.
        CEGUI::String   d_propertyValue;    //!< Value string to be set on the property.
    };

} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIFalPropertyInitialiser_h_
