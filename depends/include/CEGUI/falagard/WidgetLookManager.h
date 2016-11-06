/***********************************************************************
    created:    Mon Jun 13 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFalWidgetLookManager_h_
#define _CEGUIFalWidgetLookManager_h_

#include "../Singleton.h"
#include "../String.h"
#include "../Exceptions.h"
#include "./WidgetLookFeel.h"
#include <map>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#	pragma warning(disable : 4275)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Manager class that gives top-level access to widget data based "look and feel" specifications loaded into the system.
    */
    class CEGUIEXPORT WidgetLookManager :
        public Singleton<WidgetLookManager>,
        public AllocatedObject<WidgetLookManager>
    {
    public:
        /*!
        \brief
            Constructor.
        */
        WidgetLookManager();

        /*!
        \brief
            Destructor
        */
        ~WidgetLookManager();

       	/*!
	      \brief
		        Return singleton WidgetLookManager object

	      \return
		        Singleton WidgetLookManager object
	      */
	      static	WidgetLookManager&	getSingleton(void);


	      /*!
	      \brief
		        Return pointer to singleton WidgetLookManager object

	      \return
		        Pointer to singleton WidgetLookManager object
	      */
	      static	WidgetLookManager*	getSingletonPtr(void);


        /** Typedef for a set of WidgetLookFeel names. */
        typedef std::set<String, StringFastLessCompare CEGUI_SET_ALLOC(String)> WidgetLookNameSet;

        //! Typedef for a map of Strings to WidgetLookFeel objects
        typedef std::map<String, WidgetLookFeel*, StringFastLessCompare CEGUI_MAP_ALLOC(String, WidgetLookFeel*)> WidgetLookPointerMap;

        /*!
        \brief
            Parses a file containing window look & feel specifications (in the form of XML).

        \note
            If the new file contains specifications for widget types that are already specified, it is not an error;
            the previous definitions are overwritten by the new data.  An entry will appear in the log each time any
            look & feel component is overwritten.

        \param source
            RawDataContainer containing the source code that will be parsed

        \param resourceGroup
            Resource group identifier to pass to the resource provider when loading the file.

        \return
            Nothing.

        \exception FileIOException             thrown if there was some problem accessing or parsing the file \a filename
        \exception InvalidRequestException     thrown if an invalid filename was provided.
        */
        void parseLookNFeelSpecificationFromContainer(const RawDataContainer& source);
        
        /*!
        \see WidgetLookManager::parseLookNFeelSpecificationFromContainer
        */
        void parseLookNFeelSpecificationFromFile(const String& filename, const String& resourceGroup = "");
        
        /*!
        \see WidgetLookManager::parseLookNFeelSpecificationFromContainer
        */
        void parseLookNFeelSpecificationFromString(const String& source);

        /*!
        \brief
            Return whether a WidgetLookFeel has been created with the specified name.

        \param widget
            String object holding the name of a widget look to test for.

        \return
            - true if a WidgetLookFeel named \a widget is available.
            - false if so such WidgetLookFeel is currently available.
        */
        bool isWidgetLookAvailable(const String& widget) const;


        /*!
        \brief
            Return a const reference to a WidgetLookFeel object which has the specified name.

        \param widget
            String object holding the name of a widget look that is to be returned.

        \return
            const reference to the requested WidgetLookFeel object.

        \exception UnknownObjectException   thrown if no WidgetLookFeel is available with the requested name.
        */
        const WidgetLookFeel& getWidgetLook(const String& widget) const;


        /*!
        \brief
            Erase the WidgetLookFeel that has the specified name.

        \param widget
            String object holding the name of a widget look to be erased.  If no such WidgetLookFeel exists, nothing
            happens.

        \return
            Nothing.
        */
        void eraseWidgetLook(const String& widget);

        //! erase all defined WidgetLookFeel.
        void eraseAllWidgetLooks();

        /*!
        \brief
            Add the given WidgetLookFeel.

        \note
            If the WidgetLookFeel specification uses a name that already exists within the system, it is not an error;
            the previous definition is overwritten by the new data.  An entry will appear in the log each time any
            look & feel component is overwritten.

        \param look
            WidgetLookFeel object to be added to the system.  NB: The WidgetLookFeel is copied, no change of ownership of the
            input object occurrs.

        \return
            Nothing.
        */
        void addWidgetLook(const WidgetLookFeel& look);


        /*!
        \brief
            Writes a complete WidgetLookFeel to a stream. Note that XML file header and
            Falagard opening/closing tags will also be written.

        \param name
            String holding the name of the WidgetLookFeel to be output to the stream.

        \param out_stream
            OutStream where XML data should be sent.
        */
        void writeWidgetLookToStream(const String& widgetLookName, OutStream& out_stream) const;

        /*!
        \brief
            Writes a complete WidgetLookFeel to a string. Note that XML file header and
            Falagard opening/closing tags will also be written.

        \param name
            String holding the name of the WidgetLookFeel to be output to the string.

        \return
            String containing the WidgetLook parsed to XML.
        */
        String getWidgetLookAsString(const String& widgetLookName) const;
 
        /*!
        \brief
            Writes a set WidgetLookFeels to a string. Note that XML file header and
            Falagard opening/closing tags will also be written.

        \param widgetLookNameSet
            Set of strings containing the WidgetLookFeel names to be output to the string.

        \return
            String containing the set of WidgetLookFeels parsed to XML.
        */
        String getWidgetLookSetAsString(const WidgetLookNameSet& widgetLookNameSet) const;

        /*!
        \brief
            Writes a series of complete WidgetLook objects to a stream. Note that XML file header and
            Falagard opening/closing tags will also be written.

            The \a prefix specifies a name prefix common to all widget looks to be written, you could
            specify this as "TaharezLook/" and then any defined widget look starting with that prefix, such
            as "TaharezLook/Button" and "TaharezLook/Listbox" will be written to the stream.

        \param prefix
            String holding the widget look name prefix, which will be used when searching for the widget looks
            to be output to the stream.

        \param out_stream
            OutStream where XML data should be sent.
        */
        void writeWidgetLookSeriesToStream(const String& prefix, OutStream& out_stream) const;

        /*!
        \brief
            Writes a series of complete WidgetLook objects to a stream. Note that XML file header and
            Falagard opening/closing tags will also be written.

            The \a widgetLookSet specifies a set of strings containing the names of the WidgetLookFeels
            to be written to the stream.

        \param widgetLookNameSet
            Set of strings containing the WidgetLookFeel names to be added to be written to the stream.

        \param out_stream
            OutStream where XML data should be sent.
        */
        void writeWidgetLookSetToStream(const WidgetLookNameSet& widgetLookNameSet, OutStream& out_stream) const;

        /*!
        \brief
            Returns the default resource group currently set for LookNFeels.

        \return
            String describing the default resource group identifier that will be
            used when loading LookNFeel data.
        */
        static const String& getDefaultResourceGroup()
            { return d_defaultResourceGroup; }

        /*!
        \brief
            Sets the default resource group to be used when loading LookNFeel data

        \param resourceGroup
            String describing the default resource group identifier to be used.

        \return
            Nothing.
        */
        static void setDefaultResourceGroup(const String& resourceGroup)
            { d_defaultResourceGroup = resourceGroup; }

        /*!
        \brief
            Returns a map containing Strings to WidgetLookFeel pointers. The map contains pointers to the WidgetLookFeels
            that were added to this Manager.

        \return
            A map of Strings to WidgetLookFeel pointers.
        */
        WidgetLookPointerMap getWidgetLookPointerMap();

    private:
        //! Name of schema file used for XML validation.
        static const String FalagardSchemaName; 
        //! holds default resource group
        static String d_defaultResourceGroup;


        //! Typedef for a map of Strings to WidgetLookFeel objects
        // \deprecated  Will use the correct allocator in the next version and will
        // be renamed to "WidgetLookMap"
        typedef std::map<String, WidgetLookFeel, StringFastLessCompare> WidgetLookList;

        //! List of WidgetLookFeels added to this Manager
        WidgetLookList  d_widgetLooks;  

    public:
        //! \deprecated Use WidgetLookPointerMap instead, which provides direct access to the added elements.
        typedef ConstMapIterator<WidgetLookList> WidgetLookIterator;
        //! \deprecated Use getWidgetLookPointerMap instead, which provides direct access to the added elements. In the next version getWidgetLookMap will be added to replace the const-ness.
        WidgetLookIterator getWidgetLookIterator() const;
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIFalWidgetLookManager_h_
