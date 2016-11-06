/***********************************************************************
	created:	22/2/2004
	author:		Paul D Turner
	
	purpose:	Defines interface for WindowFactoryManager class
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
#ifndef _CEGUIWindowFactoryManager_h_
#define _CEGUIWindowFactoryManager_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/Singleton.h"
#include "CEGUI/Logger.h"
#include "CEGUI/IteratorBase.h"
#include "CEGUI/WindowFactory.h"
#include "CEGUI/TplWindowFactory.h"
#include "CEGUI/Exceptions.h"
#include <map>
#include <vector>

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
	Class that manages WindowFactory objects

\todo
    I think we could clean up the mapping stuff a bit. Possibly make it more generic now
    with the window renderers and all.
*/
class CEGUIEXPORT WindowFactoryManager :
    public Singleton<WindowFactoryManager>,
    public AllocatedObject<WindowFactoryManager>
{
public:
    /*!
    \brief
        struct used to hold mapping information required to create a falagard based window.
    */
    struct CEGUIEXPORT FalagardWindowMapping
    {
        String  d_windowType;
        String  d_lookName;
        String  d_baseType;
        String  d_rendererType;
        String  d_effectName;
    };

    /*!
    \brief
        Class used to track active alias targets for Window factory types.
    */
	class CEGUIEXPORT AliasTargetStack
	{
	public:
		/*!
		\brief
			Constructor for WindowAliasTargetStack objects
		*/
		AliasTargetStack(void) {}


		/*!
		\brief
			Destructor for WindowAliasTargetStack objects
		*/
		~AliasTargetStack(void) {}


		/*!
		\brief
			Return a String holding the current target type for this stack

		\return
			reference to a String object holding the currently active target type name for this stack.
		*/
		const String&	getActiveTarget(void) const;

		/*!
		\brief
			Return the number of stacked target types in the stack

		\return
			number of target types stacked for this alias.
		*/
		uint	getStackedTargetCount(void) const;


	private:
		friend class WindowFactoryManager;

		typedef std::vector<String
            CEGUI_VECTOR_ALLOC(String)> TargetTypeStack; //!< Type used to implement stack of target type names.

		TargetTypeStack	d_targetStack; //!< Container holding the target types.
	};


	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructs a new WindowFactoryManager object.
	*/
	WindowFactoryManager(void);


	/*!
	\brief
		Destructor for WindowFactoryManager objects
	*/
	~WindowFactoryManager(void)
	{
		Logger::getSingleton().logEvent("CEGUI::WindowFactoryManager singleton destroyed");
	}


	/*************************************************************************
		Public Interface
	*************************************************************************/
	/*!
	\brief
		Adds a new WindowFactory to the list of registered factories.

	\param factory
		Pointer to the WindowFactory to be added to the WindowManager.

	\return
		Nothing
	
	\exception NullObjectException		\a factory was null.
	\exception AlreadyExistsException	\a factory provided a Window type name which is in use by another registered WindowFactory.
	*/
	void	addFactory(WindowFactory* factory);

    /*!
    \brief
        Creates a WindowFactory of the type \a T and adds it to the system for
        use.  The created WindowFactory will automatically be deleted when the
        factory is removed from the system (either directly or at system 
        deletion time).

    \tparam T
        Specifies the type of WindowFactory subclass to add a factory for.

    \return
        Nothing
    */
    template <typename T>
    static void addFactory();

    /*!
    \brief
        Internally creates a factory suitable for creating Window objects
        of the given type and adds it to the system.

    \note
        The internally created factory is owned and managed by CEGUI,
        and will be automatically deleted when the window type is removed from
        the system - either directly by calling
        WindowFactoryManager::removeFactory or at system shut down.

    \tparam T
        Specifies the type of Window to add a factory for.
    */
    template <typename T>
    static void addWindowType();

	/*!
	\brief
		Removes a WindowFactory from the list of registered factories.
		
	\note
		The WindowFactory object is not destroyed (since it was created externally), instead it is just removed from the list.

	\param name
		String which holds the name (technically, Window type name) of the WindowFactory to be removed.  If \a name is not
		in the list, no error occurs (nothing happens).

	\return
		Nothing
	*/
	void	removeFactory(const String& name);


	/*!
	\brief
		Removes a WindowFactory from the list of registered factories.

	\note
		The WindowFactory object is not destroyed (since it was created externally), instead it is just removed from the list.

	\param factory
		Pointer to the factory object to be removed.  If \a factory is null, or if no such WindowFactory is in the list, no
		error occurs (nothing happens).

	\return
		Nothing
	*/
	void	removeFactory(WindowFactory* factory);


	/*!
	\brief
		Remove all WindowFactory objects from the list.

	\return
		Nothing
	*/
    void removeAllFactories(void);


	/*!
	\brief
		Return a pointer to the specified WindowFactory object.

	\param type
		String holding the Window object type to return the WindowFactory for.

	\return
		Pointer to the WindowFactory object that creates Windows of the type \a type.

	\exception UnknownObjectException	No WindowFactory object for Window objects of type \a type was found.
	*/
	WindowFactory*	getFactory(const String& type) const;


    /*!
    \brief
        Checks the list of registered WindowFactory objects, aliases, and
        falagard mapped types for one which can create Window objects of the
        specified type.

    \param name
        String containing the Window type name to check for.

    \return
        - true if a WindowFactory, alias, or falagard mapping for Window objects
          of type \a name is registered.
        - false if the system knows nothing about windows of type \a name.
    */
    bool	isFactoryPresent(const String& name) const;


	/*!
	\brief
		Adds an alias for a current window type.

		This method allows you to create an alias for a specified window type.  This means that you can then use
		either name as the type parameter when creating a window.

	\note
		You need to be careful using this system.  Creating an alias using a name that already exists will replace the previous
		mapping for that alias.  Each alias name maintains a stack, which means that it is possible to remove an alias and have the
		previous alias restored.  The windows created via an alias use the real type, so removing an alias after window creation is always
		safe (i.e. it is not the same as removing a real factory, which would cause an exception when trying to destroy a window with a missing
		factory).

	\param aliasName
		String object holding the alias name.  That is the name that \a targetType will also be known as from no on.

	\param targetType
		String object holding the type window type name that is to be aliased.  This type must already exist.

	\return
		Nothing.

	\exception UnknownObjectException	thrown if \a targetType is not known within the system.
	*/
	void	addWindowTypeAlias(const String& aliasName, const String& targetType);


	/*!
	\brief
		Remove the specified alias mapping.  If the alias mapping does not exist, nothing happens.

	\note
		You are required to supply both the alias and target names because there may exist more than one entry for a given
		alias - therefore you are required to be explicit about which alias is to be removed.

	\param aliasName
		String object holding the alias name.

	\param targetType
		String object holding the type window type name that was aliased.

	\return
		Nothing.
	*/
	void	removeWindowTypeAlias(const String& aliasName, const String& targetType);

	/*!
	\brief
		Remove all registered window type alias mappings.
	*/
	void removeAllWindowTypeAliases();

    /*!
    \brief
        Add a mapping for a falagard based window.

        This function creates maps a target window type and target 'look' name onto a registered window type, thus allowing
        the ususal window creation interface to be used to create windows that require extra information to full initialise
        themselves.
    \note
        These mappings support 'late binding' to the target window type, as such the type indicated by \a targetType need not
        exist in the system until attempting to create a Window using the type.
    \par
        Also note that creating a mapping for an existing type will replace any previous mapping for that same type.

    \param newType
        The type name that will be used to create windows using the target type and look.

    \param targetType
        The base window type.

    \param lookName
        The name of the 'look' that will be used by windows of this type.

    \param renderer
        The type of window renderer to assign for windows of this type.

    \param effectName
        The identifier of the RenderEffect to attempt to set up for windows of this type.

    \return
        Nothing.
    */
    void addFalagardWindowMapping(const String& newType,
                                  const String& targetType,
                                  const String& lookName,
                                  const String& renderer,
                                  const String& effectName = String(""));

    /*!
    \brief
        Remove the specified falagard type mapping if it exists.

    \return
        Nothing.
    */
    void removeFalagardWindowMapping(const String& type);

    /*!
	\brief
		Remove all registered falagard type mappings
	*/
    void removeAllFalagardWindowMappings();

    /*!
    \brief
        Return whether the given type is a falagard mapped type.

    \param type
        Name of a window type.

    \return
        - true if the requested type is a Falagard mapped window type.
        - false if the requested type is a normal WindowFactory (or alias), or if the type does not exist.
    */
    bool isFalagardMappedType(const String& type) const;

    /*!
    \brief
        Return the name of the LookN'Feel assigned to the specified window mapping.

    \param type
        Name of a window type.  The window type referenced should be a falagard mapped type.

    \return
        String object holding the name of the look mapped for the requested type.

    \exception InvalidRequestException thrown if \a type is not a falagard mapping type (or maybe the type didn't exist).
    */
    const String& getMappedLookForType(const String& type) const;

    /*!
    \brief
        Return the name of the WindowRenderer assigned to the specified window mapping.

    \param type
        Name of a window type.  The window type referenced should be a falagard mapped type.

    \return
        String object holding the name of the window renderer mapped for the requested type.

    \exception InvalidRequestException thrown if \a type is not a falagard mapping type (or maybe the type didn't exist).
    */
    const String& getMappedRendererForType(const String& type) const;

    /*!
    \brief
        Use the alias system, where required, to 'de-reference' the specified
        type to an actual window type that can be created directly (that being
        either a concrete window type, or a falagard mapped type).

    \note
        Even though implied by the above description, this method does not
        check that a factory for the final type exists; we simply say that the
        returned type is not an alias for some other type.

    \param type
        String describing the type to be de-referenced.

    \return
        String object holding a type for a window that can be created directly;
        that is, a type that does not describe an alias to some other type.
    */
    String getDereferencedAliasType(const String& type) const;

    /*!
    \brief
        Return the FalagardWindowMapping for the specified window mapping \a type.

    \param type
        Name of a window type.  The window type referenced should be a falagard mapped type.

    \return
        FalagardWindowMapping object describing the falagard mapping.

    \exception InvalidRequestException thrown if \a type is not a falagard mapping type (or maybe the type didn't exist).
    */
    const FalagardWindowMapping& getFalagardMappingForType(const String& type) const;

private:
	/*************************************************************************
		Implementation Data
	*************************************************************************/
	typedef	std::map<String, WindowFactory*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, WindowFactory*)> WindowFactoryRegistry; //!< Type used to implement registry of WindowFactory objects
	typedef std::map<String, AliasTargetStack, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, AliasTargetStack)> TypeAliasRegistry; //!< Type used to implement registry of window type aliases.
    typedef std::map<String, FalagardWindowMapping, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, FalagardWindowMapping)> FalagardMapRegistry; //!< Type used to implement registry of falagard window mappings.
    //! Type used for list of WindowFacory objects that we created ourselves
    typedef std::vector<WindowFactory*
        CEGUI_VECTOR_ALLOC(WindowFactory*)> OwnedWindowFactoryList;

	WindowFactoryRegistry	d_factoryRegistry;			//!< The container that forms the WindowFactory registry
	TypeAliasRegistry		d_aliasRegistry;			//!< The container that forms the window type alias registry.
    FalagardMapRegistry     d_falagardRegistry;         //!< Container that hold all the falagard window mappings.
    //! Container that tracks WindowFactory objects we created ourselves.
    static OwnedWindowFactoryList  d_ownedFactories;

