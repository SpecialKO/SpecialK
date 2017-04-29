/*
The MIT License (MIT)

Copyright (c) 2016 Andre Leiradella

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

namespace ImGuiAl
{
  class MsgBox
  {
  public:
    inline MsgBox() {}
    virtual ~MsgBox();
    
    bool Init( const char* title, const char* icon, const char* text, const char** captions, bool show_checkbox = false );
    int  Draw();
    void Open();
    
    inline void AskAgain()
    {
      m_DontAskAgain = false;
      m_Selected = 0;
    }
    
  protected:
    const char* m_Title;
    const char* m_Icon;
    const char* m_Text;
    const char** m_Captions;
    bool m_ShowCheckbox;
    bool m_DontAskAgain;
    int m_Selected;
  };
}