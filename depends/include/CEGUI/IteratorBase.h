/***********************************************************************
	created:	26/7/2004
	author:		Paul D Turner
	
	purpose:	Defines interface for base iterator class
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
/*************************************************************************
	This is based somewhat on MapIterator in the Ogre library (www.ogre3d.org)
*************************************************************************/
#ifndef _CEGUIIteratorBase_h_
#define _CEGUIIteratorBase_h_

#include "CEGUI/Base.h"


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Base class constant iterator used to offer iteration over various collections within the system.
*/
template<typename T, typename V = typename T::value_type>
class ConstBaseIterator
{
public:
	typedef V value_type;

	/*!
	\brief
		ConstBaseIterator constructor

	\param start_iter
		'real' iterator that will be the start of the range to be iterated over by this iterator.

	\param end_iter
		'real' iterator that will be the end of the range to be iterated over by this iterator.
	*/
	ConstBaseIterator(typename T::const_iterator start_iter, typename T::const_iterator end_iter) :
		d_currIter(start_iter),
		d_startIter(start_iter),
		d_endIter(end_iter)
	{
	}

	
	/*!
	\brief
		ConstBaseIterator destructor
	*/
	virtual ~ConstBaseIterator(void)
	{
	}


	/*!
	\brief
		ConstBaseIterator copy constructor
	*/
	ConstBaseIterator(const ConstBaseIterator<T, V>& org) :
		d_currIter(org.d_currIter),
		d_startIter(org.d_startIter),
		d_endIter(org.d_endIter)
	{
	}


	/*!
	\brief
		ConstBaseIterator assignment operator
	*/
	ConstBaseIterator<T, V>&	operator=(const ConstBaseIterator<T, V>& rhs)
	{
		d_currIter	= rhs.d_currIter;
		d_startIter	= rhs.d_startIter;
		d_endIter	= rhs.d_endIter;

		return *this;
	}


	/*!
	\brief
		Return the value for the item at the current iterator position.
	*/
	virtual value_type	getCurrentValue(void) const = 0;


	/*!
	\brief
		Return whether the current iterator position is at the end of the iterators range.
	*/
	bool	isAtEnd(void) const
	{
		return d_currIter == d_endIter;
	}


	/*!
	\brief
		Return whether the current iterator position is at the start of the iterators range.
	*/
	bool	isAtStart(void) const
	{
		return d_currIter == d_startIter;
	}

	/*!
	\brief
		Compares two iterators.  Return true if the current position of both iterators are equivalent.
	*/
	bool	operator==(const ConstBaseIterator<T, V>& rhs) const
	{
		return d_currIter == rhs.d_currIter;
	}


	/*!
	\brief
		Compares two iterators.  Return true if the current position of the iterators are different.
	*/
	bool	operator!=(const ConstBaseIterator<T, V>& rhs) const
	{
		return !operator==(rhs);
	}


	/*!
	\brief
		Return the value for the current iterator position.
	*/
	value_type	operator*() const
	{
		return getCurrentValue();
	}


	/*!
	\brief
		Set the iterator current position to the start position.
	*/
	void	toStart(void)
	{
		d_currIter = d_startIter;
	}


	/*!
	\brief
		Set the iterator current position to the end position.
	*/
	void	toEnd(void)
	{
		d_currIter = d_endIter;
	}


protected:
	/*************************************************************************
		No default construction available
	*************************************************************************/
	ConstBaseIterator(void) {}

	/*************************************************************************
		Implementation Data
	*************************************************************************/
	typename T::const_iterator	d_currIter;		//!< 'real' iterator describing the current position within the collection.
	typename T::const_iterator	d_startIter;	//!< 'real' iterator describing the start position within the collection (or what we were told was the start).
	typename T::const_iterator	d_endIter;		//!< 'real' iterator describing the end position within the collection (or what we were told was the end).
};

//! iterator class for maps
template<class T>
class ConstMapIterator : public ConstBaseIterator<T, typename T::mapped_type>
{
public:
	ConstMapIterator(typename T::const_iterator start_iter, typename T::const_iterator end_iter) :
        ConstBaseIterator<T, typename T::mapped_type>(start_iter, end_iter)
    {}

    typename ConstBaseIterator<T, typename T::mapped_type>::value_type
    getCurrentValue() const
    {
        return this->d_currIter->second;
    }

    /*!
    \brief
        Return the key for the item at the current iterator position.
    */
    typename T::key_type getCurrentKey() const
    {
        return this->d_currIter->first;
    }

    /*!
    \brief
        Increase the iterator position (prefix increment).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstMapIterator<T>&    operator++()
    {
        if (ConstBaseIterator<T, typename T::mapped_type>::d_currIter != ConstBaseIterator<T, typename T::mapped_type>::d_endIter)
            ++ConstBaseIterator<T, typename T::mapped_type>::d_currIter;

        return *this;
    }

    /*!
    \brief
        Decrease the iterator position (prefix decrement).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstMapIterator<T>&    operator--()
    {
        if (ConstBaseIterator<T, typename T::mapped_type>::d_currIter != ConstBaseIterator<T, typename T::mapped_type>::d_startIter)
            --ConstBaseIterator<T, typename T::mapped_type>::d_currIter;

        return *this;
    }

    /*!
    \brief
        Increase the iterator position (postfix increment).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstMapIterator<T> operator++(int)
    {
        ConstMapIterator<T> tmp = *this;
        ++*this;

        return tmp;
    }

    /*!
    \brief
        Decrease the iterator position (postfix decrement).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstMapIterator<T> operator--(int)
    {
        ConstMapIterator<T> tmp = *this;
        --*this;

        return tmp;
    }

protected:
    /*************************************************************************
		No default construction available
	*************************************************************************/
    ConstMapIterator(void) {}
};

//! iterator for vectors
template<class T>
class ConstVectorIterator : public ConstBaseIterator<T>
{
public:
	ConstVectorIterator(typename T::const_iterator start_iter, typename T::const_iterator end_iter) :
        ConstBaseIterator<T>(start_iter, end_iter)
    {}

    typename ConstBaseIterator<T>::value_type
    getCurrentValue() const
    {
        return *this->d_currIter;
    }

    /*!
    \brief
        Increase the iterator position (prefix increment).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstVectorIterator<T>&    operator++()
    {
        if (ConstBaseIterator<T, typename T::value_type>::d_currIter != ConstBaseIterator<T, typename T::value_type>::d_endIter)
            ++ConstBaseIterator<T, typename T::value_type>::d_currIter;

        return *this;
    }

    /*!
    \brief
        Decrease the iterator position (prefix decrement).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstVectorIterator<T>&    operator--()
    {
        if (ConstBaseIterator<T, typename T::value_type>::d_currIter != ConstBaseIterator<T, typename T::value_type>::d_startIter)
            --ConstBaseIterator<T, typename T::value_type>::d_currIter;

        return *this;
    }

    /*!
    \brief
        Increase the iterator position (postfix increment).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstVectorIterator<T> operator++(int)
    {
        ConstVectorIterator<T> tmp = *this;
        ++*this;

        return tmp;
    }

    /*!
    \brief
        Decrease the iterator position (postfix decrement).

    \note
        The iterator is checked, and this call will always succeed, so do not rely on some exception to exit a loop.
    */
    ConstVectorIterator<T> operator--(int)
    {
        ConstVectorIterator<T> tmp = *this;
        --*this;

        return tmp;
    }

protected:
    /*************************************************************************
		No default construction available
	*************************************************************************/
    ConstVectorIterator(void) {}
};

} // End of  CEGUI namespace section


#endif	// end of guard _CEGUIIteratorBase_h_
