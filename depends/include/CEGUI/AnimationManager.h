/***********************************************************************
    created:    7/8/2010
    author:     Martin Preisler

    purpose:    Defines the interface for the AnimationManager object
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
#ifndef _CEGUIAnimationManager_h_
#define _CEGUIAnimationManager_h_

#include "CEGUI/Singleton.h"
#include "CEGUI/String.h"
#include <map>
#include <vector>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

class CEGUIEXPORT AnimationManager :
    public Singleton<AnimationManager>,
    public AllocatedObject<AnimationManager>
{
public:
    //! Name of the schema used for loading animation xml files.
    static const String XMLSchemaName;

    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    /*!
    \brief
        Constructs a new AnimationManager object.

        NB: Client code should not create AnimationManager objects - they are of
        limited use to you!  The intended pattern of access is to get a pointer
        to the GUI system's AnimationManager via the System object, and use
        that.
    */
    AnimationManager(void);


    /*!
    \brief
        Destructor for AnimationManager objects

        This will properly destroy all remaining AnimationInstance and Animation
        objects.
    */
    ~AnimationManager(void);

    /*!
    \brief
        Adds interpolator to be available for Affectors

    \par
        CEGUI ships with several basic interpolators that are always available,
        float, bool, colour, UDim, UVector2, ... but you can add your own
        custom interpolator if needed! just note that AnimationManager only
        deletes inbuilt interpolators. It will remove your interpolator if you
        don't do it yourself, but you definitely have to delete it yourself!
    */
    void addInterpolator(Interpolator* interpolator);

    /*!
    \brief
        Removes interpolator
    */
    void removeInterpolator(Interpolator* interpolator);

    /*!
    \brief
        Retrieves interpolator by type
    */
    Interpolator* getInterpolator(const String& type) const;

    /*!
    \brief
        Creates a new Animation definition

    \see
        Animation
    */
    Animation* createAnimation(const String& name = "");

    /*!
    \brief
        Destroys given animation definition
    */
    void destroyAnimation(Animation* animation);

    /*!
    \brief
        Destroys given animation definition by name
    */
    void destroyAnimation(const String& name);
    
    /*!
    \brief
        Destroys all animations in existence!
    */
    void destroyAllAnimations();

    /*!
    \brief
        Retrieves animation by name
    */
    Animation* getAnimation(const String& name) const;

    /*!
    \brief
        Examines the list of Animations to see if one exists with the given name

    \param name
        String holding the name of the Animation to look for.

    \return
        true if an Animation was found with a name matching \a name.  false if
        no matching Animation was found.
    */
    bool isAnimationPresent(const String& name) const;

    /*!
    \brief
        Retrieves animation by index
    */
    Animation* getAnimationAtIdx(size_t index) const;

    /*!
    \brief
        Retrieves number of defined animations
    */
    size_t getNumAnimations() const;

    /*!
    \brief
        Instantiates given animation

    \see
        AnimationInstance
    */
    AnimationInstance* instantiateAnimation(Animation* animation);

    /*!
    \brief
        Instantiates given animation by name

    \see
        AnimationInstance
    */
    AnimationInstance* instantiateAnimation(const String& name);

    /*!
    \brief
        Destroys given animation instance
    */
    void destroyAnimationInstance(AnimationInstance* instance);

    /*!
    \brief
        Destroys all instances of given animation
    */
    void destroyAllInstancesOfAnimation(Animation* animation);
    
    /*!
    \brief
        Destroys all instances of all animations
    */
    void destroyAllAnimationInstances();

    /*!
    \brief
        Retrieves animation instance at given index
    */
    AnimationInstance* getAnimationInstanceAtIdx(size_t index) const;

    /*!
    \brief
        Retrieves number of animation instances, number of times any animation
        was instantiated.
    */
    size_t getNumAnimationInstances() const;

    /*!
    \brief
        Internal method, gets called by CEGUI::System automatically.

        Only use if you know what you're doing!

    \par
    	Steps animation instances with auto stepping enabled forward
    	by given delta.
    */
    void autoStepInstances(float delta);

    /*!
    \brief
        Parses an XML file containing animation specifications to create
        and initialise Animation objects.

    \param filename
        String object holding the filename of the XML file to be processed.

    \param resourceGroup
        Resource group identifier to be passed to the resource provider when
        loading the XML file.
    */
    void loadAnimationsFromXML(const String& filename,
                               const String& resourceGroup = "");

    /*!
    \brief
        Parses XML source containing animation specifications to create
        and initialise Animation objects.

    \param source
        String object holding the XML source to be processed.
    */
    void loadAnimationsFromString(const String& source);

    /*!
    \brief
        Writes given animation definition to the given OutStream.

    \param animation
        Animation definition to write

    \param out_stream
        OutStream (std::ostream based) object where data is to be sent.
    */
    void writeAnimationDefinitionToStream(const Animation& animation, OutStream& out_stream) const;

    /*!
    \brief
        Writes given animation definition and returns the result as String

    \param animation
        Animation definition to write

    \warning
        This is a convenience function and isn't designed to be fast at all! Use the other alternatives
        if you want performance.

    \return
        String containing the resulting XML
    */
    String getAnimationDefinitionAsString(const Animation& animation) const;

    /*!
    \brief
        Sets the default resource group to be used when loading animation xml
        data

    \param resourceGroup
        String describing the default resource group identifier to be used.
    */
    static void setDefaultResourceGroup(const String& resourceGroup)
    {
        s_defaultResourceGroup = resourceGroup;
    }

    /*!
    \brief
        Returns the default resource group currently set for loading animation
        xml data.

    \return
        String describing the default resource group identifier that will be
        used when loading Animation xml data.
    */
    static const String& getDefaultResourceGroup()
    {
        return s_defaultResourceGroup;
    }

private:
    typedef std::map<String, Interpolator*, std::less<String>
        CEGUI_MAP_ALLOC(String, Interpolator*)> InterpolatorMap;
    String generateUniqueAnimationName();
    
    //! stores available interpolators
    InterpolatorMap d_interpolators;
    typedef std::vector<Interpolator*
        CEGUI_VECTOR_ALLOC(Interpolator*)> BasicInterpolatorList;
    //! stores interpolators that are inbuilt in CEGUI
    BasicInterpolatorList d_basicInterpolators;

    typedef std::map<String, Animation*> AnimationMap;
    //! all defined animations
    AnimationMap d_animations;

    typedef std::multimap<Animation*, AnimationInstance*, std::less<Animation*>
        CEGUI_MULTIMAP_ALLOC(Animation*, AnimationInstance*)> AnimationInstanceMap;
    //! all instances of animations
    AnimationInstanceMap d_animationInstances;
    //! Default resource group used when loading animation xml files.
    static String s_defaultResourceGroup;
    //! Base name to use for generated window names.
    static const String GeneratedAnimationNameBase;
    //! Counter used to generate unique animation names.
    unsigned long d_uid_counter;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIAnimationManager_h_

