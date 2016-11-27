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

#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <atlbase.h>

ISimpleAudioVolume*
SK_GetVolumeControl (void)
{
  CComPtr <IMMDeviceEnumerator> pDevEnum;
  if (FAILED ((pDevEnum.CoCreateInstance (__uuidof (MMDeviceEnumerator)))))
    return nullptr;

  CComPtr <IMMDevice> pDevice;
  if ( FAILED (
         pDevEnum->GetDefaultAudioEndpoint ( eRender,
                                               eCommunications,
                                                 &pDevice )
              )
     ) return nullptr;

  CComPtr <IAudioSessionManager2> pSessionMgr2;
  if (FAILED (pDevice->Activate (
                __uuidof (IAudioSessionManager2),
                  CLSCTX_ALL,
                    nullptr,
                      reinterpret_cast <void **>(&pSessionMgr2)
             )
         )
     ) return nullptr;

  CComPtr <IAudioSessionEnumerator> pSessionEnum;
  if (FAILED (pSessionMgr2->GetSessionEnumerator (&pSessionEnum)))
    return nullptr;

  int num_sessions;

  if (FAILED (pSessionEnum->GetCount (&num_sessions)))
    return nullptr;

  for (int i = 0; i < num_sessions; i++) {
    CComPtr <IAudioSessionControl> pSessionCtl;
    if (FAILED (pSessionEnum->GetSession (i, &pSessionCtl)))
      return nullptr;

    CComPtr <IAudioSessionControl2> pSessionCtl2;
    if (FAILED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSessionCtl2))))
      return nullptr;

    DWORD dwProcess = 0;
    if (FAILED (pSessionCtl2->GetProcessId (&dwProcess)))
      return nullptr;

    if (dwProcess == GetCurrentProcessId ()) {
      ISimpleAudioVolume* pSimpleAudioVolume;

      if (SUCCEEDED (pSessionCtl->QueryInterface (IID_PPV_ARGS (&pSimpleAudioVolume))))
        return pSimpleAudioVolume;
    }
  }

  return nullptr;
}

void
SK_SetGameMute (BOOL bMute)
{
  ISimpleAudioVolume* pVolume =
    SK_GetVolumeControl ();

  if (pVolume != nullptr) {
    pVolume->SetMute (bMute, nullptr);
    pVolume->Release ();
  }
}