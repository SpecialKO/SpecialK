/***********************************************************************
    created:    Thu May 31 2012
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIStringTranscoder_h_
#define _CEGUIStringTranscoder_h_

#include "CEGUI/String.h"

namespace CEGUI
{
/*!
\brief
    Interface for a string transcoding utility.  Instances of classes that
    implement this interface can be used to transcode CEGUI::Strings to and
    from some other character encodings not directly supported by the
    CEGUI::String class.
*/
class CEGUIEXPORT StringTranscoder :
    public AllocatedObject<StringTranscoder>
{
public:
    /*!
    \brief
        Transcode the given string to a UTF-16 encoded buffer.

    \param input
        String object with the text to be transcoded.

    \return
        Pointer to an array of utf16 values.  This buffer should be deleted by
        calling the deleteUTF16Buffer function.
    */
    virtual uint16* stringToUTF16(const String& input) const = 0;

    /*!
    \brief
        Transcode the given string to a std::wstring object.

    \param input
        String object with the text to be transcoded.

    \return
        std::wstring holding the transcoded data in some appropriate encoding.

    \note
        What is represented by std::wstring and how it should be interpreted is
        implementation specific.  This means that the content of the returned
        std::wstring may vary according to the operating system and compiler
        used - although what is returned should be consistent with other
        std::wstring data running on the same implementation.  This largely
        means that on Microsoft Windows you will have UTF-16 and on *nix type
        environments you will have UTF-32.
    */
    virtual std::wstring stringToStdWString(const String& input) const = 0;

    /*
    \brief
        Constructs a String object from the given null terminated UTF-16 encoded
        buffer.

    \param input
        Pointer to a null terminated array of uint16 values representing a
        string encoded using UTF-16.

    \return
        String object holding the transcoded data.
    */
    virtual String stringFromUTF16(const uint16* input) const = 0;

    /*
    \brief
        Constructs a String object from the given std::wstring.

    \param input
        reference to a std::wstring object holding the string data to be
        transcoded.

    \return
        String object holding the transcoded data.

    \note
        What is represented by std::wstring and how it should be interpreted is
        implementation specific.  This means that the content of the
        std::wstring you pass must be consistent with what is expected given the
        implementation.  This largely means that on Microsoft Windows you will
        use UTF-16 and on *nix type environments you will use UTF-32.  Do NOT
        assume that blithely passing what you think is UTF-16 data in a
        std::wstring will work everywhere - because it wont.
    */
    virtual String stringFromStdWString(const std::wstring& input) const = 0;

    //! deletes a buffer returned from the stringToUTF16 function.
    virtual void deleteUTF16Buffer(uint16* input) const = 0;

protected:
    virtual ~StringTranscoder() {}
};

}

#endif

