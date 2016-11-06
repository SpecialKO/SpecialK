/***********************************************************************
	created:	8/3/2004
	author:		Paul D Turner
	
	purpose:	Defines class representing colours for four corners of a
				rectangle
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
#ifndef _CEGUIColourRect_h_
#define _CEGUIColourRect_h_

#include "CEGUI/Base.h"
#include "CEGUI/Colour.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Class that holds details of colours for the four corners of a rectangle.
*/
class CEGUIEXPORT ColourRect :
    public AllocatedObject<ColourRect>
{
public:
	/*!
	\brief
		Default constructor
	*/
	ColourRect(void);


	/*!
	\brief
		Constructor for ColourRect objects (via single colour).  Also handles default construction.
	*/
	ColourRect(const Colour& col);


	/*!
	\brief
		Constructor for ColourRect objects
	*/
	ColourRect(const Colour& top_left, const Colour& top_right, const Colour& bottom_left, const Colour& bottom_right);


	/*!
	\brief
		Set the alpha value to use for all four corners of the ColourRect.

	\param alpha
		Alpha value to use.

	\return
		Nothing.
	*/
	void	setAlpha(float alpha);


	/*!
	\brief
		Set the alpha value to use for the top edge of the ColourRect.

	\param alpha
		Alpha value to use.

	\return
		Nothing.
	*/
	void	setTopAlpha(float alpha);


	/*!
	\brief
		Set the alpha value to use for the bottom edge of the ColourRect.

	\param alpha
		Alpha value to use.

	\return
		Nothing.
	*/
	void	setBottomAlpha(float alpha);


	/*!
	\brief
		Set the alpha value to use for the left edge of the ColourRect.

	\param alpha
		Alpha value to use.

	\return
		Nothing.
	*/
	void	setLeftAlpha(float alpha);


	/*!
	\brief
		Set the alpha value to use for the right edge of the ColourRect.

	\param alpha
		Alpha value to use.

	\return
		Nothing.
	*/
	void	setRightAlpha(float alpha);


	/*!
	\brief
		Determinate the ColourRect is monochromatic or variegated.

	\return
		True if all four corners of the ColourRect has same colour, false otherwise.
	*/
	bool	isMonochromatic() const;


	/*!
	\brief
		Gets a portion of this ColourRect as a subset ColourRect

	\param left
		The left side of this subrectangle (in the range of 0-1 float)
	\param right
		The right side of this subrectangle (in the range of 0-1 float)
	\param top
		The top side of this subrectangle (in the range of 0-1 float)
	\param bottom
		The bottom side of this subrectangle (in the range of 0-1 float)

	\return
		A ColourRect from the specified range
	*/
	ColourRect getSubRectangle( float left, float right, float top, float bottom ) const;

	/*!
	\brief
		Get the colour at a point in the rectangle

	\param x
		The x coordinate of the point
	\param y
		The y coordinate of the point

	\return
		The colour at the specified point.
	*/
	Colour getColourAtPoint( float x, float y ) const;


	/*!
	\brief
		Set the colour of all four corners simultaneously.

	\param col
		colour that is to be set for all four corners of the ColourRect;
	*/
	void	setColours(const Colour& col);


	/*!
	\brief
		Module the alpha components of each corner's colour by a constant.

	\param alpha
		The constant factor to modulate all alpha colour components by.
	*/
	void	modulateAlpha(float alpha);

    /*!
    \brief
        Modulate all components of this colour rect with corresponding components from another colour rect.
    */
    ColourRect& operator*=(const ColourRect& other);

	inline ColourRect operator*(const float val) const
    {       
        return ColourRect(
            d_top_left * val, 
            d_top_right * val, 
            d_bottom_left * val,
            d_bottom_right * val 
        );  
    }

	inline ColourRect operator+(const ColourRect& val) const
    {       
        return ColourRect(
            d_top_left + val.d_top_left, 
            d_top_right + val.d_top_right, 
            d_bottom_left + val.d_bottom_left,
            d_bottom_right + val.d_bottom_right 
        );  
    }


	Colour	d_top_left, d_top_right, d_bottom_left, d_bottom_right;		//!< ColourRect component colours
};

} // End of  CEGUI namespace section


#endif	// end of guard _CEGUIColourRect_h_
