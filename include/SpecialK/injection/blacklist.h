#pragma pack (push,8)
typedef struct _UNICODE_STRING {
  USHORT         Length;
  USHORT         MaximumLength;
  PWSTR          Buffer;
} UNICODE_STRING,
*PUNICODE_STRING;
#pragma pack (pop)

struct constexpr_module_s
{
  size_t         hash;
  UNICODE_STRING str;

  constexpr constexpr_module_s ( const wchar_t* name )
  {
    hash =          hash_lower (name);
    str  = RTL_CONSTANT_STRING (name);
  }

  static constexpr
  bool get ( const std::initializer_list <constexpr_module_s>& modules,
                                                        size_t _hashval )
  {
    for (const auto& module : modules)
    {
      if (module.hash == _hashval)
        return true;
    }

    return
      false;
  }

  using list_type =
    std::initializer_list <constexpr_module_s>;
};

#define SK_MakeUnicode(str) RTL_CONSTANT_STRING(str)

typedef NTSTATUS (NTAPI *LdrGetDllHandleByName_pfn)(
  const PUNICODE_STRING  BaseDllName,
  const PUNICODE_STRING  FullDllName,
        PVOID           *DllHandle
);

// Bluelist = Bounce the DLL out ASAP, do not attempt to defer unload
//
//  * If a game is explicitly whitelisted, it will be allowed to override
//      its presence in this list.
//
static constexpr constexpr_module_s::list_type __bluelist = {
  // Early-out for most CEF-based apps;
  //   Stuff like Spotify otherwise appears to
  //     be a game since it uses D3D shaders
  L"libcef.dll",
};

static constexpr constexpr_module_s::list_type __graylist = {
  L"setup.exe",
  L"vrserver.exe",
  L"jusched.exe",
  L"perfwatson2.exe",
  L"dataexchangehost.exe",
  L"oalinst.exe",
  L"dxsetup.exe",
  L"uninstall.exe",
  L"dotnetfx35.exe",
  L"dotnetfx35client.exe",
  L"easyanticheat_setup.exe",
  L"dotnetfx40_full_x86_x64.exe",
  L"dotnetfx40_client_x86_x64.exe",
  L"ndp451-kb2872776-x86-x64-allos-enu.exe",
  L"identity_helper.exe",
  L"yourphoneserver.exe", // Injects into UWP app that later suspends itself
  L"razer synapse service.exe",

#ifdef _M_IX86
  L"rzsynapse.exe",
  L"msiafterburner.exe",
  L"googlecrashhandler.exe",
  L"crashsender1400.exe",
  L"crashsender1402.exe",
#else
  L"googlecrashhandler64.exe",
#endif

  L"xvasynth.exe",
//L"streaming_client.exe", // Don't blacklist, SK will never auto-activate.
                           //
                           //  To actually use in streaming_client.exe you
                           //  must place SpecialK.dxgi in the same dir.
  L"firaxisbugreporter.exe",
  L"writeminidump.exe",
  L"crashreporter.exe",
  L"supporttool.exe",
  L"ism2.exe",
  L"msmpeng.exe",
};

