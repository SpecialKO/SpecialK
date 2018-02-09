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

#ifndef __SK__INJECTION_BLACKLIST_H__
#define __SK__INJECTION_BLACKLIST_H__

#include <unordered_set>
#include <string>

static const std::unordered_set <std::wstring>
__blacklist = {
  L"steam.exe",
  L"gameoverlayui.exe",
  L"streaming_client.exe",
  L"steamerrorreporter.exe",
  L"steamerrorreporter64.exe",
  L"steamservice.exe",
  L"steam_monitor.exe",
  L"steamwebhelper.exe",
  L"html5app_steam.exe",
  L"wow_helper.exe",
  L"uninstall.exe",
  
  L"writeminidump.exe",
  L"crashreporter.exe",
  L"supporttool.exe",
  L"crashsender1400.exe",
  L"werfault.exe",
  
  L"dxsetup.exe",
  L"setup.exe",
  L"vc_redist.x64.exe",
  L"vc_redist.x86.exe",
  L"vc2010redist_x64.exe",
  L"vc2010redist_x86.exe",
  L"vcredist_x64.exe",
  L"vcredist_x86.exe",
  L"ndp451-kb2872776-x86-x64-allos-enu.exe",
  L"dotnetfx35.exe",
  L"dotnetfx35client.exe",
  L"dotnetfx40_full_x86_x64.exe",
  L"dotnetfx40_client_x86_x64.exe",
  L"oalinst.exe",
  L"easyanticheat_setup.exe",
  L"uplayinstaller.exe",
  
  L"x64launcher.exe",
  L"x86launcher.exe",
  L"launcher.exe",
  L"ffx&x-2_launcher.exe",
  L"fallout4launcher.exe",
  L"skyrimselauncher.exe",
  L"modlauncher.exe",
  L"akibauu_config.exe",
  L"obduction.exe",
  L"grandia2launcher.exe",
  L"ffxiii2launcher.exe",
  L"bethesda.net_launcher.exe",
  L"ubisoftgamelauncher.exe",
  L"ubisoftgamelauncher64.exe",
  L"splashscreen.exe",
  L"gamelaunchercefchildprocess.exe",
  L"launchpad.exe",
  L"cnnlauncher.exe",
  L"ff9_launcher.exe",
  L"a17config.exe",
  L"a18config.exe", // Atelier Firis
  L"dplauncher.exe",
  L"zeroescape-launcher.exe",
  L"gtavlauncher.exe",
  L"gtavlanguageselect.exe",
  L"nioh_launcher.exe",
  L"rottlauncher.exe",
  L"configtool.exe",

  L"coherentui_host.exe",
  L"activationui.exe",
  L"zossteamstarter.exe",
  L"notepad.exe",
  L"mspaint.exe",
  L"7zfm.exe",
  L"winrar.exe",
  L"eac.exe",
  L"vcpkgsrv.exe",
  L"dllhost.exe",
  L"git.exe",
  L"link.exe",
  L"cl.exe",
  L"rc.exe",
  L"conhost.exe",
  L"gamebarpresencewriter.exe",
  L"oawrapper.exe",
  L"nvoawrappercache.exe",
  L"perfwatson2.exe",
  
  L"gameserver.exe",// Sacred   game server
  L"s2gs.exe",      // Sacred 2 game server
  
  L"sihost.exe",
  L"chrome.exe",
  L"explorer.exe",
  L"browser_broker.exe",
  L"dwm.exe",
  L"launchtm.exe",
  
  L"ds3t.exe",
  L"tzt.exe"
};

#endif /* __SK__INJECTION_BLACKLIST_H__ */