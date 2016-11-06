/***********************************************************************
    created:    28/5/2011
    author:     Martin Preisler

    purpose:    Defines the interface for the Clipboard static class
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
#ifndef _CEGUIClipboard_h_
#define _CEGUIClipboard_h_

#include "CEGUI/String.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    Lightweight interface providing access to the system-wide native clipboard
*/
class CEGUIEXPORT NativeClipboardProvider :
    public AllocatedObject<NativeClipboardProvider>
{
public:
    virtual ~NativeClipboardProvider();

    /*!
    \brief sends given data to native clipboard
    */
    virtual void sendToClipboard(const String& mimeType, void* buffer, size_t size) = 0;

    /*!
    \brief retrieves given data from native clipboard
    */
    virtual void retrieveFromClipboard(String& mimeType, void*& buffer, size_t& size) = 0;
};

/*!
\brief
    Defines a clipboard handling class

\par
    Usually, there is just one instance of this class, owned by CEGUI::System,
    it contains internal CEGUI clipboard that may be (optionally) synchronised
    with native clipboard if user sets NativeClipboardProvider with:
    \code
    CEGUI::System::getSingleton()->getClipboard()->setNativeProvider(customProvider)
    \endcode
    Where customProvider is of course user implemented clipboard provider.
    
\internal
    We may provide native clipboard providers automatically for some platforms
    in the future, however for X11 this is really hard and would introduce
    dependency -lX11 to CEGUIBase which is something we would really hate.
*/
class CEGUIEXPORT Clipboard :
    public AllocatedObject<NativeClipboardProvider>
{
public:
    /*!
    \brief constructor
    */
    Clipboard();

    /*!
    \brief destructor
    */
    ~Clipboard();

    /*!
    \brief sets native clipboard provider
    
    \param provider the native clipboard provider to set
    \see NativeClipboardProvider
    You are required to deallocate given provider, this class doesn't take ownership!)
    */
    void setNativeProvider(NativeClipboardProvider* provider);

    /*!
    \brief retrieves currently set native clipboard provider
    \see NativeClipboardProvider
    */
    NativeClipboardProvider* getNativeProvider() const;

    /*!
    \brief sets contents of this clipboard to given raw data
    
    \param mimeType describes type of the data in the clipboard
    \param buffer raw buffer containing data to push into the clipboard
    \param size size (in bytes) of given data
    */
    void setData(const String& mimeType, const void* buffer, size_t size);

    /*!
    \brief retrieves contents of this clipboard as raw data
    
    \param mimeType current mime type
    \param buffer the raw data buffer (can be 0 if size == 0!)
    \param size size of the returned buffer
    
    You shan't change the buffer contents, only read from it!
    */
    void getData(String& mimeType, const void*& buffer, size_t& size);

    /*!
    \brief convenience method that sets contents to given string
    */
    void setText(const String& text);

    /*!
    \brief convenience method that retrieves contents as a string
    */
    String getText();

private:
    /// mime type of the current content
    String d_mimeType;
    // just implementation specific
    typedef char BufferElement;
    /// raw data buffer containing current clipboard contents
    BufferElement* d_buffer;
    /// size (in bytes) of the raw buffer
    size_t d_bufferSize;

    /// native clipboard provider if any
    NativeClipboardProvider* d_nativeProvider;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIClipboard_h_

