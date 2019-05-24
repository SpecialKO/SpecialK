/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __SK__OSD_TEXT_H__
#define __SK__OSD_TEXT_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <Windows.h>
#include <stdint.h>
#include <string>
#include <SpecialK/command.h>

LPVOID __stdcall SK_GetSharedMemory     (void);
BOOL   __stdcall SK_ReleaseSharedMemory (LPVOID pMemory);

void __stdcall SK_InstallOSD       (void);
BOOL __stdcall SK_DrawOSD          (void);
BOOL __stdcall SK_DrawExternalOSD  (std::string app_name, std::string text);
BOOL __stdcall SK_UpdateOSD        (LPCSTR lpText, LPVOID pMapAddr = nullptr, LPCSTR lpAppName = nullptr);
void __stdcall SK_ReleaseOSD       (void);

void __stdcall SK_SetOSDPos        (int x,   int y,                           LPCSTR lpAppName = nullptr);

// Any value out of range: [0,255] means IGNORE that color
void __stdcall SK_SetOSDColor      (int red, int green, int blue,             LPCSTR lpAppName = nullptr);

void __stdcall SK_SetOSDScale      (float fScale, bool relative = false,      LPCSTR lpAppName = nullptr);
void __stdcall SK_ResizeOSD        (float scale_incr,                         LPCSTR lpAppName = nullptr);

void __stdcall SK_OSD_GetDefaultColor (float& r,  float& g,  float& b) noexcept;

#include <map>

namespace CEGUI {
  class Renderer;
  class GUIContext;
  class Font;
  class GeometryBuffer;
}

class SK_TextOverlay
{
friend class SK_TextOverlayManager;

public:
  ~SK_TextOverlay (void);

  float update    (const char* szText);

  float draw      (float x = 0.0f, float y = 0.0f, bool full = false);
  void  reset     (CEGUI::Renderer* renderer);

  void  resize    (float incr)                 noexcept;
  void  setScale  (float scale)                noexcept;
  float getScale  (void)                       noexcept;

  void  move      (float  x_off, float  y_off) noexcept;
  void  setPos    (float  x,     float  y)     noexcept;
  void  getPos    (float& x,     float& y)     noexcept;

  char* getName   (void) noexcept { return data_.name; }
  char* getText   (void) noexcept { return data_.text; }

protected:
   SK_TextOverlay (const char* szAppName);

private:
  struct
  {
    char   name [64] = { };

    char*  text      = nullptr; // UTF-8
    size_t text_len  = 0;
    float  extent    = 0.0f;// Rendered height, in pixels
  } data_;

  struct
  {
    CEGUI::Font*
           cegui     = nullptr;

    char   name [64] = { };
    float  scale     = 1.0f;
    DWORD  primary_color = 0xFFFFFFFF; // For text that doesn't use its own
    DWORD  shadow_color  = 0x0;
  } font_;

public:
  CEGUI::GeometryBuffer*
           geometry_    = nullptr;
  CEGUI::Renderer*
           renderer_    = nullptr;;

  struct {
    float  x = 0.0f,
           y = 0.0f;
  } pos_;
};

class SK_TextOverlayManager : public SK_IVariableListener
{
private: // Singleton
  static                  CRITICAL_SECTION       cs_;

public:
  static std::unique_ptr <SK_TextOverlayManager> pSelf;
  static SK_TextOverlayManager*                  getInstance (void);

  SK_TextOverlay* createTextOverlay (const char* szAppName);
  bool            removeTextOverlay (const char* szAppName);
  SK_TextOverlay* getTextOverlay    (const char* szAppName);


  void            queueReset         (CEGUI::Renderer* renderer);

  void            resetAllOverlays   (CEGUI::Renderer* renderer);
  float           drawAllOverlays    (float x, float y, bool full = false);
  void            destroyAllOverlays (void);


  virtual
    ~SK_TextOverlayManager (void) { };

//protected:
  SK_TextOverlayManager (void);

private:
  bool                                     need_full_reset_ = true;
  CEGUI::GUIContext*                       gui_ctx_         = nullptr;
  std::map <std::string, SK_TextOverlay *> overlays_;

  struct {
    SK_IVariable*                          x      = nullptr;
    SK_IVariable*                          y      = nullptr;
  } pos_;

  SK_IVariable*                            scale_ = nullptr;

  struct
  {
    SK_IVariable*                          show   = nullptr;
  } osd_, fps_, gpu_, disk_, pagefile_,
          mem_, cpu_, io_,   clock_,
          sli_;

public:
  bool OnVarChange (SK_IVariable* var, void* val = nullptr) override;
};

#endif /* __SK__OSD_TEXT_H__ */