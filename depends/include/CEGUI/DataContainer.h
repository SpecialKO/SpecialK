/***********************************************************************
	created:	10/8/2004
	author:		James '_mental_' O'Sullivan
	
	purpose:	Declares the RawDataContainer
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIDataContainer_h_
#define _CEGUIDataContainer_h_

#include "CEGUI/Base.h"

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Class used as the databuffer for loading files throughout the library.
*/
class CEGUIEXPORT RawDataContainer :
    public AllocatedObject<RawDataContainer>
{
public:
	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for RawDataContainer class
	*/
    RawDataContainer()
      : mData(0),
        mSize(0)
    {
    }

	/*!
	\brief
		Destructor for RawDataContainer class
	*/
	~RawDataContainer(void) 
    { 
        release();
    }

	/*************************************************************************
		Accessor functions
	*************************************************************************/
	/*!
	\brief
		Set a pointer to the external data.

	\param data
        Pointer to the uint8 data buffer.
	*/
    void setData(uint8* data) { mData = data; }

	/*!
	\brief
		Return a pointer to the external data

	\return
		Pointer to an the uint8 data buffer.
	*/
    uint8* getDataPtr(void) { return mData; }
    const uint8* getDataPtr(void) const { return mData; }

	/*!
	\brief
	    Set the size of the external data.

	\param size
	    size_t containing the size of the external data
	*/
    void setSize(size_t size) { mSize = size; }

	/*!
	\brief
		Get the size of the external data.

	\return
	    size_t containing the size of the external data
	*/
    size_t getSize(void) const { return mSize; }

	/*!
	\brief
		Release supplied data.
	*/
    void release(void);

private:
    /*************************************************************************
		Implementation Data
	*************************************************************************/
    uint8* mData;
    size_t mSize;
};

} // End of  CEGUI namespace section

#endif	// end of guard _CEGUIDataContainer_h_
