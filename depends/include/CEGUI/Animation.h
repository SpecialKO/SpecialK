/***********************************************************************
    created:    7/8/2010
    author:     Martin Preisler

    purpose:    Defines the interface for the Animation class
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
#ifndef _CEGUIAnimation_h_
#define _CEGUIAnimation_h_

#include "CEGUI/String.h"
#include <vector>
#include <map>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Defines an 'animation' class

    This is definition of Animation. Can be reused multiple times via
    AnimationInstance class. You can't step this class directly, you have to
    instantiate it via AnimationManager::instantiateAnimation.

    AnimationInstance provides means for stepping the animation and applying
    it to PropertySets.

\par
    Animation itself doesn't contain key frames. It is composed of Affector(s).
    Each Affector affects one Property. So one Animation can affect multiple
    properties.

\see
    AnimationInstance, Affector
*/
class CEGUIEXPORT Animation : public AllocatedObject<Animation>
{
public:
    //! enumerates possible replay modes
    enum ReplayMode
    {
        //! plays the animation just once, then stops
        RM_Once,
        //! loops the animation infinitely
        RM_Loop,
        /** infinitely plays the animation forward, when it reaches the end, it
         * plays it backwards, etc...
         */
        RM_Bounce
    };

    /** internal constructor, please only construct animations via
     * AnimationManager::createAnimation method
     */
    Animation(const String& name);

    //! destructor, this destroys all affectors defined inside this animation
    ~Animation(void);

    /*!
    \brief
        Retrieves name of this Animation definition
    */
    const String& getName() const;

    /*!
    \brief
        Sets the replay mode of this animation
    */
    void setReplayMode(ReplayMode mode);

    /*!
    \brief
        Retrieves the replay mode of this animation
    */
    ReplayMode getReplayMode() const;

    /*!
    \brief
        Sets the duration of this animation
    */
    void setDuration(float duration);

    /*!
    \brief
        Retrieves the duration of this animation
    */
    float getDuration() const;

    /*!
    \brief
        Sets whether this animation auto starts or not

    \par
        Auto start means that the animation instances of this definition call
        Start on themselves once their target is set.
    */
    void setAutoStart(bool autoStart);

    /*!
    \brief
        Retrieves auto start.

    \see
        Animation::setAutoStart
    */
    bool getAutoStart() const;

    /*!
    \brief
        Creates a new Affector

    \see
        Affector
    */
    Affector* createAffector(void);

    /*!
    \brief
        Creates a new Affector

    \par
        This is just a helper, finger saving method.
    */
    Affector* createAffector(const String& targetProperty,
                             const String& interpolator);

    /*!
    \brief
        Destroys given Affector
    */
    void destroyAffector(Affector* affector);

    /*!
    \brief
        Retrieves the Affector at given index
    */
    Affector* getAffectorAtIdx(size_t index) const;

    /*!
    \brief
        Retrieves number of Affectors defined in this Animation
    */
    size_t getNumAffectors(void) const;

    /*!
    \brief
        This defined a new auto subscription.

    \param
        eventName the name of the event we want to subscribe to,
        CEGUI::Window::EventClicked for example

    \param
        action is the action that will be invoked on the animation instance if
        this event is fired

    \par
        Auto Subscription does subscribe to event sender (usually target window)
        of Animation Instance when the event source is set.

        Usable action strings:
        - Start
        - Stop
        - Pause
        - Unpause
        - TogglePause

        eventName is the name of the event we want to subscribe to
    */
    void defineAutoSubscription(const String& eventName, const String& action);

    /*!
    \brief
        This undefines previously defined auto subscription.

    \see
        Animation::defineAutoSubscription
    */
    void undefineAutoSubscription(const String& eventName,
                                  const String& action);

    /*!
    \brief
        This undefines all previously defined auto subscriptions.

    \see
        Animation::defineAutoSubscription
    */
    void undefineAllAutoSubscriptions();

    /*!
    \brief
        Subscribes all auto subscriptions with information from given animation
        instance

    \par
        This is internal method! Only use if you know what you're doing!
    */
    void autoSubscribe(AnimationInstance* instance);

    /*!
    \brief
        Unsubscribes all auto subscriptions with information from given
        animation instance

    \par
        This is internal method! Only use if you know what you're doing!
    */
    void autoUnsubscribe(AnimationInstance* instance);

    /*!
     \brief
        Internal method, causes all properties that are used by this animation
        and it's affectors to be saved

    \par
        So their values are still known after
        they've been affected.
     */
    void savePropertyValues(AnimationInstance* instance);

    /*!
    \brief
        Applies this Animation definition using information from given
        AnimationInstance

    \par
        This is internal method, only use if you know what you're doing!
    */
    void apply(AnimationInstance* instance);

    /*!
    \brief
        Writes an xml representation of this Animation definition to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.
    \param name_override
    	If given, this value overrides the name attribute written to the stream.
    	This is useful when writing out looknfeels
    */
    void writeXMLToStream(XMLSerializer& xml_stream, const String& name_override = "") const;

private:
    //! name of this animation
    String d_name;

    //! currently used replay mode
    ReplayMode d_replayMode;
    //! duration of animation (in seconds)
    float d_duration;
    /** if true, instantiations of this animation call start on themselves when
     * their target is set
     */
    bool d_autoStart;

    typedef std::vector<Affector*
        CEGUI_VECTOR_ALLOC(Affector*)> AffectorList;
    //! list of affectors defined in this animation
    AffectorList d_affectors;

    typedef std::multimap<String, String, std::less<String>
        CEGUI_MAP_ALLOC(String, String)> SubscriptionMap;
    /** holds pairs of 2 strings, the left string is the Event that we will
     * subscribe to, the right string is the action that will be invoked to the
     * instance if the event is fired on target window
     */
    SubscriptionMap d_autoSubscriptions;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIAnimation_h_

