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

#include <SpecialK/stdafx.h>

#include <hidclass.h>
#include <bluetoothapis.h>
#include <bthdef.h>
#include <bthioctl.h>

#pragma comment (lib, "bthprops.lib")

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Blue Tooth"

void SK_Bluetooth_InitPowerMgmt (void)
{
  class SK_Bluetooth_PowerListener : public SK_IVariableListener
  {
  public:
    SK_Bluetooth_PowerListener (void)
    {
      SK_GetCommandProcessor ()->AddVariable (
        "Input.Gamepad.PowerOff", SK_CreateVar (SK_IVariable::LongInt, &hwaddr, this)
      );
    }

    bool OnVarChange (SK_IVariable* var, void* val = nullptr)
    {
      if (var == nullptr) return false;

      if (var->getValuePointer () == &hwaddr)
      {
        std::scoped_lock <std::mutex> _(mutex);

        int powered_off = 0;

        // For proper operation, this should always be serialized onto a single
        //   thread; in other words, use SK_DeferCommand (...) to change this.
        for ( auto& device : SK_HID_PlayStationControllers )
        {
          if (device.bBluetooth && device.bConnected)
          {
            const bool bRequesting =
             (((device.xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
               (device.xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_Y))    ||
              ((device.xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
               (device.xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_Y)));

            // The serial number is actually the Bluetooth MAC address; handy...
            wchar_t wszSerialNumber [32] = { };
 
            DWORD dwBytesReturned = 0;
 
            SK_DeviceIoControl (
              device.hDeviceFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
                                                                     wszSerialNumber, 64,
                                                                    &dwBytesReturned, nullptr );

            ULONGLONG                           ullHWAddr = 0x0;
            swscanf (wszSerialNumber, L"%llx", &ullHWAddr);

            if (bRequesting || (val != nullptr && *(uint64*)val != 1 && ullHWAddr == *(uint64*)val))
            {
              SK_LOGi0 ( L"Attempting to disconnect Bluetooth MAC Address: %ws (%llu)",
                           wszSerialNumber, ullHWAddr );

              BLUETOOTH_FIND_RADIO_PARAMS findParams =
                { .dwSize = sizeof (BLUETOOTH_FIND_RADIO_PARAMS) };

              HANDLE hBtRadio    = INVALID_HANDLE_VALUE;
              HANDLE hFindRadios = INVALID_HANDLE_VALUE;

              BluetoothFindFirstRadio (&findParams, &hBtRadio);

              if ((intptr_t)hFindRadios > 0 && (intptr_t)hBtRadio > 0)
              {
                BOOL   success  = FALSE;
                while (success == FALSE && hBtRadio != 0)
                {
                  success =
                    SK_DeviceIoControl (
                      hBtRadio, IOCTL_BTH_DISCONNECT_DEVICE, &ullHWAddr, 8,
                                                             nullptr,    0,
                                           &dwBytesReturned, nullptr );

                  CloseHandle (hBtRadio);

                  if (! success)
                    if (! BluetoothFindNextRadio (hFindRadios, &hBtRadio))
                      hBtRadio = 0;

                  if (success)
                    ++powered_off;
                }

                BluetoothFindRadioClose (hFindRadios);
              }
            }
          }
        }

        // Nothing was requesting a power-off, so now power everything off wholesale!
        if (powered_off == 0)
        {
          for ( auto& device : SK_HID_PlayStationControllers )
          {
            if (device.bBluetooth && device.bConnected)
            { 
              // The serial number is actually the Bluetooth MAC address; handy...
              wchar_t wszSerialNumber [32] = { };
 
              DWORD dwBytesReturned = 0;
 
              SK_DeviceIoControl (
                device.hDeviceFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
                                                                       wszSerialNumber, 64,
                                                                      &dwBytesReturned, nullptr );

              ULONGLONG                           ullHWAddr = 0x0;
              swscanf (wszSerialNumber, L"%llx", &ullHWAddr);

              if (val != nullptr && (*(uint64 *)(val) == 1 || ullHWAddr == *(uint64*)(val)))
              {
                SK_LOGi0 ( L"Attempting to disconnect Bluetooth MAC Address: %ws (%llu)",
                             wszSerialNumber, ullHWAddr );

                BLUETOOTH_FIND_RADIO_PARAMS findParams =
                  { .dwSize = sizeof (BLUETOOTH_FIND_RADIO_PARAMS) };

                HANDLE hBtRadio    = INVALID_HANDLE_VALUE;
                HANDLE hFindRadios =
                  BluetoothFindFirstRadio (&findParams, &hBtRadio);

                if (hFindRadios != 0 && hFindRadios != INVALID_HANDLE_VALUE)
                {
                  BOOL   success  = FALSE;
                  while (success == FALSE && hBtRadio != 0 && hBtRadio != INVALID_HANDLE_VALUE)
                  {
                    success =
                      SK_DeviceIoControl (
                        hBtRadio, IOCTL_BTH_DISCONNECT_DEVICE, &ullHWAddr, 8,
                                                               nullptr,    0,
                                             &dwBytesReturned, nullptr );

                    CloseHandle (hBtRadio);

                    if (! success)
                      if (! BluetoothFindNextRadio (hFindRadios, &hBtRadio))
                        hBtRadio = 0;

                    if (success)
                      ++powered_off;
                  }

                  BluetoothFindRadioClose (hFindRadios);
                }
              }
            }
          }
        }
      }

      if (val != nullptr)
        *((uint64 *)(var->getValuePointer ())) = *(uint64 *)(val);

      return true;
    }

    std::mutex mutex;
    uint64     hwaddr;
  } static power;
}