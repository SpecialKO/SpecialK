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
#ifndef _CEGUIFalStateImagery_h_
#define _CEGUIFalStateImagery_h_

#include "./LayerSpecification.h"
#include "../Window.h"
#include <set>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Class the encapsulates imagery for a given widget state.
    */
    class CEGUIEXPORT StateImagery :
        public AllocatedObject<StateImagery>
    {
    public:
        //! Container type for LayerSpecification pointers
        typedef std::vector<LayerSpecification* 
            CEGUI_VECTOR_ALLOC(LayerSpecification*)> LayerSpecificationPointerList;

        /*!
        \brief
            Constructor
        */
        StateImagery():
        	d_clipToDisplay(false)
        {}

        /*!
        \brief
            Constructor

        \param name
            Name of the state
        */
        StateImagery(const String& name);

        /*!
        \brief
            Render imagery for this state.

        \param srcWindow
            Window to use when convering BaseDim values to pixels.

        \return
            Nothing.
        */
        void render(Window& srcWindow, const ColourRect* modcols = 0, const Rectf* clipper = 0) const;

        /*!
        \brief
            Render imagery for this state.

        \param srcWindow
            Window to use when convering BaseDim values to pixels.

        \param baseRect
            Rect to use when convering BaseDim values to pixels.

        \return
            Nothing.
        */
        void render(Window& srcWindow, const Rectf& baseRect, const ColourRect* modcols = 0, const Rectf* clipper = 0) const;

        /*!
        \brief
            Add an imagery LayerSpecification to this state.

        \param layer
            LayerSpecification to be added to this state (will be copied)

        \return
            Nothing.
        */
        void addLayer(const LayerSpecification& layer);

        /*!
        \brief
            Sorts the LayerSpecifications after their priority. Whenever a LayerSpecification, which has been added
            to this StateImagery, is changed, this sort function should be called.
        */
        void sort();

        /*!
        \brief
            Removed all LayerSpecifications from this state.

        \return
            Nothing.
        */
        void clearLayers();

        /*!
        \brief
            Return the name of this state.

        \return
            String object holding the name of the StateImagery object.
        */
        const String& getName() const;

        /*!
        \brief
            Set the name of this state.

        \return
            String object holding the name of the StateImagery object.
        */
        void setName(const String& name);

        /*!
        \brief
            Return whether this state imagery should be clipped to the display rather than the target window.

            Clipping to the display effectively implies that the imagery should be rendered unclipped.

        /return
            - true if the imagery will be clipped to the display area.
            - false if the imagery will be clipped to the target window area.
        */
        bool isClippedToDisplay() const;

        /*!
        \brief
            Set whether this state imagery should be clipped to the display rather than the target window.

            Clipping to the display effectively implies that the imagery should be rendered unclipped.

        \param setting
            - true if the imagery should be clipped to the display area.
            - false if the imagery should be clipped to the target window area.

        \return
            Nothing.
        */
        void setClippedToDisplay(bool setting);

        /*!
        \brief
            Writes an xml representation of this StateImagery to \a out_stream.

        \param xml_stream
            Stream where xml data should be output.


        \return
            Nothing.
        */
        void writeXMLToStream(XMLSerializer& xml_stream) const;


        /*!
        \brief
            Returns a vector of pointers to the LayerSpecifications that are currently added to this StateImagery.
            If a LayerSpecification is added or removed from this StateImagery, then the pointers in this vector are
            not valid anymore. The function should then be called again to retrieve valid pointers.

         \note
             Whenever a pointer from this list is changed in a way that the multiset needs to be resorted, the sort
             function of this class must be called.

         \return
            A vector of pointers to the LayerSpecifications that are currently added to this StateImagery
        */
        LayerSpecificationPointerList getLayerSpecificationPointers();

    private:
        /*!
        \deprecated 
            This type is deprecated because it doesn't use CEGUI allocators and is private and needs to be replaced with a vector that gets
            sorted whenever needed. std::multisets and sets make their elements constant which introduces issues when changing them. In the next
            version a new public type "LayerSpecificationList" will replace this one.
        */
        typedef std::multiset<LayerSpecification> LayersList;

        CEGUI::String   d_stateName;    //!< Name of this state.
        LayersList      d_layers;       //!< Collection of LayerSpecification objects to be drawn for this state.
        bool            d_clipToDisplay; //!< true if Imagery for this state should be clipped to the display instead of winodw (effectively, not clipped).
    public:
        //! \deprecated This type is deprecated and will be removed in the next version. A const reference to the Container type of getLayerSpecifications will replace this.
        typedef ConstVectorIterator<LayersList> LayerIterator;

        //! \deprecated This function is deprecated. Instead the getLayerSpecifications will be used in the next version and getLayerSpecificationPointers can be used for editing.
        LayerIterator getLayerIterator() const;

    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif  // end of guard _CEGUIFalStateImagery_h_
