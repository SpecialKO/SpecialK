/***********************************************************************
	created:	03/06/2006
	author:	    Olivier Delannoy
	
	purpose:	Define the abstract interface for all common ImageCodec
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
#ifndef _CEGUIImageCodec_h_
#define _CEGUIImageCodec_h_

#include "CEGUI/Base.h" 
#include "CEGUI/DataContainer.h"
#include "CEGUI/Texture.h" 

// Start of CEGUI namespace section 
namespace CEGUI 
{
/*!
  \brief 
  Abstract ImageLoader class. An image loader encapsulate the loading of a texture.
  
  This class define the loading of an abstract 
*/
class CEGUIEXPORT ImageCodec :
    public AllocatedObject<ImageCodec>
{
public:
    /*!
      \brief 
      Destructor 
    */ 
    virtual ~ImageCodec();
    
protected:
    /*
      \brief 
      Constructor 
      
      \param name of the codec 
    */
    ImageCodec(const String& name);

public: 
    /*!
      \brief 
      Return the name of the image codec object 
      
      Return the name of the image codec 

      \return a string containing image codec name 
    */
    const String& getIdentifierString() const;

    /*! 
      \brief
      Return the list of image file format supported 

      Return a list of space separated image format supported by this
      codec

      \return 
      list of supported image file format separated with space 
    */
    const String& getSupportedFormat() const;
    
    /*!
      \brief 
      Load an image from a memory buffer 

      \param data the image data 

      \param result the texture to use for storing the image data 
     
      \return result on success or 0 if the load failed 
    */
    virtual Texture* load(const RawDataContainer& data, Texture* result) = 0;

private:
    String d_identifierString;   //!< display the name of the codec 

protected:
    String d_supportedFormat;   //!< list all image file format supported 

private:
    ImageCodec(const ImageCodec& obj);
    ImageCodec& operator=(ImageCodec& obj);
}; 

} // End of CEGUI namespace section 

#endif // end of guard _CEGUIImageCodec_h_