public:
	/*************************************************************************
		Iterator stuff
	*************************************************************************/
	typedef	ConstMapIterator<WindowFactoryRegistry>	WindowFactoryIterator;
	typedef ConstMapIterator<TypeAliasRegistry>		TypeAliasIterator;
    typedef ConstMapIterator<FalagardMapRegistry>   FalagardMappingIterator;

	/*!
	\brief
		Return a WindowFactoryManager::WindowFactoryIterator object to iterate over the available WindowFactory types.
	*/
	WindowFactoryIterator	getIterator(void) const;


	/*!
	\brief
		Return a WindowFactoryManager::TypeAliasIterator object to iterate over the defined aliases for window types.
	*/
	TypeAliasIterator	getAliasIterator(void) const;


    /*!
    \brief
        Return a WindowFactoryManager::FalagardMappingIterator object to iterate over the defined falagard window mappings.
    */
    FalagardMappingIterator getFalagardMappingIterator() const;
};

//----------------------------------------------------------------------------//
template <typename T>
void WindowFactoryManager::addFactory()
{
    // create the factory object
    WindowFactory* factory = CEGUI_NEW_AO T;

    // only do the actual add now if our singleton has already been created
    if (WindowFactoryManager::getSingletonPtr())
    {
        Logger::getSingleton().logEvent("Created WindowFactory for '" +
                                        factory->getTypeName() +
                                        "' windows.");
        // add the factory we just created
        CEGUI_TRY
        {
            WindowFactoryManager::getSingleton().addFactory(factory);
        }
        CEGUI_CATCH (Exception&)
        {
            Logger::getSingleton().logEvent("Deleted WindowFactory for '" +
                                            factory->getTypeName() +
                                            "' windows.");
            // delete the factory object
            CEGUI_DELETE_AO factory;
            CEGUI_RETHROW;
        }
    }

    d_ownedFactories.push_back(factory);
}

//----------------------------------------------------------------------------//
template <typename T>
void WindowFactoryManager::addWindowType()
{
    WindowFactoryManager::addFactory<TplWindowFactory<T> >();
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIWindowFactoryManager_h_
