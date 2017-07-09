#if 0
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

#include <imgui/widgets/msgbox.h>
#include <imgui/imgui.h>

ImGuiAl::MsgBox::~MsgBox() {}

bool ImGuiAl::MsgBox::Init( const char* title, const char* icon, const char* text, const char** captions, bool show_checkbox )
{
  m_Title = title;
  m_Icon = icon;
  m_Text = text;
  m_Captions = captions;
  m_ShowCheckbox = show_checkbox;
  m_DontAskAgain = false;
  m_Selected = 0;
  return true;
}

int ImGuiAl::MsgBox::Draw()
{
  int index = 0;
  
  if ( ImGui::BeginPopupModal( m_Title, NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    if ( m_DontAskAgain && m_Selected != 0 )
    {
      ImGui::CloseCurrentPopup();
      index = m_Selected;
    }
    else
    {
      if ( m_Icon != NULL )
      {
        ImVec2 size = ImGui::CalcTextSize( m_Icon );
        ImVec2 pos = ImGui::GetCursorPos();
        float save_y = pos.y;
        pos.y += size.y;
        
        ImGui::SetCursorPos( pos );
        ImGui::Text( "%s", m_Icon );
        
        pos.x += size.x + pos.x;
        pos.y = save_y;
        
        ImGui::SetCursorPos( pos );
        ImGui::TextWrapped( "%s", m_Text );
      }
      else
      {
        ImGui::TextWrapped( "%s", m_Text );
      }
      
      ImGui::Separator();
      
      if ( m_ShowCheckbox )
      {
        ImGui::Checkbox( "Don't ask me again", &m_DontAskAgain );
      }
      
      ImVec2 size = ImVec2( 50.0f, 0.0f );
      int count;
      
      for ( count = 0; m_Captions[ count ] != NULL; count++ )
      {
        if ( ImGui::Button( m_Captions[ count ], size ) )
        {
          index = count + 1;
          ImGui::CloseCurrentPopup();
          break;
        }
        
        ImGui::SameLine();
      }
      
      size = ImVec2( ( 4 - count ) * 50.0f, 1.0f );
      ImGui::Dummy( size );
      
      if ( m_DontAskAgain )
      {
        m_Selected = index;
      }
    }
    
    ImGui::EndPopup();
  }
  
  return index;
}

void ImGuiAl::MsgBox::Open()
{
  ImGui::OpenPopup( m_Title );
}
#endif