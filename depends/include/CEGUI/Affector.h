/***********************************************************************
    created:    7/8/2010
    author:     Martin Preisler

    purpose:    Defines the interface for the Affector clas
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
#ifndef _CEGUIAffector_h_
#define _CEGUIAffector_h_

#include "CEGUI/String.h"
#include "CEGUI/KeyFrame.h"
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
    Defines an 'affector' class

    Affector is part of Animation definition. It is set to affect
    one Property using one Interpolator.

\todo
    moveKeyFrame, this will be vital for any animation editing tools
*/
class CEGUIEXPORT Affector : public AllocatedObject<Affector>
{
public:
    //! enumerates the possible methods of application
    enum ApplicationMethod
    {
        //! applies values as absolutes
        AM_Absolute,

        /** saves a base value after the animation is started and applies
         * relatively to that
         */
        AM_Relative,

        /** saves a base value after the animation is started and applies
         * by multiplying this base value with key frame floats
         */
        AM_RelativeMultiply
    };

    /** internal constructor, please construct Affectors via
     * Animation::createAffector only
     */
    Affector(Animation* parent);

    //! destructor, this destroys all key frames defined inside this affector
    ~Affector(void);

    /*!
    \brief
    	Retrieves the parent animation of this keyframe
    */
    inline Animation* getParent() const
    {
    	return d_parent;
    }

    /*!
    \brief
        Retrieves index with which this affector is retrievable in parent Animation

    \note
        The index is only valid as long as the list of affectors is unchanged in animation!
    */
    size_t getIdxInParent() const;

    /*!
    \brief
        Sets the application method

    \par
        Values can be applied in 2 ways - as absolute values or relative to base
        value that is retrieved and saved after animation is started
    */
    void setApplicationMethod(ApplicationMethod method);

    /*!
    \brief
        Retrieves current application method

    \see
        Affector::setApplicationMethod
    */
    ApplicationMethod getApplicationMethod() const;

    /*!
    \brief
        Sets the property that will be affected
    */
    void setTargetProperty(const String& target);

    /*!
    \brief
        Gets the property that will be affected
    */
    const String& getTargetProperty() const;

    /*!
    \brief
        Sets interpolator of this Affector

    \par
        Interpolator has to be set for the Affector to work!
    */
    void setInterpolator(Interpolator* interpolator);

    /*!
    \brief
        Sets interpolator of this Affector

    \par
        Interpolator has to be set for the Affector to work!
    */
    void setInterpolator(const String& name);

    /*!
    \brief
        Retrieves currently used interpolator of this Affector
    */
    Interpolator* getInterpolator() const;

    /*!
    \brief
        Creates a KeyFrame at given position
    */
    KeyFrame* createKeyFrame(float position);

    /*!
    \brief
        Creates a KeyFrame at given position

    \par
        This is a helper method, you can set all these values after you create
        the KeyFrame
    */
    KeyFrame* createKeyFrame(float position, const String& value,
                             KeyFrame::Progression progression = KeyFrame::P_Linear,
                             const String& sourceProperty = "");

    /*!
    \brief
        Destroys given keyframe
    */
    void destroyKeyFrame(KeyFrame* keyframe);

    /*!
    \brief
        Retrieves a KeyFrame at given position
    */
    KeyFrame* getKeyFrameAtPosition(float position) const;

    /*!
    \brief
        Checks whether there is a key frame at given position
    */
    bool hasKeyFrameAtPosition(float position) const;

    /*!
    \brief
        Retrieves a KeyFrame with given index
    */
    KeyFrame* getKeyFrameAtIdx(size_t index) const;

    /*!
    \brief
        Returns number of key frames defined in this affector
    */
    size_t getNumKeyFrames() const;

    /*!
    \brief
        Moves given key frame to given new position
    */
    void moveKeyFrameToPosition(KeyFrame* keyframe, float newPosition);

    /*!
    \brief
        Moves key frame at given old position to given new position
    */
    void moveKeyFrameToPosition(float oldPosition, float newPosition);

    /*!
     \brief
        Internal method, causes all properties that are used by this affector
        and it's keyframes to be saved

    \par
        So their values are still known after
        they've been affected.
     */
    void savePropertyValues(AnimationInstance* instance);

    /*!
    \brief
        Applies this Affector's definition with parameters from given
        Animation Instance

    \par
        This function is internal so unless you know what you're doing, don't
        touch!

    \see
        AnimationInstance
    */
    void apply(AnimationInstance* instance);

    /*!
    \brief
        Writes an xml representation of this Affector to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.
    */
    void writeXMLToStream(XMLSerializer& xml_stream) const;

private:
    //! parent animation definition
    Animation* d_parent;
    //! application method
    ApplicationMethod d_applicationMethod;
    //! property that gets affected by this affector
    String d_targetProperty;
    //! curently used interpolator (has to be set for the Affector to work!)
    Interpolator* d_interpolator;

    typedef std::map<float, KeyFrame*, std::less<float>
        CEGUI_MAP_ALLOC(float, KeyFrame*)> KeyFrameMap;
    /** keyframes of this affector (if there are no keyframes, this affector
     * won't do anything!)
     */
    KeyFrameMap d_keyFrames;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIAffector_h_

