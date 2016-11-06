/***********************************************************************
    created:    Mon Jul 27 2009
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
#ifndef _CEGUIPCRERegexMatcher_h_
#define _CEGUIPCRERegexMatcher_h_

#include "CEGUI/RegexMatcher.h"
#include "CEGUI/String.h"
#include <pcre.h>

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
/brief
    Implementation of RegexMatcher using PCRE.

    Details about the pattern syntax used by PCRE can be found on unix-like
    systems by way of <tt>man pcrepattern</tt> (or online at
    http://www.pcre.org/pcre.txt (scroll / search "PCREPATTERN(3)").
    Alternatively, see the perl regex documentation at
    http://perldoc.perl.org/perlre.html
*/
class PCRERegexMatcher : public RegexMatcher
{
public:
    //! Constructor.
    PCRERegexMatcher();
    //! Destructor.
    ~PCRERegexMatcher();

    // implement required interface
    void setRegexString(const String& regex);
    const String& getRegexString() const;
    MatchState getMatchStateOfString(const String& str) const;

private:
    //! free the compiled PCRE regex, if any.
    void release();

    //! Copy of the regex string assigned.
    String d_string;
    //! Pointer to PCRE compiled RegEx.
    pcre* d_regex;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIPCRERegexMatcher_h_
