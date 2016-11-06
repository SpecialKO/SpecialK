/***********************************************************************
    created:    Wed Mar 25 2009
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIOgreImageCodec_h_
#define _CEGUIOgreImageCodec_h_

#include "../../ImageCodec.h"
#include "CEGUI/RendererModules/Ogre/Renderer.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    ImageCodec object that loads data via image loading facilities in Ogre.
*/
class OGRE_GUIRENDERER_API OgreImageCodec : public ImageCodec
{
public:
    //! Constructor.
    OgreImageCodec();

    /*!
    \brief
        Set the file-type identifier that will be used for future load
        operations.

        This allows us to pass the type on to Ogre when we process the image
        data (because it's just file data; we do not have a filename nor file
        extension).  Ogre needs this sometimes in order to correctly select the
        right codec to use for the final decoding of the data.  If this value
        is not set, loading may still succeed, though that will depend upon the
        specific libraries and codecs that the Ogre installation has available
        to it.

    \param type
        String object that describes the type of file data that will be passed
        in subsequent load operations.  Note that this type will typically be
        the file extension (or equivalent).
    */
    void setImageFileDataType(const String& type);

    /*!
    \brief
        Return the string descibing the currently set file type.
    */
    const String& getImageFileDataType() const;

    // implement required function from ImageCodec.
    Texture* load(const RawDataContainer& data, Texture* result);

protected:
    //! Holds currently set file data type specifier (i.e. the file extension).
    String d_dataTypeID;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOgreImageCodec_h_
