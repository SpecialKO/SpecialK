/***********************************************************************
    created:    7/8/2010
    author:     Martin Preisler

    purpose:    Defines the interface for the KeyFrame class
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to de/al in the Software without restriction, including
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
#ifndef _CEGUIKeyFrame_h_
#define _CEGUIKeyFrame_h_

#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Defines a 'key frame' class

    Key frames are defined inside Affectors. The values they hold are used
    when animation is precisely at the key frame's position. If it's between
    two key frames, the value is interpolated.

\see
    Affector
*/
class CEGUIEXPORT KeyFrame :
    public AllocatedObject<KeyFrame>
{
public:
    //! enumerates possible progression methods, IE how the value progresses
    //! TOWARS this key frames, this means that progression method of the first
    //! key frame won't be used for anything!
    enum Progression
    {
        //! linear progression
        P_Linear,
        //! progress is accelerated, starts slow and speeds up
        P_QuadraticAccelerating,
        //! progress is decelerated, starts fast and slows down
        P_QuadraticDecelerating,
        /** left neighbour's value is picked if interpolation position is lower
         * than 1.0, right is only picked when interpolation position is exactly
         * 1.0
         */
        P_Discrete
    };

    //! internal constructor, please use Affector::createKeyFrame
    KeyFrame(Affector* parent, float position);

    //! internal destructor, please use Affector::destroyKeyFrame
    ~KeyFrame(void);

    /*!
    \brief
        Retrieves parent Affector of this Key Frame
    */
    Affector* getParent() const;

    /*!
    \brief
        Retrieves index with which this keyframe is retrievable in parent Affector

    \note
        The index is only valid as long as the list of affectors is unchanged in animation!
    */
    size_t getIdxInParent() const;

    /*!
    \brief
        Moves this keyframe to a new given position
    */
    void moveToPosition(float newPosition);

    /*!
    \brief
        Retrieves position of this key frame in the animation (in seconds)
    */
    float getPosition() const;

    /*!
    \brief
        Sets the value of this key frame

    \par
        This is only used if source property is empty!

    \see
        KeyFrame::setSourceProperty
    */
    void setValue(const String& value);

    /*!
    \brief
        Retrieves value of this key frame
    */
    const String& getValue() const;

    /*!
    \brief
        Sets the source property of this key frame

    \par
        Key frame can get it's value from 2 places, it's either stored inside
        it (setValue, getValue methods) or it's linked to a property
        (setSourcePropery, getSourceProperty).

        The decision about what value is used is simple, if there is a source
        property (sourceProperty is not empty, it's used)
    */
    void setSourceProperty(const String& sourceProperty);

    /*!
    \brief
        Gets the source property of this key frame
    */
    const String& getSourceProperty() const;

    /*!
    \brief
        Retrieves value of this for use when animating

    \par
        This is an internal method! Only use if you know what you're doing!

    \par
        This returns the base property value if source property is set on this
        keyframe, it works the same as getValue() if source property is empty
    */
    const String& getValueForAnimation(AnimationInstance* instance) const;

    /*!
    \brief
        Sets the progression method of this key frame

    \par
        This controls how the animation will progress TOWARDS this key frame,
        whether it will be a linear motion, accelerated, decelerated, etc...

        That means that the progression of the first key frame is never used!

        Please see KeyFrame::Progression
    */
    void setProgression(Progression p);

    /*!
    \brief
        Retrieves progression method of this key frame
    */
    Progression getProgression() const;

    /*!
    \brief
        Internal method, alters interpolation position based on progression
        method. Don't use unless you know what you're doing!
    */
    float alterInterpolationPosition(float position);

    /*!
    \brief
        Internal method, if this keyframe is using source property, this
        saves it's value to given instance before it's affected
    */
    void savePropertyValue(AnimationInstance* instance);

    /*!
    \brief
        internal method, notifies this keyframe that it has been moved

    \par
        DO NOT CALL DIRECTLY, should only be used by Affector class

    \see
        KeyFrame::moveToPosition
    */
    void notifyPositionChanged(float newPosition);

    /*!
	\brief
		Writes an xml representation of this KeyFrame to \a out_stream.

	\param xml_stream
		Stream where xml data should be output.
	*/
	void writeXMLToStream(XMLSerializer& xml_stream) const;

private:
    //! parent affector
    Affector* d_parent;
    //! position of this key frame in the animation's timeline (in seconds)
    float d_position;

    //! value of this key frame - key value
    String d_value;
    //! source property
    String d_sourceProperty;
    //! progression method used towards this key frame
    Progression d_progression;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIKeyFrame_h_

