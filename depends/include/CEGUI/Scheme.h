/***********************************************************************
	created:	21/2/2004
	author:		Paul D Turner
	
	purpose:	Defines abstract base class for the GUI Scheme object.
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
#ifndef _CEGUIScheme_h_
#define _CEGUIScheme_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/SchemeManager.h"


#include <vector>


#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	A class that groups a set of GUI elements and initialises the system to access those elements.

	A GUI Scheme is a high-level construct that loads and initialises various lower-level objects
	and registers them within the system for usage.  So, for example, a Scheme might create some
	Imageset objects, some Font objects, and register a collection of WindowFactory objects within
	the system which would then be in a state to serve those elements to client code.
*/
class CEGUIEXPORT Scheme :
    public AllocatedObject<Scheme>
{
private:
    friend class Scheme_xmlHandler;
    
    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructs an empty scheme object with the specified name.

    \param name
        String object holding the name of the Scheme object.
    */
    Scheme(const String& name);
    
public:
    /*!
    \brief
        Destroys a Scheme object

    \internal
        has to be public for luabind compatibility
    */
    ~Scheme(void);
    
	/*!
	\brief
		Loads all resources for this scheme.

	\return
		Nothing.
	*/
	void	loadResources(void);


	/*!
	\brief
		Unloads all resources for this scheme.  This should be used very carefully.

	\return
		Nothing.
	*/
	void	unloadResources(void);


	/*!
	\brief
		Return whether the resources for this Scheme are all loaded.

	\return
		true if all resources for the Scheme are loaded and available, or false of one or more resource is not currently loaded.
	*/
	bool	resourcesLoaded(void) const;


	/*!
	\brief
		Return the name of this Scheme.

	\return
		String object containing the name of this Scheme.
	*/
	const String& getName(void) const		{return d_name;}

    /*!
    \brief
        Returns the default resource group currently set for Schemes.

    \return
        String describing the default resource group identifier that will be
        used when loading Scheme xml file data.
    */
    static const String& getDefaultResourceGroup()
        { return d_defaultResourceGroup; }

    /*!
    \brief
        Sets the default resource group to be used when loading scheme xml data

    \param resourceGroup
        String describing the default resource group identifier to be used.

    \return
        Nothing.
    */
    static void setDefaultResourceGroup(const String& resourceGroup)
        { d_defaultResourceGroup = resourceGroup; }

    /*!
    \brief
        Load all XML based imagesets required by the scheme.
    */
    void loadXMLImagesets();

    /*!
    \brief
        Load all image file based imagesets required by the scheme.
    */
    void loadImageFileImagesets();

    /*!
    \brief
        Load all xml based fonts required by the scheme.
    */
    void loadFonts();

    /*!
    \brief
        Load all xml looknfeel files required by the scheme.
    */
    void loadLookNFeels();

    /*!
    \brief
        Register all window factories required by the scheme.
    */
    void loadWindowFactories();

    /*!
    \brief
        Register all window renderer factories required by the scheme.
    */
    void loadWindowRendererFactories();

    /*!
    \brief
        Register all factory aliases required by the scheme.
    */
    void loadFactoryAliases();

    /*!
    \brief
        Create all falagard mappings required by the scheme.
    */
    void loadFalagardMappings();

    /*!
    \brief
        Unload all XML based imagesets created by the scheme.
    */
    void unloadXMLImagesets();

    /*!
    \brief
        Unload all image file based imagesets created by the scheme.
    */
    void unloadImageFileImagesets();

    /*!
    \brief
        Unload all xml based fonts created by the scheme.
    */
    void unloadFonts();

    /*!
    \brief
        Unload all xml looknfeel files loaded by the scheme.
    */
    void unloadLookNFeels();

    /*!
    \brief
        Unregister all window factories registered by the scheme.
    */
    void unloadWindowFactories();

    /*!
    \brief
        Unregister all window renderer factories registered by the scheme.
    */
    void unloadWindowRendererFactories();

    /*!
    \brief
        Unregister all factory aliases created by the scheme.
    */
    void unloadFactoryAliases();

    /*!
    \brief
        Unregister all falagard mappings created by the scheme.
    */
    void unloadFalagardMappings();

    /*!
    \brief
        Check state of all XML based imagesets created by the scheme.
    */
    bool areXMLImagesetsLoaded() const;

    /*!
    \brief
        Check state of all image file based imagesets created by the scheme.
    */
    bool areImageFileImagesetsLoaded() const;

    /*!
    \brief
        Check state of all xml based fonts created by the scheme.
    */
    bool areFontsLoaded() const;

    /*!
    \brief
        Check state of all looknfeel files loaded by the scheme.
    */
    bool areLookNFeelsLoaded() const;

    /*!
    \brief
        Check state of all window factories registered by the scheme.
    */
    bool areWindowFactoriesLoaded() const;

    /*!
    \brief
        Check state of all window renderer factories registered by the scheme.
    */
    bool areWindowRendererFactoriesLoaded() const;

    /*!
    \brief
        Check state of all factory aliases created by the scheme.
    */
    bool areFactoryAliasesLoaded() const;

    /*!
    \brief
        Check state of all falagard mappings created by the scheme.
    */
    bool areFalagardMappingsLoaded() const;
    
    /*!
    \brief
        A single file reference to a font, imageset or so to be loaded as part of this Scheme
        
    \note
        This is public because you might want to iterate over these and load them yourself!
    */
    struct LoadableUIElement
    {
        String  name;
        String  filename;
        String  resourceGroup;
    };
    
private:
    //! \internal This is implementation specific so we keep it private!
    typedef std::vector<LoadableUIElement
        CEGUI_VECTOR_ALLOC(LoadableUIElement)>      LoadableUIElementList;

public:    
    typedef ConstVectorIterator<LoadableUIElementList> LoadableUIElementIterator;
    
    /*!
    \brief
        Retrieves iterator for all references to XML imagesets that are to be loaded with this Scheme
    */
    LoadableUIElementIterator getXMLImagesets() const;
    
    /*!
    \brief
        Retrieves iterator for all references to image file imagesets that are to be loaded with this Scheme
    */
    LoadableUIElementIterator getImageFileImagesets() const;
    
    /*!
    \brief
        Retrieves iterator for all references to font files that are to be loaded with this Scheme
    */
    LoadableUIElementIterator getFonts() const;
    
    /*!
    \brief
        Retrieves iterator for all references to LookNFeels files that are to be loaded with this Scheme
    */
    LoadableUIElementIterator getLookNFeels() const;

private:
	/*************************************************************************
		Structs used to hold scheme information
	*************************************************************************/

    struct UIModule
    {
        String name;
        DynamicModule* dynamicModule;
        FactoryModule* factoryModule;

        typedef std::vector<String
            CEGUI_VECTOR_ALLOC(String)> TypeList;

        TypeList types;
    };

	struct AliasMapping
	{
		String aliasName;
		String targetName;
	};

    struct FalagardMapping
    {
        String windowName;
        String targetName;
        String rendererName;
        String lookName;
        String effectName;
    };

	/*************************************************************************
		Implementation Data
	*************************************************************************/
	String	d_name;			//!< the name of this scheme.

	LoadableUIElementList                   		d_imagesets;
	LoadableUIElementList                   		d_imagesetsFromImages;
	LoadableUIElementList                   		d_fonts;
    
    typedef std::vector<UIModule
        CEGUI_VECTOR_ALLOC(UIModule)>               UIModuleList;
	UIModuleList                    				d_widgetModules;
    
    typedef std::vector<UIModule
        CEGUI_VECTOR_ALLOC(UIModule)>               WRModuleList;
    WRModuleList                                    d_windowRendererModules;

	typedef std::vector<AliasMapping
        CEGUI_VECTOR_ALLOC(AliasMapping)>			AliasMappingList;
    AliasMappingList                                d_aliasMappings;

    LoadableUIElementList                       	d_looknfeels;

    typedef std::vector<FalagardMapping
        CEGUI_VECTOR_ALLOC(FalagardMapping)>        FalagardMappingList;
    FalagardMappingList                             d_falagardMappings;

    static String d_defaultResourceGroup;   //!< holds default resource group
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIScheme_h_
