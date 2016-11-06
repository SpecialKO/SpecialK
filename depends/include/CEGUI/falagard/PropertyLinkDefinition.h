/***********************************************************************
    created:    Sat Oct 8 2005
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
#ifndef _CEGUIFalPropertyLinkDefinition_h_
#define _CEGUIFalPropertyLinkDefinition_h_

#include "CEGUI/falagard/FalagardPropertyBase.h"
#include "CEGUI/falagard/XMLHandler.h"
#include "CEGUI/IteratorBase.h"
#include <vector>

#if defined (_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

namespace CEGUI
{
//! \deprecated This will be removed in the next version as it has been replaced by Falagard_xmlHandler::ParentIdentifier
extern const String S_parentIdentifier;

/*!
\brief
    Class representing a property that links to another property defined on
    an attached child widget.
*/
template <typename T>
class PropertyLinkDefinition : public FalagardPropertyBase<T>
{
public:
    //------------------------------------------------------------------------//
    typedef typename TypedProperty<T>::Helper Helper;

    //------------------------------------------------------------------------//
    PropertyLinkDefinition(const String& propertyName, const String& widgetName,
                           const String& targetProperty, const String& initialValue,
                           const String& origin,
                           bool redrawOnWrite, bool layoutOnWrite,
                           const String& fireEvent, const String& eventNamespace) :
        FalagardPropertyBase<T>(propertyName,
                                Falagard_xmlHandler::PropertyLinkDefinitionHelpDefaultValue,
                                initialValue, origin,
                                redrawOnWrite, layoutOnWrite,
                                fireEvent, eventNamespace)
    {
        // add initial target if it was specified via constructor
        // (typically meaning it came via XML attributes)
        if (!widgetName.empty() || !targetProperty.empty())
            addLinkTarget(widgetName, targetProperty);
    }

    ~PropertyLinkDefinition() {}

    //------------------------------------------------------------------------//
    //! add a new link target to \a property on \a widget (name).
    void addLinkTarget(const String& widget, const String& property)
    {
        d_targets.push_back(std::make_pair(widget,property));
    }

    //------------------------------------------------------------------------//
    //! clear all link targets from this link definition.
    void clearLinkTargets()
    {
        d_targets.clear();
    }

    //------------------------------------------------------------------------//
    // return whether a the given widget / property pair is a target of this
    // property link.
    bool isTargetProperty(const String& widget, const String& property) const
    {
        LinkTargetCollection::const_iterator i = d_targets.begin();
        for (; i != d_targets.end(); ++i)
        {
            if (property == i->second && widget == i->first)
                return true;
        }

        return false;
    }

    //------------------------------------------------------------------------//
    void initialisePropertyReceiver(PropertyReceiver* receiver) const
    {
        updateLinkTargets(receiver, Helper::fromString(FalagardPropertyBase<T>::d_initialValue));
    }

    //------------------------------------------------------------------------//
    Property* clone() const
    {
        return CEGUI_NEW_AO PropertyLinkDefinition<T>(*this);
    }

protected:
    // override members from FalagardPropertyBase
    //------------------------------------------------------------------------//
    typename Helper::safe_method_return_type
    getNative_impl(const PropertyReceiver* receiver) const
    {
        const LinkTargetCollection::const_iterator i(d_targets.begin());

        const Window* const target_wnd =
            getTargetWindow(receiver, i->first);

        // if no target, or target (currently) invalid, return the default value
        if (d_targets.empty() || !target_wnd)
            return Helper::fromString(FalagardPropertyBase<T>::d_initialValue);

        // otherwise return the value of the property for first target, since
        // this is considered the 'master' target for get operations.
        return Helper::fromString(target_wnd->getProperty(i->second.empty() ?
                TypedProperty<T>::d_name : i->second));
    }

    //------------------------------------------------------------------------//
    void setNative_impl(PropertyReceiver* receiver,
                        typename Helper::pass_type value)
    {
        updateLinkTargets(receiver, value);

        // base handles things like ensuring redraws and such happen
        FalagardPropertyBase<T>::setNative_impl(receiver, value);
    }

