/***********************************************************************
    created:    Sat Mar 12 2005
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
#ifndef _CEGUIXMLAttributes_h_
#define _CEGUIXMLAttributes_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include <map>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Class representing a block of attributes associated with an XML element.
     */
    class CEGUIEXPORT XMLAttributes :
        public AllocatedObject<XMLAttributes>
    {
    public:
        /*!
        \brief
            XMLAttributes constructor.
         */
        XMLAttributes(void);

        /*!
        \brief
            XMLAttributes Destructor
         */
        virtual ~XMLAttributes(void);

        /*!
        \brief
            Adds an attribute to the attribute block.  If the attribute value already exists, it is replaced with
            the new value.

        \param attrName
            String object holding the name of the attribute to be added.

        \param attrValue
            String object holding a string representation of the attribute value.

        \return
            Nothing.
         */
        void add(const String& attrName, const String& attrValue);
        
        /*!
        \brief
            Removes an attribute from the attribute block.

        \param attrName
            String object holding the name of the attribute to be removed.

        \return
            Nothing.
         */
        void remove(const String& attrName);

        /*!
        \brief
            Return whether the named attribute exists within the attribute block.

        \param attrName
            String object holding the name of the attribute to be checked.

        \return
            - true if an attribute with the name \a attrName is present in the attribute block.
            - false if no attribute named \a attrName is present in the attribute block.
        */
        bool exists(const String& attrName) const;

        /*!
        \brief
            Return the number of attributes in the attribute block.

        \return
            value specifying the number of attributes in this attribute block.
         */
        size_t getCount(void) const;

        /*!
        \brief
            Return the name of an attribute based upon its index within the attribute block.

        \note
            Nothing is specified about the order of elements within the attribute block.  Elements
            may not, for example, appear in the order they were specified in the XML file.

        \param index
            zero based index of the attribute whos name is to be returned.

        \return
            String object holding the name of the attribute at the requested index.

        \exception IllegalRequestException  thrown if \a index is out of range for this attribute block.
        */
        const String& getName(size_t index) const;
        
        /*!
        \brief
            Return the value string of an attribute based upon its index within the attribute block.

        \note
            Nothing is specified about the order of elements within the attribute block.  Elements
            may not, for example, appear in the order they were specified in the XML file.
        
        \param index
            zero based index of the attribute whos value string is to be returned.

        \return
            String object holding the string value of the attribute at the requested index.

        \exception IllegalRequestException  thrown if \a index is out of range for this attribute block.
        */
        const String& getValue(size_t index) const;

        /*!
        \brief
            Return the value string for attribute \a attrName.

        \param attrName
            String object holding the name of the attribute whos value string is to be returned

        \return
            String object hilding the value string for attribute \a attrName.

        \exception UnknownObjectException   thrown if no attribute named \a attrName is present in the attribute block.
         */
        const String& getValue(const String& attrName) const;

        /*!
        \brief
            Return the value of attribute \a attrName as a string.

        \param attrName
            String object holding the name of the attribute whos value is to be returned.

        \param def
            String object holding the default value to be returned if \a attrName does not exist in the attribute block.
            For some parsers, defaults can be gotten from schemas and such like, though for others this may not be desired
            or possible, so this parameter is used to ensure a default is available in the abscence of other mechanisms.

        \return
            String object containing the value of attribute \a attrName if present, or \a def if not.
         */
        const String& getValueAsString(const String& attrName, const String& def = "") const;

        /*!
        \brief
            Return the value of attribute \a attrName as a boolean value.

        \param attrName
            String object holding the name of the attribute whos value is to be returned.

        \param def
            bool value specifying the default value to be returned if \a attrName does not exist in the attribute block.
            For some parsers, defaults can be gotten from schemas and such like, though for others this may not be desired
            or possible, so this parameter is used to ensure a default is available in the abscence of other mechanisms.

        \return
            bool value equal to the value of attribute \a attrName if present, or \a def if not.

        \exception IllegalRequestException  thrown if the attribute value string coul dnot be converted to the requested type.
        */
        bool getValueAsBool(const String& attrName, bool def = false) const;
        
        /*!
        \brief
            Return the value of attribute \a attrName as a integer value.

        \param attrName
            String object holding the name of the attribute whos value is to be returned.

        \param def
            integer value specifying the default value to be returned if \a attrName does not exist in the attribute block.
            For some parsers, defaults can be gotten from schemas and such like, though for others this may not be desired
            or possible, so this parameter is used to ensure a default is available in the abscence of other mechanisms.

        \return
            integer value equal to the value of attribute \a attrName if present, or \a def if not.
        
        \exception IllegalRequestException  thrown if the attribute value string coul dnot be converted to the requested type.
        */
        int getValueAsInteger(const String& attrName, int def = 0) const;
        
        /*!
        \brief
            Return the value of attribute \a attrName as a floating point value.

        \param attrName
            String object holding the name of the attribute whos value is to be returned.

        \param def
            float value specifying the default value to be returned if \a attrName does not exist in the attribute block.
            For some parsers, defaults can be gotten from schemas and such like, though for others this may not be desired
            or possible, so this parameter is used to ensure a default is available in the abscence of other mechanisms.

        \return
            float value equal to the value of attribute \a attrName if present, or \a def if not.

        \exception IllegalRequestException  thrown if the attribute value string coul dnot be converted to the requested type.
        */
        float getValueAsFloat(const String& attrName, float def = 0.0f) const;

    protected:
        typedef std::map<String, String, StringFastLessCompare> AttributeMap;
        AttributeMap    d_attrs;
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIXMLAttributes_h_
