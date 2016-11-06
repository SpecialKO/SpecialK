/***********************************************************************
	created:	26/2/2004
	author:		Paul D Turner

	purpose:	Defines string class used within the GUI system.
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
#ifndef _String_h_
#define _String_h_

#include "CEGUI/Base.h"
#include <cstring>
#include <stdexcept>
#include <cstddef>

// Start of CEGUI namespace section
namespace CEGUI
{
/*************************************************************************
Basic Types
*************************************************************************/
typedef		uint8	utf8;
//typedef		uint16	utf16;  // removed typedef to prevent usage, as utf16 is not supported (yet)
typedef		uint32	utf32;

#if CEGUI_STRING_CLASS == CEGUI_STRING_CLASS_UNICODE

/// encoded char signifies that it's a char (8bit) with encoding (in this case utf8)
typedef utf8 encoded_char;

#define CEGUI_STR_QUICKBUFF_SIZE 32

/*!
\brief
	String class used within the GUI system.

	For the most part, this class can replace std::string in basic usage.  However, currently String does not use the
	current locale, and also comparisons do not take into account the Unicode data tables, so are not 'correct'
	as such.
*/
class CEGUIEXPORT String :
    public AllocatedObject<String>
{
public:
	/*************************************************************************
		Integral Types
	*************************************************************************/
	typedef		utf32			value_type;					//!< Basic 'code point' type used for String (utf32)
	typedef		size_t			size_type;					//!< Unsigned type used for size values and indices
	typedef		std::ptrdiff_t	difference_type;			//!< Signed type used for differences
	typedef		utf32&			reference;					//!< Type used for utf32 code point references
	typedef		const utf32&	const_reference;			//!< Type used for constant utf32 code point references
	typedef		utf32*			pointer;					//!< Type used for utf32 code point pointers
	typedef		const utf32*	const_pointer;				//!< Type used for constant utf32 code point pointers

	static const size_type		npos;						//!< Value used to represent 'not found' conditions and 'all code points' etc.

private:
	/*************************************************************************
		Implementation data
	*************************************************************************/
	size_type	d_cplength;			//!< holds length of string in code points (not including null termination)
	size_type	d_reserve;			//!< code point reserve size (currently allocated buffer size in code points).

	mutable utf8*		d_encodedbuff;		//!< holds string data encoded as utf8 (generated only by calls to c_str() and data())
	mutable size_type	d_encodeddatlen;	//!< holds length of encoded data (in case it's smaller than buffer).
	mutable size_type	d_encodedbufflen;	//!< length of above buffer (since buffer can be bigger then the data it holds to save re-allocations).

	utf32		d_quickbuff[CEGUI_STR_QUICKBUFF_SIZE]; //!< This is a integrated 'quick' buffer to save allocations for smallish strings
	utf32*		d_buffer;							//!< Pointer the the main buffer memory.  This is only valid when quick-buffer is not being used

public:
	/*************************************************************************
		Iterator Classes
	*************************************************************************/
    //! regular iterator for String.
    class iterator : public std::iterator<std::random_access_iterator_tag, utf32>
    {
    public:
        iterator() : d_ptr(0) {}
        explicit iterator(utf32* const ptr) : d_ptr(ptr) {}

        utf32& operator*() const
        {
            return *d_ptr;
        }

        utf32* operator->() const
        {
            return &**this;
        }

        String::iterator& operator++()
        {
            ++d_ptr;
            return *this;
        }

        String::iterator operator++(int)
        {
            String::iterator temp = *this;
            ++*this;
            return temp;
        }

        String::iterator& operator--()
        {
            --d_ptr;
            return *this;
        }

        String::iterator operator--(int)
        {
            String::iterator temp = *this;
            --*this;
            return temp;
        }

        String::iterator& operator+=(difference_type offset)
        {
            d_ptr += offset;
            return *this;
        }

        String::iterator operator+(difference_type offset) const
        {
            String::iterator temp = *this;
            return temp += offset;
        }

        String::iterator& operator-=(difference_type offset)
        {
            return *this += -offset;
        }

        String::iterator operator-(difference_type offset) const
        {
            String::iterator temp = *this;
            return temp -= offset;
        }

        utf32& operator[](difference_type offset) const
        {
            return *(*this + offset);
        }

        friend difference_type operator-(const String::iterator& lhs,
                                         const String::iterator& rhs)
            { return lhs.d_ptr - rhs.d_ptr; }

        friend String::iterator operator+(difference_type offset, const String::iterator& iter)
            { return iter + offset; }

        friend bool operator==(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.d_ptr == rhs.d_ptr; }

        friend bool operator!=(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.d_ptr != rhs.d_ptr; }

        friend bool operator<(const String::iterator& lhs,
                              const String::iterator& rhs)
            { return lhs.d_ptr < rhs.d_ptr; }

        friend bool operator>(const String::iterator& lhs,
                              const String::iterator& rhs)
            { return lhs.d_ptr > rhs.d_ptr; }

        friend bool operator<=(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.d_ptr <= rhs.d_ptr; }

        friend bool operator>=(const String::iterator& lhs,
                               const String::iterator& rhs)
            { return lhs.d_ptr >= rhs.d_ptr; }

        utf32* d_ptr;
    };

    //! const iterator for String.
    class const_iterator : public std::iterator<std::random_access_iterator_tag, const utf32>
    {
    public:
        const_iterator() : d_ptr(0) {}
        explicit const_iterator(const utf32* const ptr) : d_ptr(ptr) {}
        const_iterator(const String::iterator& iter) : d_ptr(iter.d_ptr) {}

        const utf32& operator*() const
        {
            return *d_ptr;
        }

        const utf32* operator->() const
        {
            return &**this;
        }

        String::const_iterator& operator++()
        {
            ++d_ptr;
            return *this;
        }

        String::const_iterator operator++(int)
        {
            String::const_iterator temp = *this;
            ++*this;
            return temp;
        }

        String::const_iterator& operator--()
        {
            --d_ptr;
            return *this;
        }

        String::const_iterator operator--(int)
        {
            String::const_iterator temp = *this;
            --*this;
            return temp;
        }

        String::const_iterator& operator+=(difference_type offset)
        {
            d_ptr += offset;
            return *this;
        }

        String::const_iterator operator+(difference_type offset) const
        {
            String::const_iterator temp = *this;
            return temp += offset;
        }

        String::const_iterator& operator-=(difference_type offset)
        {
            return *this += -offset;
        }

        String::const_iterator operator-(difference_type offset) const
        {
            String::const_iterator temp = *this;
            return temp -= offset;
        }

        const utf32& operator[](difference_type offset) const
        {
            return *(*this + offset);
        }

        String::const_iterator& operator=(const String::iterator& iter)
        {
            d_ptr = iter.d_ptr;
            return *this;
        }

        friend String::const_iterator operator+(difference_type offset, const String::const_iterator& iter)
            { return iter + offset; }

        friend difference_type operator-(const String::const_iterator& lhs,
                                         const String::const_iterator& rhs)
            { return lhs.d_ptr - rhs.d_ptr; }

        friend bool operator==(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.d_ptr == rhs.d_ptr; }

        friend bool operator!=(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.d_ptr != rhs.d_ptr; }

        friend bool operator<(const String::const_iterator& lhs,
                              const String::const_iterator& rhs)
            { return lhs.d_ptr < rhs.d_ptr; }

        friend bool operator>(const String::const_iterator& lhs,
                              const String::const_iterator& rhs)
            { return lhs.d_ptr > rhs.d_ptr; }

        friend bool operator<=(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.d_ptr <= rhs.d_ptr; }

        friend bool operator>=(const String::const_iterator& lhs,
                               const String::const_iterator& rhs)
            { return lhs.d_ptr >= rhs.d_ptr; }

        const utf32* d_ptr;
    };

	/*!
	\brief
		Constant reverse iterator class for String objects
	*/
#if defined(_MSC_VER) && ((_MSC_VER <= 1200) || ((_MSC_VER <= 1300) && defined(_STLPORT_VERSION)))
	typedef	std::reverse_iterator<const_iterator, const_pointer, const_reference, difference_type>	const_reverse_iterator;
#else
	typedef	std::reverse_iterator<const_iterator>	const_reverse_iterator;
#endif

	/*!
	\brief
		Reverse iterator class for String objects
	*/
#if defined(_MSC_VER) && ((_MSC_VER <= 1200) || ((_MSC_VER <= 1300) && defined(_STLPORT_VERSION)))
	typedef std::reverse_iterator<iterator, pointer, reference, difference_type>			reverse_iterator;
#else
	typedef std::reverse_iterator<iterator>			reverse_iterator;
#endif

public:
	//////////////////////////////////////////////////////////////////////////
	// Default Construction and Destructor
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Constructs an empty string
	*/
	String(void)
	{
		init();
	}

	/*!
	\brief
		Destructor for String objects
	*/
	~String(void);

	//////////////////////////////////////////////////////////////////////////
	// Construction via CEGUI::String
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Copy constructor - Creates a new string with the same value as \a str

	\param str
		String object used to initialise the newly created string

	\return
		Nothing
	*/
	String(const String& str)
	{
		init();
		assign(str);
	}


	/*!
	\brief
		Constructs a new string initialised with code points from another String object.

	\param str
		String object used to initialise the newly created string

	\param str_idx
		Starting code-point of \a str to be used when initialising the new String

	\param str_num
		Maximum number of code points from \a str that are to be assigned to the new String

	\return
		Nothing
	*/
	String(const String& str, size_type str_idx, size_type str_num = npos)
	{
		init();
		assign(str, str_idx, str_num);
	}

	//////////////////////////////////////////////////////////////////////////
	// Construction via std::string
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Constructs a new string and initialises it using the std::string std_str

	\param std_str
		The std::string object that is to be used to initialise the new String object.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(const std::string& std_str)
	{
		init();
		assign(std_str);
	}

	/*!
	\brief
		Constructs a new string initialised with characters from the given std::string object.

	\param std_str
		std::string object used to initialise the newly created string

	\param str_idx
		Starting character of \a std_str to be used when initialising the new String

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param str_num
		Maximum number of characters from \a std_str that are to be assigned to the new String

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(const std::string& std_str, size_type str_idx, size_type str_num = npos)
	{
		init();
		assign(std_str, str_idx, str_num);
	}


	//////////////////////////////////////////////////////////////////////////
	// Construction via UTF-8 stream (for straight ASCII use, only codes 0x00 - 0x7f are valid)
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Constructs a new String object and initialise it using the provided utf8 encoded string buffer.

	\param utf8_str
		Pointer to a buffer containing a null-terminated Unicode string encoded as utf8 data.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(const utf8* utf8_str)
	{
		init();
		assign(utf8_str);
	}

	/*!
	\brief
		Constructs a new String object and initialise it using the provided utf8 encoded string buffer.

		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param utf8_str
		Pointer to a buffer containing Unicode string data encoded as utf8.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param chars_len
		Length of the provided utf8 string in code units (not code-points).

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(const utf8* utf8_str, size_type chars_len)
	{
		init();
		assign(utf8_str, chars_len);
	}

	//////////////////////////////////////////////////////////////////////////
	// Construction via code-point (using a UTF-32 code unit)
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Constructs a new String that is initialised with the specified code point

	\param num
		The number of times \a code_point is to be put into new String object

	\param code_point
		The Unicode code point to be used when initialising the String object

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(size_type num, utf32 code_point)
	{
		init();
		assign(num, code_point);
	}

	//////////////////////////////////////////////////////////////////////////
	// Construction via iterator
	//////////////////////////////////////////////////////////////////////////
	// Create string with characters in the range [beg, end)
	/*!
	\brief
		Construct a new string object and initialise it with code-points from the range [beg, end).

	\param beg
		Iterator describing the start of the data to be used when initialising the String object

	\param end
		Iterator describing the (exclusive) end of the data to be used when initialising the String object

	\return
		Nothing
	*/
	String(const_iterator iter_beg, const_iterator iter_end)
	{
		init();
		append(iter_beg, iter_end);
	}


	//////////////////////////////////////////////////////////////////////////
	// Construction via c-string
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Constructs a new String object and initialise it using the provided c-string.

	\param c_str
		Pointer to a c-string.

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(const char* cstr)
	{
		init();
		assign(cstr);
	}

	/*!
	\brief
		Constructs a new String object and initialise it using characters from the provided char array.

	\param chars
		char array.

	\param chars_len
		Number of chars from the array to be used.

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	String(const char* chars, size_type chars_len)
	{
		init();
		assign(chars, chars_len);
	}


	//////////////////////////////////////////////////////////////////////////
	// Size operations
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Returns the size of the String in code points

	\return
		Number of code points currently in the String
	*/
	size_type	size(void) const
	{
		return d_cplength;
	}

	/*!
	\brief
		Returns the size of the String in code points

	\return
		Number of code points currently in the String
	*/
	size_type	length(void) const
	{
		return d_cplength;
	}

	/*!
	\brief
		Returns true if the String is empty

	\return
		true if the String is empty, else false.
	*/
	bool	empty(void) const
	{
		return	(d_cplength == 0);
	}

	/*!
	\brief
		Returns the maximum size of a String.

		Any operation that would result in a String that is larger than this value will throw the std::length_error exception.

	\return
		The maximum number of code points that a string can contain
	*/
	size_type max_size(void) const
	{
		return (((size_type)-1) / sizeof(utf32));
	}

	//////////////////////////////////////////////////////////////////////////
	// Capacity Operations
	//////////////////////////////////////////////////////////////////////////
	// return the number of code points the string could hold without re-allocation
	// (due to internal encoding this will always report the figure for worst-case encoding, and could even be < size()!)
	/*!
	\brief
		Return the number of code points that the String could hold before a re-allocation would be required.

	\return
		Size of the current reserve buffer.  This is the maximum number of code points the String could hold before a buffer
		re-allocation would be required
	*/
	size_type capacity(void) const
	{
		return d_reserve - 1;
	}

	// reserve internal memory for at-least 'num' code-points (characters).  if num is 0, request is shrink-to-fit.
	/*!
	\brief
		Specifies the amount of reserve capacity to allocate.

	\param num
		The number of code points to allocate space for.  If \a num is larger that the current reserve, then a re-allocation will occur.  If
		\a num is smaller than the current reserve (but not 0) the buffer may be shrunk to the larger of the specified number, or the current
		String size (operation is currently not implemented).  If \a num is 0, then the buffer is re-allocated to fit the current String size.

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String object would be too big.
	*/
	void	reserve(size_type num = 0)
	{
		if (num == 0)
			trim();
		else
			grow(num);
	}

	//////////////////////////////////////////////////////////////////////////
	// Comparisons
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Compares this String with the String 'str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param str
		The String object that is to compared with this String.

	\return
		-  0 if the String objects are equal
		- <0 if this String is lexicographically smaller than \a str
		- >0 if this String is lexicographically greater than \a str
	*/
	int		compare(const String& str) const
	{
		return compare(0, d_cplength, str);
	}

	/*!
	\brief
		Compares code points from this String with code points from the String 'str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param idx
		Index of the first code point from this String to consider.

	\param len
		Maximum number of code points from this String to consider.

	\param str
		The String object that is to compared with this String.

	\param str_idx
		Index of the first code point from String \a str to consider.

	\param str_len
		Maximum number of code points from String \a str to consider

	\return
		-  0 if the specified sub-strings are equal
		- <0 if specified sub-strings are lexicographically smaller than \a str
		- >0 if specified sub-strings are lexicographically greater than \a str

	\exception std::out_of_range	Thrown if either \a idx or \a str_idx are invalid.
	*/
	int		compare(size_type idx, size_type len, const String& str, size_type str_idx = 0, size_type str_len = npos) const
	{
		if ((d_cplength < idx) || (str.d_cplength < str_idx))
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if ((len == npos) || (idx + len > d_cplength))
			len = d_cplength - idx;

		if ((str_len == npos) || (str_idx + str_len > str.d_cplength))
			str_len = str.d_cplength - str_idx;

		int val = (len == 0) ? 0 : utf32_comp_utf32(&ptr()[idx], &str.ptr()[str_idx], (len < str_len) ? len : str_len);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < str_len) ? -1 : (len == str_len) ? 0 : 1;
	}


	/*!
	\brief
		Compares this String with the std::string 'std_str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param std_str
		The std::string object that is to compared with this String.

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\return
		-  0 if the string objects are equal
		- <0 if this string is lexicographically smaller than \a std_str
		- >0 if this string is lexicographically greater than \a std_str
	*/
	int		compare(const std::string& std_str) const
	{
		return compare(0, d_cplength, std_str);
	}


	/*!
	\brief
		Compares code points from this String with code points from the std::string 'std_str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param idx
		Index of the first code point from this String to consider.

	\param len
		Maximum number of code points from this String to consider.

	\param std_str
		The std::string object that is to compared with this String.

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\param str_idx
		Index of the first character from std::string \a std_str to consider.

	\param str_len
		Maximum number of characters from std::string \a std_str to consider

	\return
		-  0 if the specified sub-strings are equal
		- <0 if specified sub-strings are lexicographically smaller than \a std_str
		- >0 if specified sub-strings are lexicographically greater than \a std_str

	\exception std::out_of_range	Thrown if either \a idx or \a str_idx are invalid.
	*/
	int		compare(size_type idx, size_type len, const std::string& std_str, size_type str_idx = 0, size_type str_len = npos) const
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (std_str.size() < str_idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for std::string"));

		if ((len == npos) || (idx + len > d_cplength))
			len = d_cplength - idx;

		if ((str_len == npos) || (str_idx + str_len > std_str.size()))
			str_len = (size_type)std_str.size() - str_idx;

		int val = (len == 0) ? 0 : utf32_comp_char(&ptr()[idx], &std_str.c_str()[str_idx], (len < str_len) ? len : str_len);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < str_len) ? -1 : (len == str_len) ? 0 : 1;
	}


	/*!
	\brief
		Compares this String with the null-terminated utf8 encoded 'utf8_str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param utf8_str
		The buffer containing valid Unicode data encoded as utf8 that is to compared with this String.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		-  0 if the strings are equal
		- <0 if this string is lexicographically smaller than \a utf8_str
		- >0 if this string is lexicographically greater than \a utf8_str
	*/
	int		compare(const utf8* utf8_str) const
	{
		return compare(0, d_cplength, utf8_str, encoded_size(utf8_str));
	}


	/*!
	\brief
		Compares code points from this String with the null-terminated utf8 encoded 'utf8_str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param idx
		Index of the first code point from this String to consider.

	\param len
		Maximum number of code points from this String to consider.

	\param utf8_str
		The buffer containing valid Unicode data encoded as utf8 that is to compared with this String.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		-  0 if the specified sub-strings are equal
		- <0 if specified sub-strings are lexicographically smaller than \a utf8_str
		- >0 if specified sub-strings are lexicographically greater than \a utf8_str

	\exception std::out_of_range	Thrown if \a idx is invalid.
	*/
	int		compare(size_type idx, size_type len, const utf8* utf8_str) const
	{
		return compare(idx, len, utf8_str, encoded_size(utf8_str));
	}

	/*!
	\brief
		Compares code points from this String with the utf8 encoded data in buffer 'utf8_str'.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param idx
		Index of the first code point from this String to consider.

	\param len
		Maximum number of code points from this String to consider.

	\param utf8_str
		The buffer containing valid Unicode data encoded as utf8 that is to compared with this String.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param str_cplen
		The number of encoded code points in the buffer \a utf8_str (this is not the same as the number of code units).

	\return
		-  0 if the specified sub-strings are equal
		- <0 if specified sub-strings are lexicographically smaller than \a utf8_str
		- >0 if specified sub-strings are lexicographically greater than \a utf8_str

	\exception std::out_of_range	Thrown if \a idx is invalid.
	\exception std::length_error	Thrown if \a str_cplen is set to npos.
	*/
	int		compare(size_type idx, size_type len, const utf8* utf8_str, size_type str_cplen) const
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (str_cplen == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		if ((len == npos) || (idx + len > d_cplength))
			len = d_cplength - idx;

		int val = (len == 0) ? 0 : utf32_comp_utf8(&ptr()[idx], utf8_str, (len < str_cplen) ? len : str_cplen);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < str_cplen) ? -1 : (len == str_cplen) ? 0 : 1;
	}


	/*!
	\brief
		Compares this String with the given c-string.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param c_str
		The c-string that is to compared with this String.

	\return
		-  0 if the strings are equal
		- <0 if this string is lexicographically smaller than \a c_str
		- >0 if this string is lexicographically greater than \a c_str
	*/
	int		compare(const char* cstr) const
	{
		return compare(0, d_cplength, cstr, strlen(cstr));
	}


	/*!
	\brief
		Compares code points from this String with the given c-string.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param idx
		Index of the first code point from this String to consider.

	\param len
		Maximum number of code points from this String to consider.

	\param c_str
		The c-string that is to compared with this String.

	\return
		-  0 if the specified sub-strings are equal
		- <0 if specified sub-strings are lexicographically smaller than \a c_str
		- >0 if specified sub-strings are lexicographically greater than \a c_str

	\exception std::out_of_range	Thrown if \a idx is invalid.
	*/
	int		compare(size_type idx, size_type len, const char* cstr) const
	{
		return compare(idx, len, cstr, strlen(cstr));
	}


	/*!
	\brief
		Compares code points from this String with chars in the given char array.

	\note
		This does currently not properly consider Unicode and / or the system locale.

	\param idx
		Index of the first code point from this String to consider.

	\param len
		Maximum number of code points from this String to consider.

	\param chars
		The array containing the chars that are to compared with this String.

	\param chars_len
		The number of chars in the array.

	\return
		-  0 if the specified sub-strings are equal
		- <0 if specified sub-strings are lexicographically smaller than \a chars
		- >0 if specified sub-strings are lexicographically greater than \a chars

	\exception std::out_of_range	Thrown if \a idx is invalid.
	\exception std::length_error	Thrown if \a chars_len is set to npos.
	*/
	int		compare(size_type idx, size_type len, const char* chars, size_type chars_len) const
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if ((len == npos) || (idx + len > d_cplength))
			len = d_cplength - idx;

		int val = (len == 0) ? 0 : utf32_comp_char(&ptr()[idx], chars, (len < chars_len) ? len : chars_len);

		return (val != 0) ? ((val < 0) ? -1 : 1) : (len < chars_len) ? -1 : (len == chars_len) ? 0 : 1;
	}


	//////////////////////////////////////////////////////////////////////////
	// Character access
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Returns the code point at the given index.

	\param idx
		Zero based index of the code point to be returned.

	\note
		- For constant strings length()/size() provide a valid index and will access the default utf32 value.
		- For non-constant strings length()/size() is an invalid index, and acceesing (especially writing) this index could cause string corruption.

	\return
		The utf32 code point at the given index within the String.
	*/
	reference	operator[](size_type idx)
	{
		return (ptr()[idx]);
	}

	/*!
	\brief
		Returns the code point at the given index.

	\param idx
		Zero based index of the code point to be returned.

	\note
		- For constant strings length()/size() provide a valid index and will access the default utf32 value.
		- For non-constant strings length()/size() is an invalid index, and acceesing (especially writing) this index could cause string corruption.

	\return
		The utf32 code point at the given index within the String.
	*/
	value_type	operator[](size_type idx) const
	{
		return ptr()[idx];
	}

	/*!
	\brief
		Returns the code point at the given index.

	\param idx
		Zero based index of the code point to be returned.

	\return
		The utf32 code point at the given index within the String.

	\exception std::out_of_range	Thrown if \a idx is >= length().
	*/
	reference	at(size_type idx)
	{
		if (d_cplength <= idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		return ptr()[idx];
	}

	/*!
	\brief
		Returns the code point at the given index.

	\param idx
		Zero based index of the code point to be returned.

	\return
		The utf32 code point at the given index within the String.

	\exception std::out_of_range	Thrown if \a idx is >= length().
	*/
	const_reference	at(size_type idx) const
	{
		if (d_cplength <= idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		return ptr()[idx];
	}


	//////////////////////////////////////////////////////////////////////////
	// C-Strings and arrays
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Returns contents of the String as a null terminated string of utf8 encoded data.

	\return
		Pointer to a char buffer containing the contents of the String encoded as null-terminated utf8 data.

	\note
		The buffer returned from this function is owned by the String object.

	\note
		Any function that modifies the String data will invalidate the buffer returned by this call.
	*/
	const char* c_str(void) const
	{
		return (const char*)build_utf8_buff();
	}

	/*!
	\brief
		Returns contents of the String as utf8 encoded data.

	\return
		Pointer to a buffer containing the contents of the String encoded utf8 data.

	\note
		The buffer returned from this function is owned by the String object.

	\note
		Any function that modifies the String data will invalidate the buffer returned by this call.
	*/
	const utf8* data(void) const
	{
		return build_utf8_buff();
	}

    /*!
    \brief
        Returns a pointer to the buffer in use.
    */
	utf32*	ptr(void)
	{
		return (d_reserve > CEGUI_STR_QUICKBUFF_SIZE) ? d_buffer : d_quickbuff;
	}

	/*!
    \brief
        Returns a pointer to the buffer in use. (const version)
    */
	const utf32*	ptr(void) const
	{
		return (d_reserve > CEGUI_STR_QUICKBUFF_SIZE) ? d_buffer : d_quickbuff;
	}

	// copy, at most, 'len' code-points of the string, begining with code-point 'idx', into the array 'buf' as valid utf8 encoded data
	// return number of utf8 code units placed into the buffer
	/*!
	\brief
		Copies an area of the String into the provided buffer as encoded utf8 data.

	\param buf
		Pointer to a buffer that is to receive the encoded data (this must be big enough to hold the encoded data)

	\param len
		Maximum number of code points from the String that should be encoded into the buffer

	\param idx
		Index of the first code point to be encoded into the buffer

	\return
		The number of utf8 encoded code units transferred to the buffer.

		\note A code unit does not equal a code point.  A utf32 code point, when encoded as utf8, can occupy between 1 and 4 code units.

	\exception std::out_of_range	Thrown if \a idx was invalid for this String.
	*/
	size_type	copy(utf8* buf, size_type len = npos, size_type idx = 0) const
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (len == npos)
			len = d_cplength;

		return encode(&ptr()[idx], buf, npos, len);
	}

	//////////////////////////////////////////////////////////////////////////
	// UTF8 Encoding length information
	//////////////////////////////////////////////////////////////////////////
	// return the number of bytes required to hold 'num' code-points, starting at code-point 'idx', of the the string when encoded as utf8 data.
	/*!
	\brief
		Return the number of utf8 code units required to hold an area of the String when encoded as utf8 data

	\param num
		Maximum number of code points to consider when calculating utf8 encoded size.

	\param idx
		Index of the first code point to consider when calculating the utf8 encoded size

	\return
		The number of utf8 code units (bytes) required to hold the specified sub-string when encoded as utf8 data.

	\exception std::out_of_range	Thrown if \a idx was invalid for this String.
	*/
	size_type	utf8_stream_len(size_type num = npos, size_type idx = 0) const
	{
		using namespace std;

		if (d_cplength < idx)
			CEGUI_THROW(out_of_range("Index was out of range for CEGUI::String object"));

		size_type	maxlen = d_cplength - idx;

		return encoded_size(&ptr()[idx], ceguimin(num, maxlen));
	}

	//////////////////////////////////////////////////////////////////////////
	// Assignment Functions
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Assign the value of String \a str to this String

	\param str
		String object containing the string value to be assigned.

	\return
		This String after the assignment has happened
	*/
	String&	operator=(const String& str)
	{
		return assign(str);
	}

	/*!
	\brief
		Assign a sub-string of String \a str to this String

	\param str
		String object containing the string data to be assigned.

	\param str_idx
		Index of the first code point in \a str that is to be assigned

	\param str_num
		Maximum number of code points from \a str that are be be assigned

	\return
		This String after the assignment has happened

	\exception std::out_of_range	Thrown if str_idx is invalid for \a str
	*/
	String&	assign(const String& str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (str.d_cplength < str_idx)
			CEGUI_THROW(std::out_of_range("Index was out of range for CEGUI::String object"));

		if ((str_num == npos) || (str_num > str.d_cplength - str_idx))
			str_num = str.d_cplength - str_idx;

		grow(str_num);
		setlen(str_num);
		memcpy(ptr(), &str.ptr()[str_idx], str_num * sizeof(utf32));

		return *this;
	}

	/*!
	\brief
		Assign the value of std::string \a std_str to this String

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param std_str
		std::string object containing the string value to be assigned.

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	operator=(const std::string& std_str)
	{
		return assign(std_str);
	}

	/*!
	\brief
		Assign a sub-string of std::string \a std_str to this String

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param std_str
		std::string object containing the string value to be assigned.

	\param str_idx
		Index of the first character of \a std_str to be assigned

	\param str_num
		Maximum number of characters from \a std_str to be assigned

	\return
		This String after the assignment has happened

	\exception std::out_of_range	Thrown if \a str_idx is invalid for \a std_str
	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	assign(const std::string& std_str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (std_str.size() < str_idx)
			CEGUI_THROW(std::out_of_range("Index was out of range for std::string object"));

		if ((str_num == npos) || (str_num > (size_type)std_str.size() - str_idx))
			str_num = (size_type)std_str.size() - str_idx;

		grow(str_num);
		setlen(str_num);

		while(str_num--)
		{
			((*this)[str_num]) = static_cast<utf32>(static_cast<unsigned char>(std_str[str_num + str_idx]));
		}

		return *this;
	}

	/*!
	\brief
		Assign to this String the string value represented by the given null-terminated utf8 encoded data

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param utf8_str
		Buffer containing valid null-terminated utf8 encoded data

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	operator=(const utf8* utf8_str)
	{
		return assign(utf8_str, utf_length(utf8_str));
	}

	/*!
	\brief
		Assign to this String the string value represented by the given null-terminated utf8 encoded data

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param utf8_str
		Buffer containing valid null-terminated utf8 encoded data

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	assign(const utf8* utf8_str)
	{
		return assign(utf8_str, utf_length(utf8_str));
	}

	/*!
	\brief
		Assign to this String the string value represented by the given utf8 encoded data

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param utf8_str
		Buffer containing valid utf8 encoded data

	\param str_num
		Number of code units (not code points) in the buffer pointed to by \a utf8_str

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large, or if str_num is 'npos'.
	*/
	String&	assign(const utf8* utf8_str, size_type str_num)
	{
		if (str_num == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		size_type enc_sze = encoded_size(utf8_str, str_num);

		grow(enc_sze);
		encode(utf8_str, ptr(), d_reserve, str_num);
		setlen(enc_sze);
		return *this;
	}

	/*!
	\brief
		Assigns the specified utf32 code point to this String.  Result is always a String 1 code point in length.

	\param code_point
		Valid utf32 Unicode code point to be assigned to the string

	\return
		This String after assignment
	*/
	String&	operator=(utf32 code_point)
	{
		return assign(1, code_point);
	}

	/*!
	\brief
		Assigns the specified code point repeatedly to the String

	\param num
		The number of times to assign the code point

	\param code_point
		Valid utf32 Unicode code point to be assigned to the string

	\return
		This String after assignment.

	\exception std::length_error	Thrown if \a num was 'npos'
	*/
	String&	assign(size_type num, utf32 code_point)
	{
		if (num == npos)
			CEGUI_THROW(std::length_error("Code point count can not be 'npos'"));

		grow(num);
		setlen(num);
		utf32* p = ptr();

		while(num--)
			*p++ = code_point;

		return *this;
	}


	/*!
	\brief
		Assign to this String the given C-string.

	\param c_str
		Pointer to a valid C style string.

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	operator=(const char* cstr)
	{
		return assign(cstr, strlen(cstr));
	}


	/*!
	\brief
		Assign to this String the given C-string.

	\param c_str
		Pointer to a valid C style string.

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	assign(const char* cstr)
	{
		return assign(cstr, strlen(cstr));
	}


	/*!
	\brief
		Assign to this String a number of chars from a char array.

	\param chars
		char array.

	\param chars_len
		Number of chars to be assigned.

	\return
		This String after the assignment has happened

	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String&	assign(const char* chars, size_type chars_len)
	{
		grow(chars_len);
		utf32* pt = ptr();

		for (size_type i = 0; i < chars_len; ++i)
		{
			*pt++ = static_cast<utf32>(static_cast<unsigned char>(*chars++));
		}

		setlen(chars_len);
		return *this;
	}


	/*!
	\brief
		Swaps the value of this String with the given String \a str

	\param str
		String object whos value is to be swapped with this String.

	\return
		Nothing
	*/
	void	swap(String& str)
	{
		size_type	temp_len	= d_cplength;
		d_cplength = str.d_cplength;
		str.d_cplength = temp_len;

		size_type	temp_res	= d_reserve;
		d_reserve = str.d_reserve;
		str.d_reserve = temp_res;

		utf32*		temp_buf	= d_buffer;
		d_buffer = str.d_buffer;
		str.d_buffer = temp_buf;

		// see if we need to swap 'quick buffer' data
		if (temp_res <= CEGUI_STR_QUICKBUFF_SIZE)
		{
			utf32		temp_qbf[CEGUI_STR_QUICKBUFF_SIZE];

			memcpy(temp_qbf, d_quickbuff, CEGUI_STR_QUICKBUFF_SIZE * sizeof(utf32));
			memcpy(d_quickbuff, str.d_quickbuff, CEGUI_STR_QUICKBUFF_SIZE * sizeof(utf32));
			memcpy(str.d_quickbuff, temp_qbf, CEGUI_STR_QUICKBUFF_SIZE * sizeof(utf32));
		}

	}

	//////////////////////////////////////////////////////////////////////////
	// Appending Functions
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Appends the String \a str

	\param str
		String object that is to be appended

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	operator+=(const String& str)
	{
		return append(str);
	}

	/*!
	\brief
		Appends a sub-string of the String \a str

	\param str
		String object containing data to be appended

	\param str_idx
		Index of the first code point to be appended

	\param str_num
		Maximum number of code points to be appended

	\return
		This String after the append operation

	\exception std::out_of_range	Thrown if \a str_idx is invalid for \a str.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String& append(const String& str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (str.d_cplength < str_idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if ((str_num == npos) || (str_num > str.d_cplength - str_idx))
			str_num = str.d_cplength - str_idx;

		grow(d_cplength + str_num);
		memcpy(&ptr()[d_cplength], &str.ptr()[str_idx], str_num * sizeof(utf32));
		setlen(d_cplength + str_num);
		return *this;
	}


	/*!
	\brief
		Appends the std::string \a std_str

	\param std_str
		std::string object that is to be appended

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	operator+=(const std::string& std_str)
	{
		return append(std_str);
	}

	/*!
	\brief
		Appends a sub-string of the std::string \a std_str

	\param std_str
		std::string object containing data to be appended

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param str_idx
		Index of the first character to be appended

	\param str_num
		Maximum number of characters to be appended

	\return
		This String after the append operation

	\exception std::out_of_range	Thrown if \a str_idx is invalid for \a std_str.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String& append(const std::string& std_str, size_type str_idx = 0, size_type str_num = npos)
	{
		if (std_str.size() < str_idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for std::string"));

		if ((str_num == npos) || (str_num > (size_type)std_str.size() - str_idx))
			str_num = (size_type)std_str.size() - str_idx;

		size_type newsze = d_cplength + str_num;

		grow(newsze);
		utf32* pt = &ptr()[newsze-1];

		while(str_num--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(std_str[str_num]));

		setlen(newsze);
		return *this;
	}


	/*!
	\brief
		Appends to the String the null-terminated utf8 encoded data in the buffer utf8_str.

	\param utf8_str
		buffer holding the null-terminated utf8 encoded data that is to be appended

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	operator+=(const utf8* utf8_str)
	{
		return append(utf8_str, utf_length(utf8_str));
	}

	/*!
	\brief
		Appends to the String the null-terminated utf8 encoded data in the buffer utf8_str.

	\param utf8_str
		Buffer holding the null-terminated utf8 encoded data that is to be appended

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String& append(const utf8* utf8_str)
	{
		return append(utf8_str, utf_length(utf8_str));
	}


	/*!
	\brief
		Appends to the String the utf8 encoded data in the buffer utf8_str.

	\param utf8_str
		Buffer holding the utf8 encoded data that is to be appended

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param len
		Number of code units (not code points) in the buffer to be appended

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large, or if \a len was 'npos'
	*/
	String& append(const utf8* utf8_str, size_type len)
	{
		if (len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		size_type encsz = encoded_size(utf8_str, len);
		size_type newsz = d_cplength + encsz;

		grow(newsz);
		encode(utf8_str, &ptr()[d_cplength], encsz, len);
		setlen(newsz);

		return *this;
	}


	/*!
	\brief
		Appends a single code point to the string

	\param code_point
		utf32 Unicode code point that is to be appended

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too long.
	*/
	String& operator+=(utf32 code_point)
	{
		return append(1, code_point);
	}

	/*!
	\brief
		Appends a single code point multiple times to the string

	\param num
		Number of copies of the code point to be appended

	\param code_point
		utf32 Unicode code point that is to be appended

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too long, or if \a num was 'npos'.
	*/
	String& append(size_type num, utf32 code_point)
	{
		if (num == npos)
			CEGUI_THROW(std::length_error("Code point count can not be 'npos'"));

		size_type newsz = d_cplength + num;
		grow(newsz);

		utf32* p = &ptr()[d_cplength];

		while(num--)
			*p++ = code_point;

		setlen(newsz);

		return *this;
	}

	/*!
	\brief
		Appends a single code point to the string

	\param code_point
		utf32 Unicode code point that is to be appended

	\return
		Nothing

	\exception std::length_error	Thrown if resulting String would be too long.
	*/
	void	push_back(utf32 code_point)
	{
		append(1, code_point);
	}

	/*!
	\brief
		Appends the code points in the reange [beg, end)

	\param beg
		Iterator describing the start of the range to be appended

	\param end
		Iterator describing the (exclusive) end of the range to be appended.

	\return
		This String after the append operation

	\exception std::length_error	Thrown if the resulting string would be too large.
	*/
	String&	append(const_iterator iter_beg, const_iterator iter_end)
	{
		return replace(end(), end(), iter_beg, iter_end);
	}


	/*!
	\brief
		Appends to the String the given c-string.

	\param c_str
		c-string that is to be appended.

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	operator+=(const char* cstr)
	{
		return append(cstr, strlen(cstr));
	}


	/*!
	\brief
		Appends to the String the given c-string.

	\param c_str
		c-string that is to be appended.

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String& append(const char* cstr)
	{
		return append(cstr, strlen(cstr));
	}


	/*!
	\brief
		Appends to the String chars from the given char array.

	\param chars
		char array holding the chars that are to be appended

	\param chars_len
		Number of chars to be appended

	\return
		This String after the append operation

	\exception std::length_error	Thrown if resulting String would be too large, or if \a chars_len was 'npos'
	*/
	String& append(const char* chars, size_type chars_len)
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		size_type newsz = d_cplength + chars_len;

		grow(newsz);

		utf32* pt = &ptr()[newsz-1];

		while(chars_len--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(chars[chars_len]));

		setlen(newsz);

		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// Insertion Functions
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Inserts the given String object at the specified position.

	\param idx
		Index where the string is to be inserted.

	\param str
		String object that is to be inserted.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	insert(size_type idx, const String& str)
	{
		return insert(idx, str, 0, npos);
	}

	/*!
	\brief
		Inserts a sub-string of the given String object at the specified position.

	\param idx
		Index where the string is to be inserted.

	\param str
		String object containing data to be inserted.

	\param str_idx
		Index of the first code point from \a str to be inserted.

	\param str_num
		Maximum number of code points from \a str to be inserted.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx or \a str_idx are out of range.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String& insert(size_type idx, const String& str, size_type str_idx, size_type str_num)
	{
		if ((d_cplength < idx) || (str.d_cplength < str_idx))
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if ((str_num == npos) || (str_num > str.d_cplength - str_idx))
			str_num = str.d_cplength - str_idx;

		size_type newsz = d_cplength + str_num;
		grow(newsz);
		memmove(&ptr()[idx + str_num], &ptr()[idx], (d_cplength - idx) * sizeof(utf32));
		memcpy(&ptr()[idx], &str.ptr()[str_idx], str_num * sizeof(utf32));
		setlen(newsz);

		return *this;
	}

	/*!
	\brief
		Inserts the given std::string object at the specified position.

	\param idx
		Index where the std::string is to be inserted.

	\param std_str
		std::string object that is to be inserted.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	insert(size_type idx, const std::string& std_str)
	{
		return insert(idx, std_str, 0, npos);
	}

	/*!
	\brief
		Inserts a sub-string of the given std::string object at the specified position.

	\param idx
		Index where the string is to be inserted.

	\param std_str
		std::string object containing data to be inserted.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param str_idx
		Index of the first character from \a std_str to be inserted.

	\param str_num
		Maximum number of characters from \a str to be inserted.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx or \a str_idx are out of range.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String& insert(size_type idx, const std::string& std_str, size_type str_idx, size_type str_num)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (std_str.size() < str_idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for std::string"));

		if ((str_num == npos) || (str_num > (size_type)std_str.size() - str_idx))
			str_num = (size_type)std_str.size() - str_idx;

		size_type newsz = d_cplength + str_num;
		grow(newsz);

		memmove(&ptr()[idx + str_num], &ptr()[idx], (d_cplength - idx) * sizeof(utf32));

		utf32* pt = &ptr()[idx + str_num - 1];

		while(str_num--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(std_str[str_idx + str_num]));

		setlen(newsz);

		return *this;
	}

	/*!
	\brief
		Inserts the given null-terminated utf8 encoded data at the specified position.

	\param idx
		Index where the data is to be inserted.

	\param utf8_str
		Buffer containing the null-terminated utf8 encoded data that is to be inserted.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	insert(size_type idx, const utf8* utf8_str)
	{
		return insert(idx, utf8_str, utf_length(utf8_str));
	}

	/*!
	\brief
		Inserts the given utf8 encoded data at the specified position.

	\param idx
		Index where the data is to be inserted.

	\param utf8_str
		Buffer containing the utf8 encoded data that is to be inserted.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param len
		Length of the data to be inserted in uf8 code units (not code points)

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large, or if \a len is 'npos'
	*/
	String& insert(size_type idx, const utf8* utf8_str, size_type len)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (len == npos)
			CEGUI_THROW(std::length_error("Length of utf8 encoded string can not be 'npos'"));

		size_type encsz = encoded_size(utf8_str, len);
		size_type newsz = d_cplength + encsz;

		grow(newsz);
		memmove(&ptr()[idx + encsz], &ptr()[idx], (d_cplength - idx) * sizeof(utf32));
		encode(utf8_str, &ptr()[idx], encsz, len);
		setlen(newsz);

		return *this;
	}

	/*!
	\brief
		Inserts a code point multiple times into the String

	\param idx
		Index where the code point(s) are to be inserted

	\param num
		The number of times to insert the code point

	\param code_point
		The utf32 code point that is to be inserted

	\return
		This String after the insertion.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large, or if \a num is 'npos'
	*/
	String& insert(size_type idx, size_type num, utf32 code_point)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (num == npos)
			CEGUI_THROW(std::length_error("Code point count can not be 'npos'"));

		size_type newsz = d_cplength + num;
		grow(newsz);

		memmove(&ptr()[idx + num], &ptr()[idx], (d_cplength - idx) * sizeof(utf32));

		utf32* pt = &ptr()[idx + num - 1];

		while(num--)
			*pt-- = code_point;

		setlen(newsz);

		return *this;
	}

	/*!
	\brief
		Inserts a code point multiple times into the String

	\param pos
		Iterator describing the position where the code point(s) are to be inserted

	\param num
		The number of times to insert the code point

	\param code_point
		The utf32 code point that is to be inserted

	\return
		This String after the insertion.

	\exception std::length_error	Thrown if resulting String would be too large, or if \a num is 'npos'
	*/
	void insert(iterator pos, size_type num, utf32 code_point)
	{
		insert(safe_iter_dif(pos, begin()), num, code_point);
	}

	/*!
	\brief
		Inserts a single code point into the String

	\param pos
		Iterator describing the position where the code point is to be inserted

	\param code_point
		The utf32 code point that is to be inserted

	\return
		This String after the insertion.

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	iterator insert(iterator pos, utf32 code_point)
	{
		insert(pos, 1, code_point);
		return pos;
	}

	/*!
	\brief
		Inserts code points specified by the range [beg, end).

	\param pos
		Iterator describing the position where the data is to be inserted

	\param beg
		Iterator describing the begining of the range to be inserted

	\param end
		Iterator describing the (exclusive) end of the range to be inserted.

	\return
		Nothing.

	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	void	insert(iterator iter_pos, const_iterator iter_beg, const_iterator iter_end)
	{
		replace(iter_pos, iter_pos, iter_beg, iter_end);
	}


	/*!
	\brief
		Inserts the given c-string at the specified position.

	\param idx
		Index where the c-string is to be inserted.

	\param c_str
		c-string that is to be inserted.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large.
	*/
	String&	insert(size_type idx, const char* cstr)
	{
		return insert(idx, cstr, strlen(cstr));
	}


	/*!
	\brief
		Inserts chars from the given char array at the specified position.

	\param idx
		Index where the data is to be inserted.

	\param chars
		char array containing the chars that are to be inserted.

	\param chars_len
		Length of the char array to be inserted.

	\return
		This String after the insert.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	\exception std::length_error	Thrown if resulting String would be too large, or if \a chars_len is 'npos'
	*/
	String& insert(size_type idx, const char* chars, size_type chars_len)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length of char array can not be 'npos'"));

		size_type newsz = d_cplength + chars_len;

		grow(newsz);
		memmove(&ptr()[idx + chars_len], &ptr()[idx], (d_cplength - idx) * sizeof(utf32));

		utf32* pt = &ptr()[idx + chars_len - 1];

		while(chars_len--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(chars[chars_len]));

		setlen(newsz);

		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// Erasing characters
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Removes all data from the String

	\return
		Nothing
	*/
	void	clear(void)
	{
		setlen(0);
		trim();
	}

	/*!
	\brief
		Removes all data from the String

	\return
		The empty String (*this)
	*/
	String& erase(void)
	{
		clear();
		return *this;
	}

	/*!
	\brief
		Erase a single code point from the string

	\param idx
		The index of the code point to be removed.

	\return
		This String after the erase operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	String&	erase(size_type idx)
	{
		return erase(idx, 1);
	}

	/*!
	\brief
		Erase a range of code points

	\param idx
		Index of the first code point to be removed.

	\param len
		Maximum number of code points to be removed.

	\return
		This String after the erase operation.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	String& erase(size_type idx, size_type len)
	{
        // cover the no-op case.
        if (len == 0)
            return *this;

		if (d_cplength <= idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (len == npos)
			len = d_cplength - idx;

		size_type newsz = d_cplength - len;

		memmove(&ptr()[idx], &ptr()[idx + len], (d_cplength - idx - len) * sizeof(utf32));
		setlen(newsz);
		return	*this;
	}

	/*!
	\brief
		Erase the code point described by the given iterator

	\param pos
		Iterator describing the code point to be erased

	\return
		This String after the erase operation.
	*/
	String& erase(iterator pos)
	{
		return erase(safe_iter_dif(pos, begin()), 1);
	}

	/*!
	\brief
		Erase a range of code points described by the iterators [beg, end).

	\param beg
		Iterator describing the postion of the beginning of the range to erase

	\param end
		Iterator describing the postion of the (exclusive) end of the range to erase

	\return
		This String after the erase operation.
	*/
	String& erase(iterator iter_beg, iterator iter_end)
	{
		return erase(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg));
	}

	//////////////////////////////////////////////////////////////////////////
	// Resizing
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Resizes the String either by inserting default utf32 code points to make it larger, or by truncating to make it smaller

	\param num
		The length, in code points, that the String is to be made.

	\return
		Nothing.

	\exception std::length_error	Thrown if the String would be too large.
	*/
	void	resize(size_type num)
	{
		resize(num, utf32());
	}

	/*!
	\brief
		Resizes the String either by inserting the given utf32 code point to make it larger, or by truncating to make it smaller

	\param num
		The length, in code points, that the String is to be made.

	\param code_point
		The utf32 code point that should be used when majing the String larger

	\return
		Nothing.

	\exception std::length_error	Thrown if the String would be too large.
	*/
	void	resize(size_type num, utf32 code_point)
	{
		if (num < d_cplength)
		{
			setlen(num);
		}
		else
		{
			append(num - d_cplength, code_point);
		}

	}

	//////////////////////////////////////////////////////////////////////////
	// Replacing Characters
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Replace code points in the String with the specified String object

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced (if this is 0, operation is an insert at position \a idx)

	\param str
		The String object that is to replace the specified code points

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(size_type idx, size_type len, const String& str)
	{
		return replace(idx, len, str, 0, npos);
	}

	/*!
	\brief
		Replace the code points in the range [beg, end) with the specified String object

	\note
		If \a beg == \a end, the operation is a insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param str
		The String object that is to replace the specified range of code points

	\return
		This String after the replace operation

	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const String& str)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), str, 0, npos);
	}

	/*!
	\brief
		Replace code points in the String with a specified sub-string of a given String object.

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced.  If this is 0, the operation is an insert at position \a idx.

	\param str
		String object containing the data that will replace the specified range of code points

	\param str_idx
		Index of the first code point of \a str that is to replace the specified code point range

	\param str_num
		Maximum number of code points of \a str that are to replace the specified code point range

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if either \a idx, or \a str_idx are invalid
	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String& replace(size_type idx, size_type len, const String& str, size_type str_idx, size_type str_num)
	{
		if ((d_cplength < idx) || (str.d_cplength < str_idx))
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (((str_idx + str_num) > str.d_cplength) || (str_num == npos))
			str_num = str.d_cplength - str_idx;

		if (((len + idx) > d_cplength) || (len == npos))
			len = d_cplength - idx;

		size_type newsz = d_cplength + str_num - len;

		grow(newsz);

		if ((idx + len) < d_cplength)
			memmove(&ptr()[idx + str_num], &ptr()[len + idx], (d_cplength - idx - len) * sizeof(utf32));

		memcpy(&ptr()[idx], &str.ptr()[str_idx], str_num * sizeof(utf32));
		setlen(newsz);

		return *this;
	}


	/*!
	\brief
		Replace code points in the String with the specified std::string object

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced (if this is 0, operation is an insert at position \a idx)

	\param std_str
		The std::string object that is to replace the specified code points

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(size_type idx, size_type len, const std::string& std_str)
	{
		return replace(idx, len, std_str, 0, npos);
	}

	/*!
	\brief
		Replace the code points in the range [beg, end) with the specified std::string object

	\note
		If \a beg == \a end, the operation is a insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param std_str
		The std::string object that is to replace the specified range of code points

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\return
		This String after the replace operation

	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const std::string& std_str)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), std_str, 0, npos);
	}

	/*!
	\brief
		Replace code points in the String with a specified sub-string of a given std::string object.

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced.  If this is 0, the operation is an insert at position \a idx.

	\param std_str
		std::string object containing the data that will replace the specified range of code points

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\param str_idx
		Index of the first code point of \a std_str that is to replace the specified code point range

	\param str_num
		Maximum number of code points of \a std_str that are to replace the specified code point range

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if either \a idx, or \a str_idx are invalid
	\exception std::length_error	Thrown if the resulting String would have been too large.
	*/
	String& replace(size_type idx, size_type len, const std::string& std_str, size_type str_idx, size_type str_num)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (std_str.size() < str_idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for std::string"));

		if (((str_idx + str_num) > std_str.size()) || (str_num == npos))
			str_num = (size_type)std_str.size() - str_idx;

		if (((len + idx) > d_cplength) || (len == npos))
			len = d_cplength - idx;

		size_type newsz = d_cplength + str_num - len;

		grow(newsz);

		if ((idx + len) < d_cplength)
			memmove(&ptr()[idx + str_num], &ptr()[len + idx], (d_cplength - idx - len) * sizeof(utf32));

		utf32* pt = &ptr()[idx + str_num - 1];

		while (str_num--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(std_str[str_idx + str_num]));

		setlen(newsz);

		return *this;
	}


	/*!
	\brief
		Replace code points in the String with the specified null-terminated utf8 encoded data.

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced (if this is 0, operation is an insert at position \a idx)

	\param utf8_str
		Buffer containing the null-terminated utf8 encoded data that is to replace the specified code points

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(size_type idx, size_type len, const utf8* utf8_str)
	{
		return replace(idx, len, utf8_str, utf_length(utf8_str));
	}

	/*!
	\brief
		Replace the code points in the range [beg, end) with the specified null-terminated utf8 encoded data.

	\note
		If \a beg == \a end, the operation is a insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param utf8_str
		Buffer containing the null-terminated utf8 encoded data that is to replace the specified range of code points

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\return
		This String after the replace operation

	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const utf8* utf8_str)
	{
		return replace(iter_beg, iter_end, utf8_str, utf_length(utf8_str));
	}

	/*!
	\brief
		Replace code points in the String with the specified utf8 encoded data.

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced (if this is 0, operation is an insert at position \a idx)

	\param utf8_str
		Buffer containing the null-terminated utf8 encoded data that is to replace the specified code points

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param str_len
		Length of the utf8 encoded data in utf8 code units (not code points).

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if the resulting String would be too large, or if \a str_len was 'npos'.
	*/
	String& replace(size_type idx, size_type len, const utf8* utf8_str, size_type str_len)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		if (((len + idx) > d_cplength) || (len == npos))
			len = d_cplength - idx;

		size_type encsz = encoded_size(utf8_str, str_len);
		size_type newsz = d_cplength + encsz - len;

		grow(newsz);

		if ((idx + len) < d_cplength)
			memmove(&ptr()[idx + encsz], &ptr()[len + idx], (d_cplength - idx - len) * sizeof(utf32));

		encode(utf8_str, &ptr()[idx], encsz, str_len);

		setlen(newsz);
		return *this;
	}

	/*!
	\brief
		Replace the code points in the range [beg, end) with the specified null-terminated utf8 encoded data.

	\note
		If \a beg == \a end, the operation is a insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param utf8_str
		Buffer containing the null-terminated utf8 encoded data that is to replace the specified range of code points

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param str_len
		Length of the utf8 encoded data in utf8 code units (not code points).

	\return
		This String after the replace operation

		\exception std::length_error	Thrown if the resulting String would be too large, or if \a str_len was 'npos'.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const utf8* utf8_str, size_type str_len)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), utf8_str, str_len);
	}

	/*!
	\brief
		Replaces a specified range of code points with occurrences of a given code point

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to replace.  If this is 0 the operation is an insert

	\param num
		Number of occurrences of \a code_point that are to replace the specified range of code points

	\param code_point
		Code point that is to be used when replacing the specified range of code points

	\return
		This String after the replace operation.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if resulting String would have been too long, or if \a num was 'npos'.
	*/
	String& replace(size_type idx, size_type len, size_type num, utf32 code_point)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (num == npos)
			CEGUI_THROW(std::length_error("Code point count can not be 'npos'"));

		if (((len + idx) > d_cplength) || (len == npos))
			len = d_cplength - idx;

		size_type newsz = d_cplength + num - len;

		grow(newsz);

		if ((idx + len) < d_cplength)
			memmove(&ptr()[idx + num], &ptr()[len + idx], (d_cplength - idx - len) * sizeof(utf32));

		utf32* pt = &ptr()[idx + num - 1];

		while (num--)
			*pt-- = code_point;

		setlen(newsz);

		return *this;
	}

	/*!
	\brief
		Replace the code points in the range [beg, end) with occurrences of a given code point

	\note
		If \a beg == \a end, the operation is an insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param num
		Number of occurrences of \a code_point that are to replace the specified range of code points

	\param code_point
		Code point that is to be used when replacing the specified range of code points

	\return
		This String after the replace operation

	\exception std::length_error	Thrown if resulting String would have been too long, or if \a num was 'npos'.
	*/
	String& replace(iterator iter_beg, iterator iter_end, size_type num, utf32 code_point)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), num, code_point);
	}


	/*!
	\brief
		Replace the code points in the range [beg, end) with code points from the range [newBeg, newEnd).

	\note
		If \a beg == \a end, the operation is an insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param newBeg
		Iterator describing the beginning of the range to insert.

	\param newEnd
		Iterator describing the (exclusive) end of the range to insert.

	\return
		This String after the insert operation.

	\exception std::length_error	Thrown if the resulting string would be too long.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const_iterator iter_newBeg, const_iterator iter_newEnd)
	{
		if (iter_newBeg == iter_newEnd)
		{
			erase(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg));
		}
		else
		{
			size_type str_len = safe_iter_dif(iter_newEnd, iter_newBeg);
			size_type idx = safe_iter_dif(iter_beg, begin());
			size_type len = safe_iter_dif(iter_end, iter_beg);

			if ((len + idx) > d_cplength)
				len = d_cplength - idx;

			size_type newsz = d_cplength + str_len - len;

			grow(newsz);

			if ((idx + len) < d_cplength)
				memmove(&ptr()[idx + str_len], &ptr()[len + idx], (d_cplength - idx - len) * sizeof(utf32));

			memcpy(&ptr()[idx], iter_newBeg.d_ptr, str_len * sizeof(utf32));
			setlen(newsz);
		}

		return *this;
	}


	/*!
	\brief
		Replace code points in the String with the specified c-string.

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced (if this is 0, operation is an insert at position \a idx)

	\param c_str
		c-string that is to replace the specified code points

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(size_type idx, size_type len, const char* cstr)
	{
		return replace(idx, len, cstr, strlen(cstr));
	}


	/*!
	\brief
		Replace the code points in the range [beg, end) with the specified c-string.

	\note
		If \a beg == \a end, the operation is a insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param c_str
		c-string that is to replace the specified range of code points

	\return
		This String after the replace operation

	\exception std::length_error	Thrown if the resulting String would be too large.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const char* cstr)
	{
		return replace(iter_beg, iter_end, cstr, strlen(cstr));
	}


	/*!
	\brief
		Replace code points in the String with chars from the given char array.

	\param idx
		Index of the first code point to be replaced

	\param len
		Maximum number of code points to be replaced (if this is 0, operation is an insert at position \a idx)

	\param chars
		char array containing the cars that are to replace the specified code points

	\param chars_len
		Number of chars in the char array.

	\return
		This String after the replace operation

	\exception std::out_of_range	Thrown if \a idx is invalid for this String
	\exception std::length_error	Thrown if the resulting String would be too large, or if \a chars_len was 'npos'.
	*/
	String& replace(size_type idx, size_type len, const char* chars, size_type chars_len)
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for CEGUI::String"));

		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for the char array can not be 'npos'"));

		if (((len + idx) > d_cplength) || (len == npos))
			len = d_cplength - idx;

		size_type newsz = d_cplength + chars_len - len;

		grow(newsz);

		if ((idx + len) < d_cplength)
			memmove(&ptr()[idx + chars_len], &ptr()[len + idx], (d_cplength - idx - len) * sizeof(utf32));

		utf32* pt = &ptr()[idx + chars_len - 1];

		while (chars_len--)
			*pt-- = static_cast<utf32>(static_cast<unsigned char>(chars[chars_len]));

		setlen(newsz);
		return *this;
	}


	/*!
	\brief
		Replace the code points in the range [beg, end) with chars from the given char array.

	\note
		If \a beg == \a end, the operation is a insert at iterator position \a beg

	\param beg
		Iterator describing the start of the range to be replaced

	\param end
		Iterator describing the (exclusive) end of the range to be replaced.

	\param chars
		char array containing the chars that are to replace the specified range of code points

	\param chars_len
		Number of chars in the char array.

	\return
		This String after the replace operation

	\exception std::length_error	Thrown if the resulting String would be too large, or if \a chars_len was 'npos'.
	*/
	String& replace(iterator iter_beg, iterator iter_end, const char* chars, size_type chars_len)
	{
		return replace(safe_iter_dif(iter_beg, begin()), safe_iter_dif(iter_end, iter_beg), chars, chars_len);
	}


	//////////////////////////////////////////////////////////////////////////
	// Find a code point
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Search forwards for a given code point

	\param code_point
		The utf32 code point to search for

	\param idx
		Index of the code point where the search is to start.

	\return
		- Index of the first occurrence of \a code_point travelling forwards from \a idx.
		- npos if the code point could not be found
	*/
	size_type	find(utf32 code_point, size_type idx = 0) const
	{
		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			while (idx < d_cplength)
			{
				if (*pt++ == code_point)
					return idx;

				++idx;
			}

		}

		return npos;
	}

	/*!
	\brief
		Search backwards for a given code point

	\param code_point
		The utf32 code point to search for

	\param idx
		Index of the code point where the search is to start.

	\return
		- Index of the first occurrence of \a code_point travelling backwards from \a idx.
		- npos if the code point could not be found
	*/
	size_type	rfind(utf32 code_point, size_type idx = npos) const
	{
		if (idx >= d_cplength)
			idx = d_cplength - 1;

		if (d_cplength > 0)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (*pt-- == code_point)
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	//////////////////////////////////////////////////////////////////////////
	// Find a substring
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Search forwards for a sub-string

	\param str
		String object describing the sub-string to search for

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a str travelling forwards from \a idx.
		- npos if the sub-string could not be found
	*/
	size_type	find(const String& str, size_type idx = 0) const
	{
		if ((str.d_cplength == 0) && (idx < d_cplength))
			return idx;

		if (idx < d_cplength)
		{
			// loop while search string could fit in to search area
			while (d_cplength - idx >= str.d_cplength)
			{
				if (0 == compare(idx, str.d_cplength, str))
					return idx;

				++idx;
			}

		}

		return npos;
	}

	/*!
	\brief
		Search backwards for a sub-string

	\param str
		String object describing the sub-string to search for

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a str travelling backwards from \a idx.
		- npos if the sub-string could not be found
	*/
	size_type	rfind(const String& str, size_type idx = npos) const
	{
		if (str.d_cplength == 0)
			return (idx < d_cplength) ? idx : d_cplength;

		if (str.d_cplength <= d_cplength)
		{
			if (idx > (d_cplength - str.d_cplength))
				idx = d_cplength - str.d_cplength;

			do
			{
				if (0 == compare(idx, str.d_cplength, str))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	/*!
	\brief
		Search forwards for a sub-string

	\param std_str
		std::string object describing the sub-string to search for

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a std_str travelling forwards from \a idx.
		- npos if the sub-string could not be found
	*/
	size_type	find(const std::string& std_str, size_type idx = 0) const
	{
		std::string::size_type sze = std_str.size();

		if ((sze == 0) && (idx < d_cplength))
			return idx;

		if (idx < d_cplength)
		{
			// loop while search string could fit in to search area
			while (d_cplength - idx >= sze)
			{
				if (0 == compare(idx, (size_type)sze, std_str))
					return idx;

				++idx;
			}

		}

		return npos;
	}

	/*!
	\brief
		Search backwards for a sub-string

	\param std_str
		std::string object describing the sub-string to search for

	\note
		Characters from \a std_str are considered to represent Unicode code points in the range 0x00..0xFF.  No translation of
		the encountered data is performed.

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a std_str travelling backwards from \a idx.
		- npos if the sub-string could not be found
	*/
	size_type	rfind(const std::string& std_str, size_type idx = npos) const
	{
		std::string::size_type sze = std_str.size();

		if (sze == 0)
			return (idx < d_cplength) ? idx : d_cplength;

		if (sze <= d_cplength)
		{
			if (idx > (d_cplength - sze))
				idx = d_cplength - sze;

			do
			{
				if (0 == compare(idx, (size_type)sze, std_str))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	/*!
	\brief
		Search forwards for a sub-string

	\param utf8_str
		Buffer containing null-terminated utf8 encoded data describing the sub-string to search for

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a utf8_str travelling forwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find(const utf8* utf8_str, size_type idx = 0) const
	{
		return find(utf8_str, idx, utf_length(utf8_str));
	}

	/*!
	\brief
		Search backwards for a sub-string

	\param utf8_str
		Buffer containing null-terminated utf8 encoded data describing the sub-string to search for

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a utf8_str travelling backwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	rfind(const utf8* utf8_str, size_type idx = npos) const
	{
		return rfind(utf8_str, idx, utf_length(utf8_str));
	}

	/*!
	\brief
		Search forwards for a sub-string

	\param utf8_str
		Buffer containing utf8 encoded data describing the sub-string to search for

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the code point where the search is to start

	\param str_len
		Length of the utf8 encoded sub-string in utf8 code units (not code points)

	\return
		- Index of the first occurrence of sub-string \a utf8_str travelling forwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::length_error	Thrown if \a str_len is 'npos'
	*/
	size_type	find(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		size_type sze = encoded_size(utf8_str, str_len);

		if ((sze == 0) && (idx < d_cplength))
			return idx;

		if (idx < d_cplength)
		{
			// loop while search string could fit in to search area
			while (d_cplength - idx >= sze)
			{
				if (0 == compare(idx, sze, utf8_str, sze))
					return idx;

				++idx;
			}

		}

		return npos;
	}

	/*!
	\brief
		Search backwards for a sub-string

	\param utf8_str
		Buffer containing utf8 encoded data describing the sub-string to search for

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the code point where the search is to start

	\param str_len
		Length of the utf8 encoded sub-string in utf8 code units (not code points)

	\return
		- Index of the first occurrence of sub-string \a utf8_str travelling backwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::length_error	Thrown if \a str_len is 'npos'
	*/
	size_type	rfind(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		size_type sze = encoded_size(utf8_str, str_len);

		if (sze == 0)
			return (idx < d_cplength) ? idx : d_cplength;

		if (sze <= d_cplength)
		{
			if (idx > (d_cplength - sze))
				idx = d_cplength - sze;

			do
			{
				if (0 == compare(idx, sze, utf8_str, sze))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	/*!
	\brief
		Search forwards for a sub-string

	\param c_str
		c-string describing the sub-string to search for

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a c_str travelling forwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find(const char* cstr, size_type idx = 0) const
	{
		return find(cstr, idx, strlen(cstr));
	}


	/*!
	\brief
		Search backwards for a sub-string

	\param c_str
		c-string describing the sub-string to search for

	\param idx
		Index of the code point where the search is to start

	\return
		- Index of the first occurrence of sub-string \a c_str travelling backwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	rfind(const char* cstr, size_type idx = npos) const
	{
		return rfind(cstr, idx, strlen(cstr));
	}


	/*!
	\brief
		Search forwards for a sub-string

	\param chars
		char array describing the sub-string to search for

	\param idx
		Index of the code point where the search is to start

	\param chars_len
		Number of chars in the char array.

	\return
		- Index of the first occurrence of sub-string \a chars travelling forwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::length_error	Thrown if \a chars_len is 'npos'
	*/
	size_type	find(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if ((chars_len == 0) && (idx < d_cplength))
			return idx;

		if (idx < d_cplength)
		{
			// loop while search string could fit in to search area
			while (d_cplength - idx >= chars_len)
			{
				if (0 == compare(idx, chars_len, chars, chars_len))
					return idx;

				++idx;
			}

		}

		return npos;
	}


	/*!
	\brief
		Search backwards for a sub-string

	\param chars
		char array describing the sub-string to search for

	\param idx
		Index of the code point where the search is to start

	\param chars_len
		Number of chars in the char array.

	\return
		- Index of the first occurrence of sub-string \a chars travelling backwards from \a idx.
		- npos if the sub-string could not be found

	\exception std::length_error	Thrown if \a chars_len is 'npos'
	*/
	size_type	rfind(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if (chars_len == 0)
			return (idx < d_cplength) ? idx : d_cplength;

		if (chars_len <= d_cplength)
		{
			if (idx > (d_cplength - chars_len))
				idx = d_cplength - chars_len;

			do
			{
				if (0 == compare(idx, chars_len, chars, chars_len))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	//////////////////////////////////////////////////////////////////////////
	// Find first of different code-points
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Find the first occurrence of one of a set of code points.

	\param str
		String object describing the set of code points.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first occurrence of any one of the code points in \a str starting from from \a idx.
		- npos if none of the code points in \a str were found.
	*/
	size_type	find_first_of(const String& str, size_type idx = 0) const
	{
		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != str.find(*pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}

	/*!
	\brief
		Find the first code point that is not one of a set of code points.

	\param str
		String object describing the set of code points.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first code point that does not match any one of the code points in \a str starting from from \a idx.
		- npos if all code points matched one of the code points in \a str.
	*/
	size_type	find_first_not_of(const String& str, size_type idx = 0) const
	{
		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == str.find(*pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}


	/*!
	\brief
		Find the first occurrence of one of a set of code points.

	\param std_str
		std::string object describing the set of code points.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first occurrence of any one of the code points in \a std_str starting from from \a idx.
		- npos if none of the code points in \a std_str were found.
	*/
	size_type	find_first_of(const std::string& std_str, size_type idx = 0) const
	{
		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(std_str, *pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}

	/*!
	\brief
		Find the first code point that is not one of a set of code points.

	\param std_str
		std::string object describing the set of code points.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first code point that does not match any one of the code points in \a std_str starting from from \a idx.
		- npos if all code points matched one of the code points in \a std_str.
	*/
	size_type	find_first_not_of(const std::string& std_str, size_type idx = 0) const
	{
		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(std_str, *pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}


	/*!
	\brief
		Find the first occurrence of one of a set of code points.

	\param utf8_str
		Buffer containing null-terminated utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first occurrence of any one of the code points in \a utf8_str starting from from \a idx.
		- npos if none of the code points in \a utf8_str were found.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_first_of(const utf8* utf8_str, size_type idx = 0) const
	{
		return find_first_of(utf8_str, idx, utf_length(utf8_str));
	}

	/*!
	\brief
		Find the first code point that is not one of a set of code points.

	\param utf8_str
		Buffer containing null-terminated utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first code point that does not match any one of the code points in \a utf8_str starting from from \a idx.
		- npos if all code points matched one of the code points in \a utf8_str.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_first_not_of(const utf8* utf8_str, size_type idx = 0) const
	{
		return find_first_not_of(utf8_str, idx, utf_length(utf8_str));
	}

	/*!
	\brief
		Find the first occurrence of one of a set of code points.

	\param utf8_str
		Buffer containing utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\param str_len
		Length of the utf8 encoded data in utf8 code units (not code points).

	\return
		- Index of the first occurrence of any one of the code points in \a utf8_str starting from from \a idx.
		- npos if none of the code points in \a utf8_str were found.

	\exception std::length_error	Thrown if \a str_len was 'npos'.
	*/
	size_type	find_first_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		if (idx < d_cplength)
		{
			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(utf8_str, encsze, *pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}

	/*!
	\brief
		Find the first code point that is not one of a set of code points.

	\param utf8_str
		Buffer containing utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\param str_len
		Length of the utf8 encoded data in utf8 code units (not code points).

	\return
		- Index of the first code point that does not match any one of the code points in \a utf8_str starting from from \a idx.
		- npos if all code points matched one of the code points in \a utf8_str.

	\exception std::length_error	Thrown if \a str_len was 'npos'.
	*/
	size_type	find_first_not_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		if (idx < d_cplength)
		{
			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(utf8_str, encsze, *pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}


	/*!
	\brief
		Search forwards for a given code point

	\param code_point
		The utf32 code point to search for

	\param idx
		Index of the code point where the search is to start.

	\return
		- Index of the first occurrence of \a code_point starting from from \a idx.
		- npos if the code point could not be found
	*/
	size_type	find_first_of(utf32 code_point, size_type idx = 0) const
	{
		return find(code_point, idx);
	}

	/*!
	\brief
		Search forwards for the first code point that does not match a given code point

	\param code_point
		The utf32 code point to search for

	\param idx
		Index of the code point where the search is to start.

	\return
		- Index of the first code point that does not match \a code_point starting from from \a idx.
		- npos if all code points matched \a code_point

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_first_not_of(utf32 code_point, size_type idx = 0) const
	{
		if (idx < d_cplength)
		{
			do
			{
				if ((*this)[idx] != code_point)
					return idx;

			} while(idx++ < d_cplength);

		}

		return npos;
	}


	/*!
	\brief
		Find the first occurrence of one of a set of chars.

	\param c_str
		c-string describing the set of chars.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first occurrence of any one of the chars in \a c_str starting from from \a idx.
		- npos if none of the chars in \a c_str were found.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_first_of(const char* cstr, size_type idx = 0) const
	{
		return find_first_of(cstr, idx, strlen(cstr));
	}


	/*!
	\brief
		Find the first code point that is not one of a set of chars.

	\param c_str
		c-string describing the set of chars.

	\param idx
		Index of the start point for the search

	\return
		- Index of the first code point that does not match any one of the chars in \a c_str starting from from \a idx.
		- npos if all code points matched any of the chars in \a c_str.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_first_not_of(const char* cstr, size_type idx = 0) const
	{
		return find_first_not_of(cstr, idx, strlen(cstr));
	}


	/*!
	\brief
		Find the first occurrence of one of a set of chars.

	\param chars
		char array containing the set of chars.

	\param idx
		Index of the start point for the search

	\param chars_len
		Number of chars in the char array.

	\return
		- Index of the first occurrence of any one of the chars in \a chars starting from from \a idx.
		- npos if none of the chars in \a chars were found.

	\exception std::length_error	Thrown if \a chars_len was 'npos'.
	*/
	size_type	find_first_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(chars, chars_len, *pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}


	/*!
	\brief
		Find the first code point that is not one of a set of chars.

	\param chars
		char array containing the set of chars.

	\param idx
		Index of the start point for the search

	\param chars_len
		Number of chars in the car array.

	\return
		- Index of the first code point that does not match any one of the chars in \a chars starting from from \a idx.
		- npos if all code points matched any of the chars in \a chars.

	\exception std::length_error	Thrown if \a chars_len was 'npos'.
	*/
	size_type	find_first_not_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if (idx < d_cplength)
		{
			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(chars, chars_len, *pt++))
					return idx;

			} while (++idx != d_cplength);

		}

		return npos;
	}


	//////////////////////////////////////////////////////////////////////////
	// Find last of different code-points
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Find the last occurrence of one of a set of code points.

	\param str
		String object describing the set of code points.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last occurrence of any one of the code points in \a str starting from \a idx.
		- npos if none of the code points in \a str were found.
	*/
	size_type	find_last_of(const String& str, size_type idx = npos) const
	{
		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != str.find(*pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	/*!
	\brief
		Find the last code point that is not one of a set of code points.

	\param str
		String object describing the set of code points.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last code point that does not match any one of the code points in \a str starting from \a idx.
		- npos if all code points matched one of the code points in \a str.
	*/
	size_type	find_last_not_of(const String& str, size_type idx = npos) const
	{
		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == str.find(*pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	/*!
	\brief
		Find the last occurrence of one of a set of code points.

	\param std_str
		std::string object describing the set of code points.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last occurrence of any one of the code points in \a std_str starting from \a idx.
		- npos if none of the code points in \a std_str were found.
	*/
	size_type	find_last_of(const std::string& std_str, size_type idx = npos) const
	{
		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(std_str, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	/*!
	\brief
		Find the last code point that is not one of a set of code points.

	\param std_str
		std::string object describing the set of code points.

	\note
		The characters of \a std_str are taken to be unencoded data which represent Unicode code points 0x00..0xFF.  No translation of
		the provided data will occur.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last code point that does not match any one of the code points in \a std_str starting from \a idx.
		- npos if all code points matched one of the code points in \a std_str.
	*/
	size_type	find_last_not_of(const std::string& std_str, size_type idx = npos) const
	{
		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(std_str, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	/*!
	\brief
		Find the last occurrence of one of a set of code points.

	\param utf8_str
		Buffer containing null-terminated utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last occurrence of any one of the code points in \a utf8_str starting from \a idx.
		- npos if none of the code points in \a utf8_str were found.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_last_of(const utf8* utf8_str, size_type idx = npos) const
	{
		return find_last_of(utf8_str, idx, utf_length(utf8_str));
	}

	/*!
	\brief
		Find the last code point that is not one of a set of code points.

	\param utf8_str
		Buffer containing null-terminated utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last code point that does not match any one of the code points in \a utf8_str starting from \a idx.
		- npos if all code points matched one of the code points in \a utf8_str.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_last_not_of(const utf8* utf8_str, size_type idx = npos) const
	{
		return find_last_not_of(utf8_str, idx, utf_length(utf8_str));
	}

	/*!
	\brief
		Find the last occurrence of one of a set of code points.

	\param utf8_str
		Buffer containing utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\param str_len
		Length of the utf8 encoded data in utf8 code units (not code points).

	\return
		- Index of the last occurrence of any one of the code points in \a utf8_str starting from from \a idx.
		- npos if none of the code points in \a utf8_str were found.

	\exception std::length_error	Thrown if \a str_len was 'npos'.
	*/
	size_type	find_last_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(utf8_str, encsze, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}

	/*!
	\brief
		Find the last code point that is not one of a set of code points.

	\param utf8_str
		Buffer containing utf8 encoded data describing the set of code points.

	\note
		A basic string literal (cast to utf8*) can be passed to this function, provided that the string is
		comprised only of code points 0x00..0x7f.  The use of extended ASCII characters (with values >0x7f)
		would result in incorrect behaviour as the String will attempt to 'decode' the data, with unpredictable
		results.

	\param idx
		Index of the start point for the search

	\param str_len
		Length of the utf8 encoded data in utf8 code units (not code points).

	\return
		- Index of the last code point that does not match any one of the code points in \a utf8_str starting from from \a idx.
		- npos if all code points matched one of the code points in \a utf8_str.

	\exception std::length_error	Thrown if \a str_len was 'npos'.
	*/
	size_type	find_last_not_of(const utf8* utf8_str, size_type idx, size_type str_len) const
	{
		if (str_len == npos)
			CEGUI_THROW(std::length_error("Length for utf8 encoded string can not be 'npos'"));

		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			size_type encsze = encoded_size(utf8_str, str_len);

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(utf8_str, encsze, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	/*!
	\brief
		Search for last occurrence of a given code point

	\param code_point
		The utf32 code point to search for

	\param idx
		Index of the code point where the search is to start.

	\return
		- Index of the last occurrence of \a code_point starting from \a idx.
		- npos if the code point could not be found
	*/
	size_type	find_last_of(utf32 code_point, size_type idx = npos) const
	{
		return rfind(code_point, idx);
	}

	/*!
	\brief
		Search for the last code point that does not match a given code point

	\param code_point
		The utf32 code point to search for

	\param idx
		Index of the code point where the search is to start.

	\return
		- Index of the last code point that does not match \a code_point starting from from \a idx.
		- npos if all code points matched \a code_point
	*/
	size_type	find_last_not_of(utf32 code_point, size_type idx = npos) const
	{
		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			do
			{
				if ((*this)[idx] != code_point)
					return idx;

			} while(idx-- != 0);

		}

		return npos;
	}


	/*!
	\brief
		Find the last occurrence of one of a set of chars.

	\param c_str
		c-string describing the set of chars.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last occurrence of any one of the chars in \a c_str starting from \a idx.
		- npos if none of the chars in \a c_str were found.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_last_of(const char* cstr, size_type idx = npos) const
	{
		return find_last_of(cstr, idx, strlen(cstr));
	}


	/*!
	\brief
		Find the last code point that is not one of a set of chars.

	\param c_str
		c-string describing the set of chars.

	\param idx
		Index of the start point for the search

	\return
		- Index of the last code point that does not match any one of the chars in \a c_str starting from \a idx.
		- npos if all code points matched any of the chars in \a c_str.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	size_type	find_last_not_of(const char* cstr, size_type idx = npos) const
	{
		return find_last_not_of(cstr, idx, strlen(cstr));
	}


	/*!
	\brief
		Find the last occurrence of one of a set of chars.

	\param chars
		char array containing the set of chars.

	\param idx
		Index of the start point for the search

	\param chars_len
		Number of chars in the char array.

	\return
		- Index of the last occurrence of any one of the chars in \a chars, starting from from \a idx.
		- npos if none of the chars in \a chars were found.

	\exception std::length_error	Thrown if \a chars_len was 'npos'.
	*/
	size_type	find_last_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos != find_codepoint(chars, chars_len, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	/*!
	\brief
		Find the last code point that is not one of a set of chars.

	\param chars
		char array containing the set of chars.

	\param idx
		Index of the start point for the search

	\param chars_len
		Number of chars in the char array.

	\return
		- Index of the last code point that does not match any one of the chars in \a chars, starting from from \a idx.
		- npos if all code points matched any of the chars in \a chars.

	\exception std::length_error	Thrown if \a chars_len was 'npos'.
	*/
	size_type	find_last_not_of(const char* chars, size_type idx, size_type chars_len) const
	{
		if (chars_len == npos)
			CEGUI_THROW(std::length_error("Length for char array can not be 'npos'"));

		if (d_cplength > 0)
		{
			if (idx >= d_cplength)
				idx = d_cplength - 1;

			const utf32* pt = &ptr()[idx];

			do
			{
				if (npos == find_codepoint(chars, chars_len, *pt--))
					return idx;

			} while (idx-- != 0);

		}

		return npos;
	}


	//////////////////////////////////////////////////////////////////////////
	// Substring
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Returns a substring of this String.

	\param idx
		Index of the first code point to use for the sub-string.

	\param len
		Maximum number of code points to use for the sub-string

	\return
		A String object containing the specified sub-string.

	\exception std::out_of_range	Thrown if \a idx is invalid for this String.
	*/
	String	substr(size_type idx = 0, size_type len = npos) const
	{
		if (d_cplength < idx)
			CEGUI_THROW(std::out_of_range("Index is out of range for this CEGUI::String"));

		return String(*this, idx, len);
	}

	//////////////////////////////////////////////////////////////////////////
	// Iterator creation
	//////////////////////////////////////////////////////////////////////////
	/*!
	\brief
		Return a forwards iterator that describes the beginning of the String

	\return
		iterator object that describes the beginning of the String.
	*/
	iterator		begin(void)
	{
		return iterator(ptr());
	}

	/*!
	\brief
		Return a constant forwards iterator that describes the beginning of the String

	\return
		const_iterator object that describes the beginning of the String.
	*/
	const_iterator	begin(void) const
	{
		return const_iterator(ptr());
	}

	/*!
	\brief
		Return a forwards iterator that describes the end of the String

	\return
		iterator object that describes the end of the String.
	*/
	iterator		end(void)
	{
		return iterator(&ptr()[d_cplength]);
	}

	/*!
	\brief
		Return a constant forwards iterator that describes the end of the String

	\return
		const_iterator object that describes the end of the String.
	*/
	const_iterator	end(void) const
	{
		return const_iterator(&ptr()[d_cplength]);
	}

	/*!
	\brief
		Return a reverse iterator that describes the beginning of the String

	\return
		reverse_iterator object that describes the beginning of the String (so is actually at the end)
	*/
	reverse_iterator		rbegin(void)
	{
		return reverse_iterator(end());
	}

	/*!
	\brief
		Return a constant reverse iterator that describes the beginning of the String

	\return
		const_reverse_iterator object that describes the beginning of the String (so is actually at the end)
	*/
	const_reverse_iterator	rbegin(void) const
	{
		return const_reverse_iterator(end());
	}

	/*!
	\brief
		Return a reverse iterator that describes the end of the String

	\return
		reverse_iterator object that describes the end of the String (so is actually at the beginning)
	*/
	reverse_iterator		rend(void)
	{
		return reverse_iterator(begin());
	}

	/*!
	\brief
		Return a constant reverse iterator that describes the end of the String

	\return
		const_reverse_iterator object that describes the end of the String (so is actually at the beginning)
	*/
	const_reverse_iterator	rend(void) const
	{
		return const_reverse_iterator(begin());
	}

private:
	/*************************************************************************
		Implementation Functions
	*************************************************************************/
	// string management

	// change size of allocated buffer so it is at least 'new_size'.
	// May or may not cause re-allocation and copy of buffer if size is larger
	// will never re-allocate to make size smaller.  (see trim())
    bool	grow(size_type new_size);

	// perform re-allocation to remove wasted space.
    void	trim(void);

	// set the length of the string, and terminate it, according to the given value (will not re-allocate, use grow() first).
	void	setlen(size_type len)
	{
		d_cplength = len;
		ptr()[len] = (utf32)(0);
	}

	// initialise string object
	void	init(void)
	{
		d_reserve			= CEGUI_STR_QUICKBUFF_SIZE;
		d_encodedbuff		= 0;
		d_encodedbufflen	= 0;
		d_encodeddatlen		= 0;
        d_buffer            = 0;
		setlen(0);
	}

	// return true if the given pointer is inside the string data
	bool	inside(utf32* inptr)
	{
		if (inptr < ptr() || ptr() + d_cplength <= inptr)
			return false;
		else
			return true;
	}

	// compute distance between two iterators, returning a 'safe' value
	size_type safe_iter_dif(const const_iterator& iter1, const const_iterator& iter2) const
	{
		return (iter1.d_ptr == 0) ? 0 : (iter1 - iter2);
	}

	// encoding functions
	// for all:
	//	src_len is in code units, or 0 for null terminated string.
	//	dest_len is in code units.
	//	returns number of code units put into dest buffer.
	size_type encode(const utf32* src, utf8* dest, size_type dest_len, size_type src_len = 0) const
	{
		// count length for null terminated source...
		if (src_len == 0)
		{
			src_len = utf_length(src);
		}

		size_type destCapacity = dest_len;

		// while there is data in the source buffer,
		for (uint idx = 0; idx < src_len; ++idx)
		{
			utf32	cp = src[idx];

			// check there is enough destination buffer to receive this encoded unit (exit loop & return if not)
			if (destCapacity < encoded_size(cp))
			{
				break;
			}

			if (cp < 0x80)
			{
				*dest++ = (utf8)cp;
				--destCapacity;
			}
			else if (cp < 0x0800)
			{
				*dest++ = (utf8)((cp >> 6) | 0xC0);
				*dest++ = (utf8)((cp & 0x3F) | 0x80);
				destCapacity -= 2;
			}
			else if (cp < 0x10000)
			{
				*dest++ = (utf8)((cp >> 12) | 0xE0);
				*dest++ = (utf8)(((cp >> 6) & 0x3F) | 0x80);
				*dest++ = (utf8)((cp & 0x3F) | 0x80);
				destCapacity -= 3;
			}
			else
			{
				*dest++ = (utf8)((cp >> 18) | 0xF0);
				*dest++ = (utf8)(((cp >> 12) & 0x3F) | 0x80);
				*dest++ = (utf8)(((cp >> 6) & 0x3F) | 0x80);
				*dest++ = (utf8)((cp & 0x3F) | 0x80);
				destCapacity -= 4;
			}

		}

		return dest_len - destCapacity;
	}

	size_type encode(const utf8* src, utf32* dest, size_type dest_len, size_type src_len = 0) const
	{
		// count length for null terminated source...
		if (src_len == 0)
		{
			src_len = utf_length(src);
		}

		size_type destCapacity = dest_len;

		// while there is data in the source buffer, and space in the dest buffer
		for (uint idx = 0; ((idx < src_len) && (destCapacity > 0));)
		{
			utf32	cp;
			utf8	cu = src[idx++];

			if (cu < 0x80)
			{
				cp = (utf32)(cu);
			}
			else if (cu < 0xE0)
			{
				cp = ((cu & 0x1F) << 6);
				cp |= (src[idx++] & 0x3F);
			}
			else if (cu < 0xF0)
			{
				cp = ((cu & 0x0F) << 12);
				cp |= ((src[idx++] & 0x3F) << 6);
				cp |= (src[idx++] & 0x3F);
			}
			else
			{
				cp = ((cu & 0x07) << 18);
				cp |= ((src[idx++] & 0x3F) << 12);
				cp |= ((src[idx++] & 0x3F) << 6);
				cp |= (src[idx++] & 0x3F);
			}

			*dest++ = cp;
			--destCapacity;
		}

		return dest_len - destCapacity;
	}

	// return the number of utf8 code units required to encode the given utf32 code point
	size_type encoded_size(utf32 code_point) const
	{
		if (code_point < 0x80)
			return 1;
		else if (code_point < 0x0800)
			return 2;
		else if (code_point < 0x10000)
			return 3;
		else
			return 4;
	}

	// return number of code units required to re-encode given null-terminated utf32 data as utf8.  return does not include terminating null.
	size_type encoded_size(const utf32* buf) const
	{
		return encoded_size(buf, utf_length(buf));
	}

	// return number of code units required to re-encode given utf32 data as utf8.   len is number of code units in 'buf'.
	size_type encoded_size(const utf32* buf, size_type len) const
	{
		size_type count = 0;

		while (len--)
		{
			count += encoded_size(*buf++);
		}

		return count;
	}

	// return number of utf32 code units required to re-encode given utf8 data as utf32.  return does not include terminating null.
	size_type encoded_size(const utf8* buf) const
	{
		return encoded_size(buf, utf_length(buf));
	}

	// return number of utf32 code units required to re-encode given utf8 data as utf32.  len is number of code units in 'buf'.
	size_type encoded_size(const utf8* buf, size_type len) const
	{
		utf8 tcp;
		size_type count = 0;

		while (len--)
		{
			tcp = *buf++;
			++count;
			size_type size = 0;

			if (tcp < 0x80)
			{
			}
			else if (tcp < 0xE0)
			{
				size = 1;
				++buf;
			}
			else if (tcp < 0xF0)
			{
				size = 2;
				buf += 2;
			}
			else
			{
				size = 3;
				buf += 3;
			}

			if (len >= size)
				len -= size;
			else 
				break;
		}

		return count;
	}

	// return number of code units in a null terminated string
	size_type utf_length(const utf8* utf8_str) const
	{
		size_type cnt = 0;
		while (*utf8_str++)
			cnt++;

		return cnt;
	}

	// return number of code units in a null terminated string
	size_type utf_length(const utf32* utf32_str) const
	{
		size_type cnt = 0;
		while (*utf32_str++)
			cnt++;

		return cnt;
	}

	// build an internal buffer with the string encoded as utf8 (remains valid until string is modified).
    utf8* build_utf8_buff(void) const;

	// compare two utf32 buffers
	int	utf32_comp_utf32(const utf32* buf1, const utf32* buf2, size_type cp_count) const
	{
		if (!cp_count)
			return 0;

		while ((--cp_count) && (*buf1 == *buf2))
			buf1++, buf2++;

		return *buf1 - *buf2;
	}

	// compare utf32 buffer with char buffer (chars are taken to be code-points in the range 0x00-0xFF)
	int utf32_comp_char(const utf32* buf1, const char* buf2, size_type cp_count) const
	{
		if (!cp_count)
			return 0;

		while ((--cp_count) && (*buf1 == static_cast<utf32>(static_cast<unsigned char>(*buf2))))
			buf1++, buf2++;

		return *buf1 - static_cast<utf32>(static_cast<unsigned char>(*buf2));
	}

	// compare utf32 buffer with encoded utf8 data
	int utf32_comp_utf8(const utf32* buf1, const utf8* buf2, size_type cp_count) const
	{
		if (!cp_count)
			return 0;

		utf32	cp;
		utf8	cu;

		do
		{
			cu = *buf2++;

			if (cu < 0x80)
			{
				cp = (utf32)(cu);
			}
			else if (cu < 0xE0)
			{
				cp = ((cu & 0x1F) << 6);
				cp |= (*buf2++ & 0x3F);
			}
			else if (cu < 0xF0)
			{
				cp = ((cu & 0x0F) << 12);
				cp |= ((*buf2++ & 0x3F) << 6);
				cp |= (*buf2++ & 0x3F);
			}
			else
			{
				cp = ((cu & 0x07) << 18);
				cp |= ((*buf2++ & 0x3F) << 12);
				cp |= ((*buf2++ & 0x3F) << 6);
				cp |= (*buf2++ & 0x3F);
			}

		} while ((*buf1++ == cp) && (--cp_count));

		return (*--buf1) - cp;
	}

	// return index of first occurrence of 'code_point' in std::string 'str', or npos if none
	size_type find_codepoint(const std::string& str, utf32 code_point) const
	{
		size_type idx = 0, sze = (size_type)str.size();

		while (idx != sze)
		{
			if (code_point == static_cast<utf32>(static_cast<unsigned char>(str[idx])))
				return idx;

			++idx;
		}

		return npos;
	}

	// return index of first occurrence of 'code_point' in utf8 encoded string 'str', or npos if none.  len is in code points.
	size_type find_codepoint(const utf8* str, size_type len, utf32 code_point) const
	{
		size_type idx = 0;

		utf32	cp;
		utf8	cu;

		while (idx != len) {
			cu = *str++;

			if (cu < 0x80)
			{
				cp = (utf32)(cu);
			}
			else if (cu < 0xE0)
			{
				cp = ((cu & 0x1F) << 6);
				cp |= (*str++ & 0x3F);
			}
			else if (cu < 0xF0)
			{
				cp = ((cu & 0x0F) << 12);
				cp |= ((*str++ & 0x3F) << 6);
				cp |= (*str++ & 0x3F);
			}
			else
			{
				cp = ((cu & 0x07) << 18);
				cp |= ((*str++ & 0x3F) << 12);
				cp |= ((*str++ & 0x3F) << 6);
				cp |= (*str++ & 0x3F);
			}

			if (code_point == cp)
				return idx;

			++idx;
		}

		return npos;
	}


	// return index of first occurrence of 'code_point' in char array 'chars', or npos if none
	size_type find_codepoint(const char* chars, size_type chars_len, utf32 code_point) const
	{
		for (size_type idx = 0; idx != chars_len; ++idx)
		{
			if (code_point == static_cast<utf32>(static_cast<unsigned char>(chars[idx])))
				return idx;
		}

		return npos;
	}

};


//////////////////////////////////////////////////////////////////////////
// Comparison operators
//////////////////////////////////////////////////////////////////////////
/*!
\brief
	Return true if String \a str1 is equal to String \a str2
*/
bool CEGUIEXPORT	operator==(const String& str1, const String& str2);

/*!
\brief
	Return true if String \a str is equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator==(const String& str, const std::string& std_str);

/*!
\brief
	Return true if String \a str is equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator==(const std::string& std_str, const String& str);

/*!
\brief
	Return true if String \a str is equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator==(const String& str, const utf8* utf8_str);

/*!
\brief
	Return true if String \a str is equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator==(const utf8* utf8_str, const String& str);

/*!
\brief
	Return true if String \a str1 is not equal to String \a str2
*/
bool CEGUIEXPORT	operator!=(const String& str1, const String& str2);

/*!
\brief
	Return true if String \a str is not equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator!=(const String& str, const std::string& std_str);

/*!
\brief
	Return true if String \a str is not equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator!=(const std::string& std_str, const String& str);

/*!
\brief
	Return true if String \a str is not equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator!=(const String& str, const utf8* utf8_str);

/*!
\brief
	Return true if String \a str is not equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator!=(const utf8* utf8_str, const String& str);

/*!
\brief
	Return true if String \a str1 is lexicographically less than String \a str2
*/
bool CEGUIEXPORT	operator<(const String& str1, const String& str2);

/*!
\brief
	Return true if String \a str is lexicographically less than std::string \a std_str
*/
bool CEGUIEXPORT	operator<(const String& str, const std::string& std_str);

/*!
\brief
	Return true if String \a str is lexicographically less than std::string \a std_str
*/
bool CEGUIEXPORT	operator<(const std::string& std_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically less than null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator<(const String& str, const utf8* utf8_str);

/*!
\brief
	Return true if String \a str is lexicographically less than null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator<(const utf8* utf8_str, const String& str);

/*!
\brief
	Return true if String \a str1 is lexicographically greater than String \a str2
*/
bool CEGUIEXPORT	operator>(const String& str1, const String& str2);

/*!
\brief
	Return true if String \a str is lexicographically greater than std::string \a std_str
*/
bool CEGUIEXPORT	operator>(const String& str, const std::string& std_str);

/*!
\brief
	Return true if String \a str is lexicographically greater than std::string \a std_str
*/
bool CEGUIEXPORT	operator>(const std::string& std_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically greater than null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator>(const String& str, const utf8* utf8_str);

/*!
\brief
	Return true if String \a str is lexicographically greater than null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator>(const utf8* utf8_str, const String& str);

/*!
\brief
	Return true if String \a str1 is lexicographically less than or equal to String \a str2
*/
bool CEGUIEXPORT	operator<=(const String& str1, const String& str2);

/*!
\brief
	Return true if String \a str is lexicographically less than or equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator<=(const String& str, const std::string& std_str);

/*!
\brief
	Return true if String \a str is lexicographically less than or equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator<=(const std::string& std_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically less than or equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator<=(const String& str, const utf8* utf8_str);

/*!
\brief
	Return true if String \a str is lexicographically less than or equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator<=(const utf8* utf8_str, const String& str);

/*!
\brief
	Return true if String \a str1 is lexicographically greater than or equal to String \a str2
*/
bool CEGUIEXPORT	operator>=(const String& str1, const String& str2);

/*!
\brief
	Return true if String \a str is lexicographically greater than or equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator>=(const String& str, const std::string& std_str);

/*!
\brief
	Return true if String \a str is lexicographically greater than or equal to std::string \a std_str
*/
bool CEGUIEXPORT	operator>=(const std::string& std_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically greater than or equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator>=(const String& str, const utf8* utf8_str);

/*!
\brief
	Return true if String \a str is lexicographically greater than or equal to null-terminated utf8 data \a utf8_str
*/
bool CEGUIEXPORT	operator>=(const utf8* utf8_str, const String& str);

/*!
\brief
	Return true if String \a str is equal to c-string \a c_str
*/
bool CEGUIEXPORT	operator==(const String& str, const char* c_str);

/*!
\brief
	Return true if c-string \a c_str is equal to String \a str
*/
bool CEGUIEXPORT	operator==(const char* c_str, const String& str);

/*!
\brief
	Return true if String \a str is not equal to c-string \a c_str
*/
bool CEGUIEXPORT	operator!=(const String& str, const char* c_str);

/*!
\brief
	Return true if c-string \a c_str is not equal to String \a str
*/
bool CEGUIEXPORT	operator!=(const char* c_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically less than c-string \a c_str
*/
bool CEGUIEXPORT	operator<(const String& str, const char* c_str);

/*!
\brief
	Return true if c-string \a c_str is lexicographically less than String \a str
*/
bool CEGUIEXPORT	operator<(const char* c_str, const String& str);

/*!
\brief
Return true if String \a str is lexicographically greater than c-string \a c_str
*/
bool CEGUIEXPORT	operator>(const String& str, const char* c_str);

/*!
\brief
Return true if c-string \a c_str is lexicographically greater than String \a str
*/
bool CEGUIEXPORT	operator>(const char* c_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically less than or equal to c-string \a c_str
*/
bool CEGUIEXPORT	operator<=(const String& str, const char* c_str);

/*!
\brief
	Return true if c-string \a c_str is lexicographically less than or equal to String \a str
*/
bool CEGUIEXPORT	operator<=(const char* c_str, const String& str);

/*!
\brief
	Return true if String \a str is lexicographically greater than or equal to c-string \a c_str
*/
bool CEGUIEXPORT	operator>=(const String& str, const char* c_str);

/*!
\brief
	Return true if c-string \a c_str is lexicographically greater than or equal to String \a str
*/
bool CEGUIEXPORT	operator>=(const char* c_str, const String& str);

//////////////////////////////////////////////////////////////////////////
// Concatenation operator functions
//////////////////////////////////////////////////////////////////////////
/*!
\brief
	Return String object that is the concatenation of the given inputs

\param str1
	String object describing first part of the new string

\param str2
	String object describing the second part of the new string

\return
	A String object that is the concatenation of \a str1 and \a str2

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const String& str1, const String& str2);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param str
	String object describing first part of the new string

\param std_str
	std::string object describing the second part of the new string

\return
	A String object that is the concatenation of \a str and \a std_str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const String& str, const std::string& std_str);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param std_str
	std::string object describing the first part of the new string

\param str
	String object describing the second part of the new string

\return
	A String object that is the concatenation of \a std_str and \a str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const std::string& std_str, const String& str);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param str
	String object describing first part of the new string

\param utf8_str
	Buffer containing null-terminated utf8 encoded data describing the second part of the new string

\return
	A String object that is the concatenation of \a str and \a utf8_str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const String& str, const utf8* utf8_str);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param utf8_str
	Buffer containing null-terminated utf8 encoded data describing the first part of the new string

\param str
	String object describing the second part of the new string

\return
	A String object that is the concatenation of \a str and \a utf8_str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const utf8* utf8_str, const String& str);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param str
	String object describing the first part of the new string

\param code_point
	utf32 code point describing the second part of the new string

\return
	A String object that is the concatenation of \a str and \a code_point

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const String& str, utf32 code_point);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param code_point
	utf32 code point describing the first part of the new string

\param str
	String object describing the second part of the new string

\return
	A String object that is the concatenation of \a code_point and \a str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(utf32 code_point, const String& str);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param str
	String object describing first part of the new string

\param c_str
	c-string describing the second part of the new string

\return
	A String object that is the concatenation of \a str and \a c_str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const String& str, const char* c_str);

/*!
\brief
	Return String object that is the concatenation of the given inputs

\param c_str
	c-string describing the first part of the new string

\param str
	String object describing the second part of the new string

\return
	A String object that is the concatenation of \a c_str and \a str

\exception std::length_error	Thrown if the resulting String would be too large.
*/
String CEGUIEXPORT	operator+(const char* c_str, const String& str);


//////////////////////////////////////////////////////////////////////////
// Output (stream) functions
//////////////////////////////////////////////////////////////////////////
CEGUIEXPORT std::ostream& operator<<(std::ostream& s, const String& str);


//////////////////////////////////////////////////////////////////////////
// Modifying operations
//////////////////////////////////////////////////////////////////////////
/*!
\brief
	Swap the contents for two String objects

\param str1
	String object who's contents are to be swapped with \a str2

\param str2
	String object who's contents are to be swapped with \a str1

\return
	Nothing
*/
void CEGUIEXPORT swap(String& str1, String& str2);

/*!
\brief
    Functor that can be used as comparator in a std::map with String keys.
    It's faster than using the default, but the map will no longer be sorted alphabetically.
*/
struct StringFastLessCompare
{
    bool operator() (const String& a, const String& b) const
    {
        const size_t la = a.length();
        const size_t lb = b.length();
        if (la == lb)
            return (memcmp(a.ptr(), b.ptr(), la * sizeof(utf32)) < 0);

        return (la < lb);
    }
};

#else

/// encoded char signifies that it's a char (8bit) with encoding (in this case ASCII)
typedef char encoded_char;

#if CEGUI_STRING_CLASS == CEGUI_STRING_CLASS_STD

typedef std::string String;

#else // CEGUI_STRING_CLASS_STD_AO

typedef std::basic_string<char, std::char_traits<char>, STLAllocatorWrapper<char, AllocatorConfig<STLAllocator>::Allocator> > String;

#endif

/*!
\brief
    Functor that can be used as comparator in a std::map with String keys.
    It's faster than using the default, but the map will no longer be sorted alphabetically.
*/
struct StringFastLessCompare
{
    bool operator() (const String& a, const String& b) const
    {
        const size_t la = a.length();
        const size_t lb = b.length();
        if (la == lb)
            return (memcmp(a.c_str(), b.c_str(), la * sizeof(String::value_type)) < 0);

        return (la < lb);
    }
};

#if defined(_MSC_VER)
#	pragma warning(disable : 4251)
#endif

#endif

} // End of  CEGUI namespace section


#endif	// end of guard _CEGUIString_h_