    //------------------------------------------------------------------------//
    void updateLinkTargets(PropertyReceiver* receiver,
                           typename Helper::pass_type value) const
    {
        LinkTargetCollection::const_iterator i = d_targets.begin();
        for ( ; i != d_targets.end(); ++i)
        {
            Window* target_wnd = getTargetWindow(receiver, i->first);

            // only try to set property if target is currently valid.
            if (target_wnd)
            {
                const CEGUI::String& propertyName = i->second.empty() ? TypedProperty<T>::d_name : i->second;
                CEGUI::String propertyValue = Helper::toString(value);
                target_wnd->setProperty(propertyName, propertyValue);
                target_wnd->banPropertyFromXML(propertyName);
            }
        }
    }

    //------------------------------------------------------------------------//
    void writeDefinitionXMLElementType(XMLSerializer& xml_stream) const
    {
        xml_stream.openTag(Falagard_xmlHandler::PropertyLinkDefinitionElement);
        writeFalagardXMLAttributes(xml_stream);
        writeDefinitionXMLAdditionalAttributes(xml_stream);
    }

    //------------------------------------------------------------------------//
    void writeDefinitionXMLAdditionalAttributes(XMLSerializer& xml_stream) const
    {
        if(FalagardPropertyBase<T>::d_dataType.compare(Falagard_xmlHandler::GenericDataType) != 0)
            xml_stream.attribute(Falagard_xmlHandler::TypeAttribute, FalagardPropertyBase<T>::d_dataType);

        if (!FalagardPropertyBase<T>::d_helpString.empty() && FalagardPropertyBase<T>::d_helpString.compare(CEGUI::Falagard_xmlHandler::PropertyLinkDefinitionHelpDefaultValue) != 0)
            xml_stream.attribute(Falagard_xmlHandler::HelpStringAttribute, FalagardPropertyBase<T>::d_helpString);
    }

    //------------------------------------------------------------------------//
    void writeFalagardXMLAttributes(XMLSerializer& xml_stream) const
    {
        // HACK: Here we abuse some intimate knowledge in that we know it's
        // safe to write our sub-elements out although the function is named
        // for writing attributes.  The alternative was to repeat code from the
        // base class, also demonstrating intimate knowledge ;)

        LinkTargetCollection::const_iterator i(d_targets.begin());

        // if there is one target only, write it out as attributes
        if (d_targets.size() == 1)
        {
            if (!i->first.empty())
                xml_stream.attribute(Falagard_xmlHandler::WidgetAttribute, i->first);

            if (!i->second.empty())
                xml_stream.attribute(Falagard_xmlHandler::TargetPropertyAttribute, i->second);
        }
        // we have multiple targets, so write them as PropertyLinkTarget tags
        else
        {
            for ( ; i != d_targets.end(); ++i)
            {
                xml_stream.openTag(Falagard_xmlHandler::PropertyLinkTargetElement);

                if (!i->first.empty())
                    xml_stream.attribute(Falagard_xmlHandler::WidgetAttribute, i->first);

                if (!i->second.empty())
                    xml_stream.attribute(Falagard_xmlHandler::PropertyAttribute, i->second);

                xml_stream.closeTag();
            }
        }
    }

    //------------------------------------------------------------------------//
    //! Return a pointer to the target window with the given name.
    const Window* getTargetWindow(const PropertyReceiver* receiver,
                                  const String& name) const
    {
        if (name.empty())
            return static_cast<const Window*>(receiver);

        // handle link back to parent.  Return receiver if no parent.
        if (name == Falagard_xmlHandler::ParentIdentifier)
            return static_cast<const Window*>(receiver)->getParent();

        return static_cast<const Window*>(receiver)->getChild(name);
    }

    //------------------------------------------------------------------------//
    //! Return a pointer to the target window with the given name.
    Window* getTargetWindow(PropertyReceiver* receiver,
                            const String& name) const
    {
        return const_cast<Window*>(
            getTargetWindow(static_cast<const PropertyReceiver*>(receiver), name));
    }

    //------------------------------------------------------------------------//
    typedef std::pair<String,String> StringPair;
    //! type used for the collection of targets.
    typedef std::vector<StringPair CEGUI_VECTOR_ALLOC(StringPair)> LinkTargetCollection;

    //! collection of targets for this PropertyLinkDefinition.
    LinkTargetCollection d_targets;

public:
    typedef ConstVectorIterator<LinkTargetCollection> LinkTargetIterator;

    LinkTargetIterator getLinkTargetIterator() const
    {
        return LinkTargetIterator(d_targets.begin(),d_targets.end());
    }
};

}

#if defined (_MSC_VER)
#	pragma warning(pop)
#endif

#endif

