/***********************************************************************
	created:	20/2/2004
	author:		Paul D Turner

	purpose:	Defines interface for main GUI system class
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
#ifndef _CEGUISystem_h_
#define _CEGUISystem_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/Singleton.h"
#include "CEGUI/EventSet.h"
#include "CEGUI/Renderer.h"
#include "CEGUI/InputEvent.h"
#include "CEGUI/ResourceProvider.h"
#include <vector>

#if defined(__WIN32__) || defined(_WIN32)
#   include "CEGUI/Win32StringTranscoder.h"
#else
#   include "CEGUI/IconvStringTranscoder.h"
#endif

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4275)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	The System class is the CEGUI class that provides access to all other elements in this system.

	This object must be created by the client application.  The System object requires that you pass it
	an initialised Renderer object which it can use to interface to whatever rendering system will be
	used to display the GUI imagery.
*/
class CEGUIEXPORT System :
    public Singleton<System>,
    public EventSet,
    public AllocatedObject<System>
{
public:
	static const String EventNamespace;				//!< Namespace for global events

	/*************************************************************************
		Constants
	*************************************************************************/
	// event names
    /** Event fired for display size changes (as notified by client code).
     * Handlers are passed a const DisplayEventArgs reference with
     * DisplayEventArgs::size set to the pixel size that was notifiied to the
     * system.
     */
    static const String EventDisplaySizeChanged;
    /** Event fired when global custom RenderedStringParser is set.
     * Handlers are passed a const reference to a generic EventArgs struct.
     */
    static const String EventRenderedStringParserChanged;

	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
    /*!
    \brief
        Create the System object and return a reference to it.

    \param renderer
        Reference to a valid Renderer object that will be used to render GUI
        imagery.

    \param resourceProvider
        Pointer to a ResourceProvider object, or NULL to use whichever default
        the Renderer provides.

    \param xmlParser
        Pointer to a valid XMLParser object to be used when parsing XML files,
        or NULL to use a default parser.

    \param imageCodec
        Pointer to a valid ImageCodec object to be used when loading image
        files, or NULL to use a default image codec.

    \param scriptModule
        Pointer to a ScriptModule object.  may be NULL for none.

    \param configFile
        String object containing the name of a configuration file to use.

    \param logFile
        String object containing the name to use for the log file.

    \param abi
        This must be set to CEGUI_VERSION_ABI
    */
    static System& create(Renderer& renderer,
                          ResourceProvider* resourceProvider = 0,
                          XMLParser* xmlParser = 0,
                          ImageCodec* imageCodec = 0,
                          ScriptModule* scriptModule = 0,
                          const String& configFile = "",
                          const String& logFile = "CEGUI.log",
                          const int abi = CEGUI_VERSION_ABI);

    //! Destroy the System object.
    static void destroy();

    /*!
    \brief
        Retrieves CEGUI's major version as an integer

    \note
        API/headers major version is a macro called CEGUI_MAJOR_VERSION,
        this returns the version your application is linking to
    */
    static unsigned int getMajorVersion();

    /*!
    \brief
        Retrieves CEGUI's minor version as an integer

    \note
        API/headers minor version is a macro called CEGUI_MINOR_VERSION,
        this returns the version your application is linking to
    */
    static unsigned int getMinorVersion();

    /*!
    \brief
        Retrieves CEGUI's patch version as an integer

    \note
        API/headers patch version is a macro called CEGUI_PATCH_VERSION,
        this returns the version your application is linking to
    */
    static unsigned int getPatchVersion();

    /*!
    \brief
        Retrieves CEGUI's "short" version ("1.2.3" for example)

    \note
        API/headers version can be constructed using CEGUI_*_VERSION macros,
        this returns the version your application is linking to
    */
    static const String& getVersion();

    /*!
    \brief
        Retrieves CEGUI's "verbose" version, includes info about compiler, platform, etc...

    \note
        API/headers verbose version can be constructed using various compiler specific macros,
        this returns the version your application is linking to
    */
    static const String& getVerboseVersion();

	/*!
	\brief
		Return a pointer to the Renderer object being used by the system

	\return
		Pointer to the Renderer object used by the system.
	*/
	Renderer*	getRenderer(void) const			{return d_renderer;}


	/*!
	\brief
		Return singleton System object

	\return
		Singleton System object
	*/
	static	System&	getSingleton(void);


	/*!
	\brief
		Return pointer to singleton System object

	\return
		Pointer to singleton System object
	*/
	static	System*	getSingletonPtr(void);

    /*!
    \brief
        Retrieves internal CEGUI clipboard, optionally synced with system wide clipboard
    */
    Clipboard* getClipboard() const         {return d_clipboard;}

    GUIContext& getDefaultGUIContext() const;

    /*!
    \brief
        Depending upon the internal state, for each GUIContext this may either
        re-use cached rendering from last time or trigger a full re-draw of all
        elements.
    */
    void renderAllGUIContexts();


	/*!
	\brief
		Return a pointer to the ScriptModule being used for scripting within the GUI system.

	\return
		Pointer to a ScriptModule based object.
	*/
	ScriptModule*	getScriptingModule(void) const;

    /*!
    \brief
        Set the ScriptModule to be used for scripting within the GUI system.

    \param scriptModule
        Pointer to a ScriptModule based object, or 0 for none (be careful!)

    \return
        Nothing
    */
    void setScriptingModule(ScriptModule* scriptModule);

	/*!
	\brief
		Return a pointer to the ResourceProvider being used within the GUI system.

	\return
		Pointer to a ResourceProvider based object.
	*/
	ResourceProvider* getResourceProvider(void) const;

	/*!
	\brief
		Execute a script file if possible.

	\param filename
		String object holding the filename of the script file that is to be executed

	\param resourceGroup
		Resource group identifier to be passed to the ResourceProvider when loading the script file.
	*/
	void	executeScriptFile(const String& filename, const String& resourceGroup = "") const;


	/*!
	\brief
		Execute a scripted global function if possible.  The function should not take any parameters and should return an integer.

	\param function_name
		String object holding the name of the function, in the global script environment, that
		is to be executed.

	\return
		The integer value returned from the script function.
	*/
	int		executeScriptGlobal(const String& function_name) const;


    /*!
    \brief
        If possible, execute script code contained in the given CEGUI::String object.

    \param str
        String object holding the valid script code that should be executed.

    \return
        Nothing.
    */
    void executeScriptString(const String& str) const;


    /*!
    \brief
        Set a new XML parser module to be used.

        The current XMLParser will be cleaned up and, if owned by the system,
        also deleted, as will any dynamically loaded module associated with the
        XMLParser object.  The newly created XMLParser object, and the
        associated module will be owned by the system.

    \param parserName
        String object describing the name of the XML parser module to be used.
    */
    void setXMLParser(const String& parserName);

    /*!
    \brief
        Sets the XMLParser object to be used by the system.

        The current XMLParser will be cleaned up and, if owned by the system,
        also deleted, as will any dynamically loaded module associated with the
        XMLParser object.

        If the argument passed in the \a parser parameter is 0, the system will
        cleanup any existing parser as described above, and revert to using
        the parser provided by the default module (see getDefaultXMLParserName
        and setDefaultXMLParserName).

    \param parser
        Pointer to the XMLParser object to be used by the system, or 0 to cause
        the system to initialise a default parser.
    */
    void setXMLParser(XMLParser* parser);

    /*!
    \brief
        Return the XMLParser object.
     */
    XMLParser* getXMLParser(void) const     { return d_xmlParser; }


    /*!
    \brief
        Static member to set the name of the default XML parser module that
        should be used.

        If you want to modify the default parser from the one compiled in, you
        need to call this static member prior to instantiating the main
        CEGUI::System object.

        Note that calling this member to change the name of the default module
        after CEGUI::System, and therefore the default xml parser, has been
        created will have no real effect - the default parser name will be
        updated, though no actual changes to the xml parser module will occur.

        The built-in options for this are:
         - XercesParser
         - ExpatParser
         - LibxmlParser
         - TinyXMLParser

        Whether these are actually available, depends upon how you built the
        system.  If you have some custom parser, you can provide the name of
        that here to have it used as the default, though note that the
        final filename of the parser module should be of the form:

        [prefix]CEGUI[parserName][suffix]

        where:
        - [prefix] is some optional prefix; like 'lib' on linux.
        - CEGUI is a required prefix.
        - [parserName] is the name of the parser, as supplied to this function.
        - [suffix] is the filename suffix, like .dll or .so

        Final module filenames are, thus, of the form:
        - CEGUIXercesParser.dll
        - libCEGUIXercesParser.so

    \param parserName
        String describing the name of the xml parser module to be used as the
        default.

    \return
        Nothing.
    */
    static void setDefaultXMLParserName(const String& parserName);

    /*!
    \brief
        Return the name of the currently set default xml parser module.

    \return
        String holding the currently set default xml parser name.  Note that if
        this name has been changed after instantiating the system, the name
        returned may not actually correspond to the module in use.
    */
    static const String getDefaultXMLParserName();
    
    /*!
    \brief
        Retrieve the image codec to be used by the system.
    */
    ImageCodec& getImageCodec() const;

    /*!
    \brief
        Set the image codec to be used by the system.
    */
    void setImageCodec(const String& codecName);

    /*!
    \brief
        Set the image codec to use from an existing image codec.

        In this case the renderer does not take the ownership of the image codec
        object.

    \param codec
        The ImageCodec object to be used.
    */
    void setImageCodec(ImageCodec& codec);

    /*!
    \brief
        Set the name of the default image codec to be used.
    */
    static void setDefaultImageCodecName(const String& codecName);

    /*!
    \brief
        Get the name of the default image codec.
    */
    static const String& getDefaultImageCodecName();

    /*!
    \brief
        Notification function to be called when the main display changes size.
        Client code should call this function when the host window changes size,
        or if the display resolution is changed in full-screen mode.

        Calling this function ensures that any other parts of the system that
        need to know about display size changes are notified.  This affects
        things such as the MouseCursor default constraint area, and also the
        auto-scale functioning of Imagesets and Fonts.

    \note
        This function will also fire the System::EventDisplaySizeChanged event.

    \param new_size
        Size object describing the new display size in pixels.
    */
    void notifyDisplaySizeChanged(const Sizef& new_size);

    /*!
    \brief
        Return pointer to the currently set global default custom
        RenderedStringParser object.

        The returned RenderedStringParser is used for all windows that have
        parsing enabled and no custom RenderedStringParser set on the window
        itself.

        If this global custom RenderedStringParser is set to 0, then all windows
        with parsing enabled and no custom RenderedStringParser set on the
        window itself will use the systems BasicRenderedStringParser. 
    */
    RenderedStringParser* getDefaultCustomRenderedStringParser() const;

    /*!
    \brief
        Set the global default custom RenderedStringParser object.  This change
        is reflected the next time an affected window reparses it's text.  This
        may be set to 0 for no system wide custom parser (which is the default).

        The set RenderedStringParser is used for all windows that have
        parsing enabled and no custom RenderedStringParser set on the window
        itself.

        If this global custom RenderedStringParser is set to 0, then all windows
        with parsing enabled and no custom RenderedStringParser set on the
        window itself will use the systems BasicRenderedStringParser. 
    */
    void setDefaultCustomRenderedStringParser(RenderedStringParser* parser);

    /*!
    \brief
        Invalidate all imagery and geometry caches for CEGUI managed elements.

        This function will invalidate the caches used for both imagery and
        geometry for all content that is managed by the core CEGUI manager
        objects, causing a full and total redraw of that content.  This
        includes Window object's cached geometry, rendering surfaces and
        rendering windows and the mouse pointer geometry.
    */
    void invalidateAllCachedRendering();

    /*!
    \brief
        Create a RegexMatcher instance if support is available.

    \return
        Pointer to an object that implements the RegexMatcher interface, or 0
        if the system has no built in support for RegexMatcher creation.

    \note
        The created RegexMatcher is not tracked in any way, and it is the
        resposibility of the caller to destroy the RegexMatcher when it is no
        longer needed by calling System::destroyRegexMatcher.
    */
    RegexMatcher* createRegexMatcher() const;

    //! destroy a RegexMatcher instance returned by System::createRegexMatcher.
    void destroyRegexMatcher(RegexMatcher* rm) const;

    //! call this to ensure system-level time based updates occur.
    bool injectTimePulse(float timeElapsed);

    GUIContext& createGUIContext(RenderTarget& rt);
    void destroyGUIContext(GUIContext& context);

    /*!
    \brief adds factories for all the basic window types

    You do not need to call this manually! Standard Window factories will be
    added automatically. One occasion when you will need this is if you
    remove all window factories from WindowFactoryManager and want to add the
    standard ones back
    */
    void addStandardWindowFactories();

    //! Return the system StringTranscoder object
    static const StringTranscoder& getStringTranscoder();

    //! Internal CEGUI version validation function.
    static void performVersionTest(const int expected, const int received,
                                   const String& func);

private:
    // unimplemented constructors / assignment
    System(const System& obj);
    System& operator=(const System& obj);

protected:
	/*************************************************************************
		Implementation Functions
	*************************************************************************/
    /*!
    \brief
        Construct a new System object

    \param renderer
        Reference to a valid Renderer object that will be used to render GUI
        imagery.

    \param resourceProvider
        Pointer to a ResourceProvider object, or NULL to use whichever default
        the Renderer provides.

    \param xmlParser
        Pointer to a valid XMLParser object to be used when parsing XML files,
        or NULL to use a default parser.

    \param imageCodec
        Pointer to a valid ImageCodec object to be used when loading image
        files, or NULL to use a default image codec.

    \param scriptModule
        Pointer to a ScriptModule object.  may be NULL for none.

    \param configFile
        String object containing the name of a configuration file to use.

    \param logFile
        String object containing the name to use for the log file.
    */
    System(Renderer& renderer, ResourceProvider* resourceProvider,
           XMLParser* xmlParser, ImageCodec* imageCodec,
           ScriptModule* scriptModule, const String& configFile,
           const String& logFile);

    /*!
    \brief
        Destructor for System objects.
    */
    ~System(void);

    //! output the standard log header
    void outputLogHeader();

    //! create the other core system singleton objects (except the logger)
    void createSingletons();

    //! cleanup the core system singleton objects
    void destroySingletons();

    //! handle creation and initialisation of the XML parser.
    void setupXMLParser();

    //! handle cleanup of the XML parser
    void cleanupXMLParser();

    //! setup image codec 
    void setupImageCodec(const String& codecName);

    //! cleanup image codec 
    void cleanupImageCodec();

    //! invalidate all windows and any rendering surfaces they may be using.
    void invalidateAllWindows();

	/*************************************************************************
		Implementation Data
	*************************************************************************/
	Renderer*	d_renderer;			//!< Holds the pointer to the Renderer object given to us in the constructor
    ResourceProvider* d_resourceProvider;      //!< Holds the pointer to the ResourceProvider object given to us by the renderer or the System constructor.
	bool d_ourResourceProvider;

    Clipboard* d_clipboard;         //!< Internal clipboard with optional sync with native clipboard

	// scripting
	ScriptModule*	d_scriptModule;			//!< Points to the scripting support module.
	String			d_termScriptName;		//!< Name of the script to run upon system shutdown.

    XMLParser*  d_xmlParser;        //!< XMLParser object we use to process xml files.
    bool        d_ourXmlParser;     //!< true when we created the xml parser.
    DynamicModule* d_parserModule;  //! pointer to parser module.

    static String   d_defaultXMLParserName; //!< Holds name of default XMLParser

    //! Holds a pointer to the image codec to use.
    ImageCodec* d_imageCodec;
    //! true when we created the image codec.
    bool d_ourImageCodec;
    /** Holds a pointer to the image codec module. If d_imageCodecModule is 0 we
     *  are not owner of the image codec object
     */
    DynamicModule* d_imageCodecModule;
    //! Holds the name of the default codec to use.
    static String d_defaultImageCodecName;
    //! true when we created the CEGUI::Logger based object.
    bool d_ourLogger;
    //! currently set global RenderedStringParser.
    RenderedStringParser* d_customRenderedStringParser;

    typedef std::vector<GUIContext* CEGUI_VECTOR_ALLOC(GUIContext*)> GUIContextCollection;
    GUIContextCollection d_guiContexts;
    //! instance of class that can convert string encodings
#if defined(__WIN32__) || defined(_WIN32)
    static const Win32StringTranscoder d_stringTranscoder;
#else
    static const IconvStringTranscoder d_stringTranscoder;
#endif
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUISystem_h_
