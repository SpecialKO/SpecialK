/***********************************************************************
	created:	21/2/2004
	author:		Paul D Turner
	
	purpose:	Main system include for client code
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
#ifndef _CEGUI_h_
#define _CEGUI_h_

// base stuff
#include "CEGUI/Affector.h"
#include "CEGUI/Animation.h"
#include "CEGUI/AnimationInstance.h"
#include "CEGUI/AnimationManager.h"
#include "CEGUI/BasicImage.h"
#include "CEGUI/BasicRenderedStringParser.h"
#include "CEGUI/BidiVisualMapping.h"
#include "CEGUI/BoundSlot.h"
#include "CEGUI/CentredRenderedString.h"
#include "CEGUI/Clipboard.h"
#include "CEGUI/Colour.h"
#include "CEGUI/ColourRect.h"
#include "CEGUI/CoordConverter.h"
#include "CEGUI/DataContainer.h"
#include "CEGUI/DefaultLogger.h"
#include "CEGUI/DefaultRenderedStringParser.h"
#include "CEGUI/DefaultResourceProvider.h"
#include "CEGUI/DynamicModule.h"
#include "CEGUI/Element.h"
#include "CEGUI/Event.h"
#include "CEGUI/EventArgs.h"
#include "CEGUI/EventSet.h"
#include "CEGUI/Exceptions.h"
#include "CEGUI/FactoryModule.h"
#include "CEGUI/FactoryRegisterer.h"
#include "CEGUI/Font.h"
#include "CEGUI/FontGlyph.h"
#include "CEGUI/FontManager.h"
#include "CEGUI/FormattedRenderedString.h"
#include "CEGUI/GeometryBuffer.h"
#include "CEGUI/GlobalEventSet.h"
#include "CEGUI/GUIContext.h"
#include "CEGUI/Image.h"
#include "CEGUI/ImageCodec.h"
#include "CEGUI/ImageManager.h"
#include "CEGUI/InputEvent.h"
#include "CEGUI/Interpolator.h"
#include "CEGUI/JustifiedRenderedString.h"
#include "CEGUI/KeyFrame.h"
#include "CEGUI/LeftAlignedRenderedString.h"
#include "CEGUI/LinkedEvent.h"
#include "CEGUI/Logger.h"
#include "CEGUI/MouseCursor.h"
#include "CEGUI/NamedElement.h"
#include "CEGUI/Property.h"
#include "CEGUI/PropertyHelper.h"
#include "CEGUI/PropertySet.h"
#include "CEGUI/Quaternion.h"
#include "CEGUI/Rect.h"
#include "CEGUI/RegexMatcher.h"
#include "CEGUI/RenderedString.h"
#include "CEGUI/RenderedStringImageComponent.h"
#include "CEGUI/RenderedStringParser.h"
#include "CEGUI/RenderedStringTextComponent.h"
#include "CEGUI/RenderedStringWidgetComponent.h"
#include "CEGUI/RenderedStringWordWrapper.h"
#include "CEGUI/RenderEffect.h"
#include "CEGUI/RenderEffectManager.h"
#include "CEGUI/Renderer.h"
#include "CEGUI/RenderingContext.h"
#include "CEGUI/RenderingSurface.h"
#include "CEGUI/RenderingWindow.h"
#include "CEGUI/RenderQueue.h"
#include "CEGUI/RenderTarget.h"
#include "CEGUI/RightAlignedRenderedString.h"
#include "CEGUI/Scheme.h"
#include "CEGUI/SchemeManager.h"
#include "CEGUI/ScriptModule.h"
#include "CEGUI/SimpleTimer.h"
#include "CEGUI/Size.h"
#include "CEGUI/String.h"
#include "CEGUI/StringTranscoder.h"
#include "CEGUI/System.h"
#include "CEGUI/SystemKeys.h"
#include "CEGUI/Texture.h"
#include "CEGUI/TextureTarget.h"
#include "CEGUI/TextUtils.h"
#include "CEGUI/TplInterpolators.h"
#include "CEGUI/TplWindowFactory.h"
#include "CEGUI/TplWindowRendererFactory.h"
#include "CEGUI/TplWindowFactoryRegisterer.h"
#include "CEGUI/TplWRFactoryRegisterer.h"
#include "CEGUI/Vector.h"
#include "CEGUI/Version.h"
#include "CEGUI/Vertex.h"
#include "CEGUI/WindowFactory.h"
#include "CEGUI/WindowFactoryManager.h"
#include "CEGUI/Window.h"
#include "CEGUI/WindowManager.h"
#include "CEGUI/WindowRendererManager.h"
#include "CEGUI/XMLAttributes.h"
#include "CEGUI/XMLHandler.h"
#include "CEGUI/XMLParser.h"
#include "CEGUI/XMLSerializer.h"

// Falagard core system include
#include "CEGUI/falagard/WidgetLookManager.h"

// gui elements
#include "CEGUI/widgets/All.h"

#endif	// end of guard _CEGUI_h_
