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

#include <SpecialK/widgets/widget.h>

#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <SpecialK/control_panel.h>

extern iSK_INI* osd_ini;

SK_DXGI_HDRControl*
SK_HDR_GetControl (void)
{
  static SK_DXGI_HDRControl hdr_ctl = { };
  return                   &hdr_ctl;
}

class SKWG_HDR_Control : public SK_Widget
{
public:
  SKWG_HDR_Control (void) : SK_Widget ("DXGI_HDR")
  {
    SK_ImGui_Widgets.hdr_control = this;

    setAutoFit (true).setDockingPoint (DockAnchor::NorthEast).setClickThrough (false);
  };

  virtual void run (void) override
  {
  }

  virtual void draw (void) override
  {
    if (! ImGui::GetFont ()) return;

    SK_DXGI_HDRControl* pHDRCtl =
      SK_HDR_GetControl ();

    ImGui::Checkbox ("###HDR_Override_MinMasterLevel", &pHDRCtl->overrides.MinMaster); ImGui::SameLine ();
    float fMinMaster = (float)pHDRCtl->meta.MinMasteringLuminance / 10000.0f;
    if (ImGui::SliderFloat ("Minimum Luminance", &fMinMaster, pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    {
      if (pHDRCtl->overrides.MinMaster)
      {
        pHDRCtl->meta.MinMasteringLuminance = (UINT)(fMinMaster * 10000);
      }
    }

    ImGui::Checkbox ("###HDR_Override_MaxMasterLevel", &pHDRCtl->overrides.MaxMaster); ImGui::SameLine ();
    float fMaxMaster = (float)pHDRCtl->meta.MaxMasteringLuminance / 10000.0f;
    if (ImGui::SliderFloat ("Maximum Luminance", &fMaxMaster, pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    {
      if (pHDRCtl->overrides.MaxMaster)
      {
        pHDRCtl->meta.MaxMasteringLuminance = (UINT)(fMaxMaster * 10000);
      }
    }

    ImGui::Separator ();

    ImGui::Checkbox ("###HDR_Override_MaxContentLevel", &pHDRCtl->overrides.MaxContentLightLevel); ImGui::SameLine ();
    float fBrightest = (float)pHDRCtl->meta.MaxContentLightLevel;
    if (ImGui::SliderFloat ("Max. Content Light Level (nits)",       &fBrightest,          pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    {
      if (pHDRCtl->overrides.MaxContentLightLevel)
      {
        pHDRCtl->meta.MaxContentLightLevel = (UINT16)fBrightest;
      }
    }

    ImGui::Checkbox ("###HDR_Override_MaxFrameAverageLightLevel", &pHDRCtl->overrides.MaxFrameAverageLightLevel); ImGui::SameLine ();
    float fBrightestLastFrame = (float)pHDRCtl->meta.MaxFrameAverageLightLevel;
    if (ImGui::SliderFloat ("Max. Frame Average Light Level (nits)", &fBrightestLastFrame, pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    {
      if (pHDRCtl->overrides.MaxFrameAverageLightLevel)
      {
        pHDRCtl->meta.MaxFrameAverageLightLevel = (UINT16)fBrightestLastFrame;
      }
    }

    ImGui::TreePush   ("");
    ImGui::BulletText ("Game has adjusted HDR Metadata %lu times...", pHDRCtl->meta._AdjustmentCount);
    ImGui::TreePop    (  );
  }

  virtual void OnConfig (ConfigEvent event) override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:

private:
} __dxgi_hdr__;