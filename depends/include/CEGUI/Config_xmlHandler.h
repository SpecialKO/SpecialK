/***********************************************************************
    created:    Sat Jul 25 2009
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIConfig_xmlHandler_h_
#define _CEGUIConfig_xmlHandler_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/Logger.h"
#include "CEGUI/XMLHandler.h"
#include <vector>

#if defined (_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
//! Handler class used to parse the Configuration XML file.
class Config_xmlHandler : public XMLHandler
{
public:
    //! Name of xsd schema file used for validation.
    static const String CEGUIConfigSchemaName;
    // xml tag names
    static const String CEGUIConfigElement;
    static const String LoggingElement;
    static const String AutoLoadElement;
    static const String ResourceDirectoryElement;
    static const String DefaultResourceGroupElement;
    static const String ScriptingElement;
    static const String XMLParserElement;
    static const String ImageCodecElement;
    static const String DefaultFontElement;
    static const String DefaultMouseCursorElement;
    static const String DefaultTooltipElement;
    // xml attribute names
    static const String FilenameAttribute;
    static const String LevelAttribute;
    static const String TypeAttribute;
    static const String GroupAttribute;
    static const String PatternAttribute;
    static const String DirectoryAttribute;
    static const String InitScriptAttribute;
    static const String TerminateScriptAttribute;
    static const String ImageAttribute;
    static const String NameAttribute;

    //! Constructor.
    Config_xmlHandler();

    //! Destructor.
    ~Config_xmlHandler();

    //! Initialise the CEGUI XMLParser according to info parsed from config.
    void initialiseXMLParser() const;
    //! Initialise the CEGUI ImageCodec according to info parsed from config.
    void initialiseImageCodec() const;
    //! Initialise the CEGUI Logger according to info parsed from config.
    void initialiseLogger(const String& default_filename) const;
    //! Set resource group dirs read from the config. (DefaultResourceProvider only).
    void initialiseResourceGroupDirectories() const;
    //! Set default resource groups according to those in the config.
    void initialiseDefaultResourceGroups() const;
    //! Auto-load all resources specified in the config.
    void loadAutoResources() const;
    //! initialise the system default font according to the config.
    void initialiseDefaultFont() const;
    //! initialise the system default mouse cursor image according to the config.
    void initialiseDefaultMouseCursor() const;
    //! initialise the system default tooltip according to the config.
    void initialiseDefaulTooltip() const;
    //! execute the init script as specified in the config.
    void executeInitScript() const;
    //! return the name of the terminate script from the config (hacky!)
    const String& getTerminateScriptName() const;

    // XMLHandler overrides
    const String& getSchemaName() const;
    const String& getDefaultResourceGroup() const;

    void elementStart(const String& element, const XMLAttributes& attributes);
    void elementEnd(const String& element);

private:
    //! enumeration of resource types.
    enum ResourceType
    {
        RT_IMAGESET,
        RT_FONT,
        RT_SCHEME,
        RT_LOOKNFEEL,
        RT_LAYOUT,
        RT_SCRIPT,
        RT_XMLSCHEMA,
        RT_DEFAULT
    };

    //! struct to track a resource group directory specification.
    struct ResourceDirectory
    {
        String group;
        String directory;
    };

    //! struct to track a default resource group specification.
    struct DefaultResourceGroup
    {
        ResourceType type;
        String group;
    };

    //! struct to track a set of resources to be auto-loaded.
    struct AutoLoadResource
    {
        String type_string;
        ResourceType type;
        String group;
        String pattern;
    };

    // functions to handle the various elements
    void handleCEGUIConfigElement(const XMLAttributes& attr);
    void handleLoggingElement(const XMLAttributes& attr);
    void handleAutoLoadElement(const XMLAttributes& attr);
    void handleResourceDirectoryElement(const XMLAttributes& attr);
    void handleDefaultResourceGroupElement(const XMLAttributes& attr);
    void handleScriptingElement(const XMLAttributes& attr);
    void handleXMLParserElement(const XMLAttributes& attr);
    void handleImageCodecElement(const XMLAttributes& attr);
    void handleDefaultTooltipElement(const XMLAttributes& attr);
    void handleDefaultFontElement(const XMLAttributes& attr);
    void handleDefaultMouseCursorElement(const XMLAttributes& attr);

    //! helper to convert resource type string to something more useful.
    ResourceType stringToResourceType(const String& type) const;
    //! helper to auto-load looknfeels
    void autoLoadLookNFeels(const String& pattern, const String& group) const;
    //! helper to auto-load imagesets
    void autoLoadImagesets(const String& pattern, const String& group) const;

    //! type of collection holding resource group directory specifications.
    typedef std::vector<ResourceDirectory
        CEGUI_VECTOR_ALLOC(ResourceDirectory)> ResourceDirVector;
    //! type of collection holding default resource group specifications.
    typedef std::vector<DefaultResourceGroup
        CEGUI_VECTOR_ALLOC(DefaultResourceGroup)> DefaultGroupVector;
    //! type of collection holding specifications of resources to auto-load.
    typedef std::vector<AutoLoadResource
        CEGUI_VECTOR_ALLOC(AutoLoadResource)> AutoResourceVector;
    //! The name to use for the CEGUI log file.
    String d_logFileName;
    //! The logging level to be set.
    LoggingLevel d_logLevel;
    //! The name of the XML parser module to initialise.
    String d_xmlParserName;
    //! The name of the image codec module to initialise.
    String d_imageCodecName;
    //! The name of the default font to be set.
    String d_defaultFont;
    //! The name of the default mouse cursor image to use.
    String d_defaultMouseImage;
    //! The name of the default tooltip window type.
    String d_defaultTooltipType;
    //! name of the initialisation script.
    String d_scriptingInitScript;
    //! name of the termination script.
    String d_scriptingTerminateScript;
    //! Collection of resouce group directories to be set.
    ResourceDirVector d_resourceDirectories;
    //! Collection of default resource groups to be set.
    DefaultGroupVector d_defaultResourceGroups;
    //! Collection of resource specifications to be auto-loaded.
    AutoResourceVector d_autoLoadResources;
};

} // End of  CEGUI namespace section

#if defined (_MSC_VER)
#   pragma warning(pop)
#endif

#endif // end of guard _CEGUIConfig_xmlHandler_h_
