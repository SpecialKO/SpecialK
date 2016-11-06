/***********************************************************************
    created:    Mon Jun 13 2005
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIFalLayerSpecification_h_
#define _CEGUIFalLayerSpecification_h_

#include "./SectionSpecification.h"
#include "../Window.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Class that encapsulates a single layer of imagery.
    */
    class CEGUIEXPORT LayerSpecification :
        public AllocatedObject<LayerSpecification>
    {
    public:
        //! The container type for SectionSpecifications
        typedef std::vector<SectionSpecification*
            CEGUI_VECTOR_ALLOC(SectionSpecification*)> SectionSpecificationPointerList;


        /*!
        \brief
            Constructor.

        \param priority
            Specifies the priority of the layer.  Layers with higher priorities will be drawn on top
            of layers with lower priorities.
        */
        LayerSpecification(uint priority = 0);

        /*!
        \brief
            Render this layer.

        \param srcWindow
            Window to use when calculating pixel values from BaseDim values.

        \return
            Nothing.
        */
        void render(Window& srcWindow, const ColourRect* modcols = 0, const Rectf* clipper = 0, bool clipToDisplay = false) const;

        /*!
        \brief
            Render this layer.

        \param srcWindow
            Window to use when calculating pixel values from BaseDim values.

        \param baseRect
            Rect to use when calculating pixel values from BaseDim values.

        \return
            Nothing.
        */
        void render(Window& srcWindow, const Rectf& baseRect, const ColourRect* modcols = 0, const Rectf* clipper = 0, bool clipToDisplay = false) const;

        /*!
        \brief
            Add a section specification to the layer.

            A section specification is a reference to a named ImagerySection within the WidgetLook.

        \param section
            SectionSpecification object descibing the section that should be added to this layer.

        \return
            Nothing,
        */
        void addSectionSpecification(const SectionSpecification& section);

        /*!
        \brief
            Clear all section specifications from this layer,

        \return
            Nothing.
        */
        void clearSectionSpecifications();

        /*!
        \brief
            Return the priority of this layer.

        \return
            uint value descibing the priority of this LayerSpecification.
        */
        uint getLayerPriority() const;
        /*!
        \brief
            Sets the priority of this layer.

        \return
            uint value descibing the priority of this LayerSpecification.
        */
        void setLayerPriority(uint priority);

        /*!
        \brief
            Writes an xml representation of this Layer to \a out_stream.

        \param xml_stream
            Stream where xml data should be output.


        \return
            Nothing.
        */
        void writeXMLToStream(XMLSerializer& xml_stream) const;

        /*!
        \brief
            Returns a vector of pointers to the SectionSpecifications that are currently added to this LayerSpecification. If a
            SectionSpecification is added or removed from this LayerSpecification, then the pointers in this vector are not valid
            anymore. The function should then be called again to retrieve valid pointers.

         \return
            A vector of pointers to the SectionSpecifications that are currently added to this LayerSpecification
        */
        SectionSpecificationPointerList getSectionSpecificationPointers();  


        //! The comparison operator, which is used for sorting
        bool operator< (const LayerSpecification& otherLayerSpec) const;

    private:
        //! \deprecated This type will be made public in the next version and be renamed to SectionSpecificationList.
        typedef std::vector<SectionSpecification
            CEGUI_VECTOR_ALLOC(SectionSpecification)> SectionList;

        SectionList d_sections;         //!< Collection of SectionSpecification objects descibing the sections to be drawn for this layer.
        uint        d_layerPriority;    //!< Priority of the layer.
    public:
        //! \deprecated This type will be removed in the next version and replaced by a const list in the getSectionSpecifications function.
        typedef ConstVectorIterator<SectionList> SectionIterator;
        //! \deprecated This function will be replaced by the getSectionSpecifications function in the next version. For editing getSectionSpecificationPointers can be used.
        SectionIterator getSectionIterator() const;
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIFalLayerSpecification_h_
