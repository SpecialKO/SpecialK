/***********************************************************************
    created:    Wed Aug 11 2010
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
#ifndef _CEGUIAnimation_xmlHandler_h_
#define _CEGUIAnimation_xmlHandler_h_

#include "CEGUI/ChainedXMLHandler.h"
#include "CEGUI/String.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! Class used to parse stand alone Animation XML files.
class CEGUIEXPORT Animation_xmlHandler : public ChainedXMLHandler
{
public:
	//! String holding the element handled by this class.
    static const String ElementName;

    Animation_xmlHandler();
    virtual ~Animation_xmlHandler();

    // XMLHandler overrides
    const String& getSchemaName() const;
    const String& getDefaultResourceGroup() const;

protected:
    // implement ChainedXMLHandler interface.
    void elementStartLocal(const String& element,
                           const XMLAttributes& attributes);
    void elementEndLocal(const String& element);
};

//----------------------------------------------------------------------------//
//! Chained sub-handler for AnimationDefinition XML elements
class CEGUIEXPORT AnimationDefinitionHandler : public ChainedXMLHandler
{
public:
    //! String holding the element handled by this class.
    static const String ElementName;

    static const String NameAttribute;
    static const String DurationAttribute;

    static const String ReplayModeAttribute;
    static const String ReplayModeOnce;
    static const String ReplayModeLoop;
    static const String ReplayModeBounce;

    static const String AutoStartAttribute;

    AnimationDefinitionHandler(const XMLAttributes& attributes,
                               const String& name_prefix);
    virtual ~AnimationDefinitionHandler();

protected:
    // implement ChainedXMLHandler interface.
    void elementStartLocal(const String& element,
                           const XMLAttributes& attributes);
    void elementEndLocal(const String& element);

    //! Pointer to the Animation created by this handler.
    Animation* d_anim;
};

//----------------------------------------------------------------------------//
//! Chained sub-handler for Affector XML elements
class CEGUIEXPORT AnimationAffectorHandler : public ChainedXMLHandler
{
public:
    //! String holding the element handled by this class.
    static const String ElementName;

    static const String TargetPropertyAttribute;
    static const String InterpolatorAttribute;

    static const String ApplicationMethodAttribute;
    static const String ApplicationMethodAbsolute;
    static const String ApplicationMethodRelative;
    static const String ApplicationMethodRelativeMultiply;

    AnimationAffectorHandler(const XMLAttributes& attributes,
                             Animation& anim);
    virtual ~AnimationAffectorHandler();

protected:
    // implement ChainedXMLHandler interface.
    void elementStartLocal(const String& element,
                           const XMLAttributes& attributes);
    void elementEndLocal(const String& element);

    //! Affector created by this handler.
    Affector* d_affector;
};

//----------------------------------------------------------------------------//
//! Chained sub-handler for KeyFrame XML elements.
class CEGUIEXPORT AnimationKeyFrameHandler : public ChainedXMLHandler
{
public:
    //! String holding the element handled by this class.
    static const String ElementName;

    static const String PositionAttribute;
    static const String ValueAttribute;
    static const String SourcePropertyAttribute;

    static const String ProgressionAttribute;
    static const String ProgressionLinear;
    static const String ProgressionDiscrete;
    static const String ProgressionQuadraticAccelerating;
    static const String ProgressionQuadraticDecelerating;

    AnimationKeyFrameHandler(const XMLAttributes& attributes,
                             Affector& affector);
    virtual ~AnimationKeyFrameHandler();

protected:
    // implement ChainedXMLHandler interface.
    void elementStartLocal(const String& element,
                           const XMLAttributes& attributes);
    void elementEndLocal(const String& element);
};

//----------------------------------------------------------------------------//
//! Chained sub-handler for Subscription XML elements.
class CEGUIEXPORT AnimationSubscriptionHandler : public ChainedXMLHandler
{
public:
    //! String holding the element handled by this class.
    static const String ElementName;

    static const String EventAttribute;
    static const String ActionAttribute;

    AnimationSubscriptionHandler(const XMLAttributes& attributes,
                                 Animation& anim);
    virtual ~AnimationSubscriptionHandler();

protected:
    // implement ChainedXMLHandler interface.
    void elementStartLocal(const String& element,
                           const XMLAttributes& attributes);
    void elementEndLocal(const String& element);
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIAnimation_xmlHandler_h_