static constexpr constexpr_module_s::list_type __blacklist = {
#ifdef _M_AMD64
  L"msbuild.exe",
  L"systemsettings.exe",
  L"systemsettingsbroker.exe",
  L"nvcplui.exe",
  L"gamebarft.exe",
  L"gamebarftserver.exe",

  L"vhui64.exe",
  L"x64launcher.exe",
  L"ff9_launcher.exe",
  L"vcredist_x64.exe",
  L"vc_redist.x64.exe",
  L"vc2010redist_x64.exe",
  L"ubisoftgamelauncher64.exe",
  L"sen3launcher.exe",

  L"wallpaper64.exe",
  L"winrtutil64.exe",
  L"diagnostics64.exe",
  L"applicationwallpaperinject64.exe", // Wallpaper Engine's UI app
  L"Reloaded-II.exe",

#else
  L"Reloaded-II32.exe",
  L"vacodeinspectionsserver.exe",

  L"vhui.exe",
  L"x86launcher.exe",
  L"vcredist_x86.exe",
  L"vc_redist.x86.exe",
  L"vc2010redist_x86.exe",
  L"ffxiii2launcher.exe",
  L"ffx&x-2_launcher.exe",
  L"ff7_launcher.exe",
  L"ff8_launcher.exe",
  L"ff9_launcher.exe",
  L"ubisoftgamelauncher.exe",
  L"uplayinstaller.exe",

  L"diagnostics32.exe",
  L"edgewallpaper32.exe",
  L"wallpaper32.exe",
  L"wallpaperservice32.exe",
  L"webwallpaper32.exe",
  L"winrtutil32.exe",
  L"applicationwallpaperinject32.exe",
  L"apputil32.exe",
  L"ui32.exe",               // Wallpaper Engine's stpid UI app

  L"akibauu_config.exe",
  L"gameserver.exe",// Sacred   game server
  L"s2gs.exe",      // Sacred 2 game server

  L"redprelauncher.exe", // CD PROJEKT RED's pre-launcher included in the root of Cyberpunk 2077
  L"redlauncher.exe",    // CD PROJEKT RED's launcher located below %APPDATA%
  L"chronocross_launcher.exe",
  L"wrath-epic.exe",  // 32-bit Launcher  (EGS)
  L"wrath-steam.exe", // 32-bit Launcher  (Steam)

  // OBS Stuff
  L"get-graphics-offsets32.exe",
  L"obs32.exe",
#endif

  L"postcrashdump.exe",

  L"launcher.exe",
  L"launchpad.exe",
  L"launcher_epic.exe", // Genshin Impact
  L"fallout4launcher.exe",
  L"skyrimselauncher.exe",
  L"modlauncher.exe",
  L"obduction.exe",
  L"grandia2launcher.exe",
  L"bethesda.net_launcher.exe",
  L"splashscreen.exe",
  L"gamelaunchercefchildprocess.exe",
  L"dplauncher.exe",
  L"cnnlauncher.exe",
  L"gtavlauncher.exe",
  L"a17config.exe",
  L"a18config.exe", // Atelier Firis
  L"zeroescape-launcher.exe",
  L"gtavlanguageselect.exe",
  L"controllercompanion.exe",
  L"nioh_launcher.exe",
  L"rottlauncher.exe",
  L"configtool.exe",
  L"crs-uploader.exe", // Days Gone Launcher thingy
  L"dndbrowserhelper64.exe",

  L"launcherpatcher.exe",      // GTA IV (32-bit)
  L"rockstarservice.exe",      //  ...
  L"rockstarsteamhelper.exe",  //  ...
  L"gta4Browser.exe",          //  (no idea what this is, but ignore it)
  L"rockstarerrorhandler.exe", // 64-bit even in 32-bit games

  L"easteamproxy.exe",         // Stupid EA bullcrap
  L"link2ea.exe",              // More stupid EA stuff

  L"t2gp.exe",                 // 2K Launcher
  L"launcher_helper.exe",      // More 2K crap

  L"steamless.exe",            // Steam DRM workaround needed to mod some games

  L"zfgamebrowser.exe",        // Genshin Impact Launcher
  L"dsx.exe",                  // Dual Sense X

  L"beamng.drive.exe",         // BeamNG's 32-bit launcher
  L"losslessscaling.exe",      // Lossless Scaling

  L"setup_redlauncher.exe",    // The Witcher 3's Launcher

  L"cefsharp.browsersubprocess.exe", // Baldur's Gate 3
  L"larilauncher.exe",
  L"idtechlauncher.exe",
  L"gamelaunchhelper.exe",

  L"coherentui_host.exe",
  L"activationui.exe",
  L"zossteamstarter.exe",
  L"eac.exe",
  L"ealink.exe",

  L"dock.exe",
  L"dock_64.exe",
  L"dockmod.exe",
  L"dockmod64.exe",

  L"crashpad_handler.exe",
  L"crashpad_handler.dll", // WTF? Dave the Diver runs this DLL as an exe
  L"clang-tidy.exe",
  L"clupdater.exe",
  L"activate.exe",
  L"werfault.exe",
  L"x64dbg.exe",
  L"locationnotificationwindows.exe",
  L"servicehub.datawarehousehost.exe",
  L"launchtm.exe",
  L"displayhdrcompliancetests.exe",
  L"scriptedsandbox64.exe",
  L"crashreportclient.exe",
  L"blizzarderror.exe",
  L"crs-handler.exe",

  L"gamebar.exe",
  L"gamingservicesui.exe",
  L"pwahelper.exe",

  // Does not reply to DLL unload requests
  L"mspaint.exe",
  L"notepad.exe",
  L"explorer.exe",
  L"prevhost.exe",

  L"windowsterminal.exe",
  L"cmd.exe",
  L"steam.exe",
  L"openconsole.exe",
  L"epicwebhelper.exe",
  L"steamwebhelper.exe",
  L"epicgameslauncher.exe",
  L"galaxyclient helper.exe",


  L"applicationframehost.exe",
  L"servicehub.host.clr.x86.exe",
  L"servicehub.settingshost.exe",
  L"servicehub.identityhost.exe",
  L"servicehub.threadedwaitdialog.exe",
  L"nvidia web helper.exe",
  L"tobii.eyex.engine.exe",
  L"esrv.exe",
  L"ipoint.exe",
  L"itype.exe",
  L"vsgraphics.exe",
  L"powershell.exe",
  L"devenv.exe",

  // OBS Stuff
  L"obs-browser-page.exe",
  L"get-graphics-offsets64.exe",
  L"obs64.exe"
};