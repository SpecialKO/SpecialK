#define SK_MakeUnicode(str) RTL_CONSTANT_STRING(str)

#pragma pack (push,8)
typedef struct _UNICODE_STRING {
  USHORT         Length;
  USHORT         MaximumLength;
  PWSTR          Buffer;
} UNICODE_STRING,
*PUNICODE_STRING;
#pragma pack (pop)

typedef NTSTATUS (NTAPI* LdrGetDllHandle_pfn)(
  _In_opt_    PWORD           pwPath,
  _In_opt_    PVOID           Unused,
  _In_  const UNICODE_STRING *ModuleFileName,
  _Out_       PHANDLE         pHModule
);

typedef NTSTATUS (NTAPI *LdrGetDllHandleByName_pfn)(
  const PUNICODE_STRING  BaseDllName,
  const PUNICODE_STRING  FullDllName,
        PVOID           *DllHandle
);

#if 0
// Bluelist = Bounce the DLL out ASAP, do not attempt to defer unload
static constexpr UNICODE_STRING __bluelist [] = {
  SK_MakeUnicode (L"node.exe"), // Node.Js
  SK_MakeUnicode (L"setup.exe"),
  SK_MakeUnicode (L"libcef.dll"),
  SK_MakeUnicode (L"oalinst.exe"),
  SK_MakeUnicode (L"dxsetup.exe"),
  SK_MakeUnicode (L"uninstall.exe"),
  SK_MakeUnicode (L"dotnetfx35.exe"),
  SK_MakeUnicode (L"perfwatson2.exe"),
  SK_MakeUnicode (L"dotnetfx35client.exe"),
  SK_MakeUnicode (L"easyanticheat_setup.exe"),
  SK_MakeUnicode (L"dotnetfx40_full_x86_x64.exe"),
  SK_MakeUnicode (L"dotnetfx40_client_x86_x64.exe"),
  SK_MakeUnicode (L"ndp451-kb2872776-x86-x64-allos-enu.exe"),
  SK_MakeUnicode (L"applicationframehost.exe"),
  SK_MakeUnicode (L"MagnetEngine.exe"),
  SK_MakeUnicode (L"gamebarftserver.exe"),
  SK_MakeUnicode (L"gamebarft.exe"),
  SK_MakeUnicode (L"gamebar.exe"),
  SK_MakeUnicode (L"spotify.exe"),
  SK_MakeUnicode (L"jusched.exe"),
  SK_MakeUnicode (L"conhost.exe"),
  SK_MakeUnicode (L"dllhost.exe"),
  SK_MakeUnicode (L"dllhst3g.exe"),
  SK_MakeUnicode (L"identity_helper.exe"),
};

static constexpr UNICODE_STRING __graylist [] = {
#ifdef _M_IX86
  SK_MakeUnicode (L"devenv.exe"),
  SK_MakeUnicode (L"rzsynapse.exe"),
  SK_MakeUnicode (L"msiafterburner.exe"),
  SK_MakeUnicode (L"onedrive.exe"),
  SK_MakeUnicode (L"scriptedsandbox.exe"),
  SK_MakeUnicode (L"googlecrashhandler.exe"),
  SK_MakeUnicode (L"servicehub.settingshost.exe"),
  SK_MakeUnicode (L"servicehub.host.clr.x86.exe"),
  SK_MakeUnicode (L"servicehub.identityhost.exe"),
  SK_MakeUnicode (L"servicehub.vsdetouredhost.exe"),
  SK_MakeUnicode (L"steelseriesengine3client.exe"),
  SK_MakeUnicode (L"amazon music.exe"),
  SK_MakeUnicode (L"icloudphotos.exe"),
  SK_MakeUnicode (L"icloudservices.exe"),
  SK_MakeUnicode (L"applefirefoxhost.exe"),
  SK_MakeUnicode (L"applephotostreams.exe"),
  SK_MakeUnicode (L"samsungmagician.exe"),
  SK_MakeUnicode (L"lightshot.exe"),
  SK_MakeUnicode (L"acrotray.exe"),
  SK_MakeUnicode (L"sdtray.exe"),
  SK_MakeUnicode (L"secd.exe"),
  SK_MakeUnicode (L"crashsender1400.exe"),
  SK_MakeUnicode (L"crashsender1402.exe"),
  SK_MakeUnicode (L"wallpaper32.exe"),
#else
  SK_MakeUnicode (L"dataexchangehost.exe"),
  SK_MakeUnicode (L"microsoft.servicehub.controller.exe"),
  SK_MakeUnicode (L"googlecrashhandler64.exe"),
  SK_MakeUnicode (L"shellexperiencehost.exe"),
  SK_MakeUnicode (L"skypebackgroundhost.exe"),
  SK_MakeUnicode (L"steelseriesengine3.exe"),
  SK_MakeUnicode (L"scriptedsandbox64.exe"),
  SK_MakeUnicode (L"settingsynchost.exe"),
  SK_MakeUnicode (L"systemsettings.exe"),
  SK_MakeUnicode (L"browser_broker.exe"),
  SK_MakeUnicode (L"runtimebroker.exe"),
  SK_MakeUnicode (L"ituneshelper.exe"),
  SK_MakeUnicode (L"skypebridge.exe"),
  SK_MakeUnicode (L"wallpaper64.exe"),
  SK_MakeUnicode (L"wmiprvse.exe"),
  SK_MakeUnicode (L"sqlservr.exe"),
#endif

  SK_MakeUnicode (L"aurasupportservice.exe"),
  SK_MakeUnicode (L"streaming_client.exe"),
  SK_MakeUnicode (L"wow_helper.exe"),
  SK_MakeUnicode (L"firaxisbugreporter.exe"),
  SK_MakeUnicode (L"writeminidump.exe"),
  SK_MakeUnicode (L"crashreporter.exe"),
  SK_MakeUnicode (L"supporttool.exe"),
  SK_MakeUnicode (L"ism2.exe"),
  SK_MakeUnicode (L"msmpeng.exe"),
};

static constexpr UNICODE_STRING __blacklist [] = {
#ifdef _M_AMD64
  SK_MakeUnicode (L"vhui64.exe"),
  SK_MakeUnicode (L"x64launcher.exe"),
  SK_MakeUnicode (L"ff9_launcher.exe"),
  SK_MakeUnicode (L"vcredist_x64.exe"),
  SK_MakeUnicode (L"vc_redist.x64.exe"),
  SK_MakeUnicode (L"vc2010redist_x64.exe"),
  SK_MakeUnicode (L"ubisoftgamelauncher64.exe"),
  SK_MakeUnicode (L"sen3launcher.exe"),

  ////SK_MakeUnicode (L"control.exe"),
#else
  SK_MakeUnicode (L"vacodeinspectionsserver.exe"),

  SK_MakeUnicode (L"vhui.exe"),
  SK_MakeUnicode (L"x86launcher.exe"),
  SK_MakeUnicode (L"vcredist_x86.exe"),
  SK_MakeUnicode (L"vc_redist.x86.exe"),
  SK_MakeUnicode (L"vc2010redist_x86.exe"),
  SK_MakeUnicode (L"ffxiii2launcher.exe"),
  SK_MakeUnicode (L"ffx&x-2_launcher.exe"),
  SK_MakeUnicode (L"ubisoftgamelauncher.exe"),
  SK_MakeUnicode (L"uplayinstaller.exe"),

  SK_MakeUnicode (L"akibauu_config.exe"),
  SK_MakeUnicode (L"gameserver.exe"),// Sacred   game server
  SK_MakeUnicode (L"s2gs.exe"),      // Sacred 2 game server

  SK_MakeUnicode (L"redprelauncher.exe"), // CD PROJEKT RED's pre-launcher included in the root of Cyberpunk 2077
  SK_MakeUnicode (L"redlauncher.exe"),    // CD PROJEKT RED's launcher located below %APPDATA%
#endif

  SK_MakeUnicode (L"sihost.exe"),
  SK_MakeUnicode (L"postcrashdump.exe"),

  //SK_MakeUnicode (L"launcher.exe"),
  SK_MakeUnicode (L"launchpad.exe"),
  SK_MakeUnicode (L"fallout4launcher.exe"),
  SK_MakeUnicode (L"skyrimselauncher.exe"),
  SK_MakeUnicode (L"modlauncher.exe"),
  SK_MakeUnicode (L"obduction.exe"),
  SK_MakeUnicode (L"grandia2launcher.exe"),
  SK_MakeUnicode (L"bethesda.net_launcher.exe"),
  SK_MakeUnicode (L"splashscreen.exe"),
  SK_MakeUnicode (L"gamelaunchercefchildprocess.exe"),
  SK_MakeUnicode (L"dplauncher.exe"),
  SK_MakeUnicode (L"cnnlauncher.exe"),
  SK_MakeUnicode (L"gtavlauncher.exe"),
  SK_MakeUnicode (L"a17config.exe"),
  SK_MakeUnicode (L"a18config.exe"), // Atelier Firis
  SK_MakeUnicode (L"zeroescape-launcher.exe"),
  SK_MakeUnicode (L"gtavlanguageselect.exe"),
  SK_MakeUnicode (L"controllercompanion.exe"),
  SK_MakeUnicode (L"nioh_launcher.exe"),
  SK_MakeUnicode (L"rottlauncher.exe"),
  SK_MakeUnicode (L"configtool.exe"),

  SK_MakeUnicode (L"coherentui_host.exe"),
  SK_MakeUnicode (L"activationui.exe"),
  SK_MakeUnicode (L"zossteamstarter.exe"),
//SK_MakeUnicode (L"eac.exe"),
  SK_MakeUnicode (L"ealink.exe"),

  SK_MakeUnicode (L"clupdater.exe"),
  SK_MakeUnicode (L"activate.exe"),
  SK_MakeUnicode (L"clmpsvc.exe"),
  SK_MakeUnicode (L"clmshardwaretranscode.exe"),

  SK_MakeUnicode (L"olrstatecheck.exe"),
  SK_MakeUnicode (L"olrsubmission.exe"),

  SK_MakeUnicode (L"steamtours.exe"),
  SK_MakeUnicode (L"vrcompositor.exe"),
  SK_MakeUnicode (L"vrdashboard.exe"),
  SK_MakeUnicode (L"vrmonitor.exe"),
  SK_MakeUnicode (L"vrserver.exe"),
  SK_MakeUnicode (L"vrservice.exe"),
  SK_MakeUnicode (L"vrstartup.exe"),
  SK_MakeUnicode (L"vrwebhelper.exe"),
  SK_MakeUnicode (L"vrserverhelper.exe"),
  SK_MakeUnicode (L"vrpathreg.exe"),
  SK_MakeUnicode (L"localbridge.exe"),
  SK_MakeUnicode (L"xboxappservices.exe"),
  SK_MakeUnicode (L"win32bridge.server.exe"),
};
#else
// Bluelist = Bounce the DLL out ASAP, do not attempt to defer unload
static constexpr UNICODE_STRING __bluelist [] = {
  SK_MakeUnicode (L"setup.exe"),
  SK_MakeUnicode (L"cl.exe"),
  SK_MakeUnicode (L"gamebarft.exe"),
  SK_MakeUnicode (L"gamebarftserver.exe"),
  SK_MakeUnicode (L"msbuild.exe"),
  SK_MakeUnicode (L"vrserver.exe"),
  SK_MakeUnicode (L"jusched.exe"),
  SK_MakeUnicode (L"systemsettings.exe"),
  SK_MakeUnicode (L"nvcplui.exe"),
  SK_MakeUnicode (L"perfwatson2.exe"),
  SK_MakeUnicode (L"dataexchangehost.exe"),
  SK_MakeUnicode (L"oalinst.exe"),
  SK_MakeUnicode (L"dxsetup.exe"),
  SK_MakeUnicode (L"uninstall.exe"),
  SK_MakeUnicode (L"dotnetfx35.exe"),
  SK_MakeUnicode (L"dotnetfx35client.exe"),
  SK_MakeUnicode (L"easyanticheat_setup.exe"),
  SK_MakeUnicode (L"dotnetfx40_full_x86_x64.exe"),
  SK_MakeUnicode (L"dotnetfx40_client_x86_x64.exe"),
  SK_MakeUnicode (L"ndp451-kb2872776-x86-x64-allos-enu.exe"),
  SK_MakeUnicode (L"identity_helper.exe"),
  SK_MakeUnicode (L"yourphoneserver.exe"), // Injects into UWP app that later suspends itself
  SK_MakeUnicode (L"razer synapse service.exe"),
};

static constexpr UNICODE_STRING __graylist [] = {
#ifdef _M_IX86
  SK_MakeUnicode (L"rzsynapse.exe"),
  SK_MakeUnicode (L"msiafterburner.exe"),
  SK_MakeUnicode (L"googlecrashhandler.exe"),
  SK_MakeUnicode (L"crashsender1400.exe"),
  SK_MakeUnicode (L"crashsender1402.exe"),
  SK_MakeUnicode (L"wallpaper32.exe"),
#else
  SK_MakeUnicode (L"googlecrashhandler64.exe"),
  SK_MakeUnicode (L"wallpaper64.exe"),
#endif

  SK_MakeUnicode (L"streaming_client.exe"),
  SK_MakeUnicode (L"firaxisbugreporter.exe"),
  SK_MakeUnicode (L"writeminidump.exe"),
  SK_MakeUnicode (L"crashreporter.exe"),
  SK_MakeUnicode (L"supporttool.exe"),
  SK_MakeUnicode (L"ism2.exe"),
  SK_MakeUnicode (L"msmpeng.exe"),
};

static constexpr UNICODE_STRING __blacklist [] = {
#ifdef _M_AMD64
  SK_MakeUnicode (L"vhui64.exe"),
  SK_MakeUnicode (L"x64launcher.exe"),
  SK_MakeUnicode (L"ff9_launcher.exe"),
  SK_MakeUnicode (L"vcredist_x64.exe"),
  SK_MakeUnicode (L"vc_redist.x64.exe"),
  SK_MakeUnicode (L"vc2010redist_x64.exe"),
  SK_MakeUnicode (L"ubisoftgamelauncher64.exe"),
  SK_MakeUnicode (L"sen3launcher.exe"),
#else
  SK_MakeUnicode (L"vacodeinspectionsserver.exe"),

  SK_MakeUnicode (L"vhui.exe"),
  SK_MakeUnicode (L"x86launcher.exe"),
  SK_MakeUnicode (L"vcredist_x86.exe"),
  SK_MakeUnicode (L"vc_redist.x86.exe"),
  SK_MakeUnicode (L"vc2010redist_x86.exe"),
  SK_MakeUnicode (L"ffxiii2launcher.exe"),
  SK_MakeUnicode (L"ffx&x-2_launcher.exe"),
  SK_MakeUnicode (L"ubisoftgamelauncher.exe"),
  SK_MakeUnicode (L"uplayinstaller.exe"),

  SK_MakeUnicode (L"akibauu_config.exe"),
  SK_MakeUnicode (L"gameserver.exe"),// Sacred   game server
  SK_MakeUnicode (L"s2gs.exe"),      // Sacred 2 game server

  SK_MakeUnicode (L"redprelauncher.exe"), // CD PROJEKT RED's pre-launcher included in the root of Cyberpunk 2077
  SK_MakeUnicode (L"redlauncher.exe"),    // CD PROJEKT RED's launcher located below %APPDATA%
  SK_MakeUnicode (L"chronocross_launcher.exe"),
#endif

  SK_MakeUnicode (L"postcrashdump.exe"),

  SK_MakeUnicode (L"launcher.exe"),
  SK_MakeUnicode (L"launchpad.exe"),
  SK_MakeUnicode (L"launcher_epic.exe"), // Genshin Impact
  SK_MakeUnicode (L"fallout4launcher.exe"),
  SK_MakeUnicode (L"skyrimselauncher.exe"),
  SK_MakeUnicode (L"modlauncher.exe"),
  SK_MakeUnicode (L"obduction.exe"),
  SK_MakeUnicode (L"grandia2launcher.exe"),
  SK_MakeUnicode (L"bethesda.net_launcher.exe"),
  SK_MakeUnicode (L"splashscreen.exe"),
  SK_MakeUnicode (L"gamelaunchercefchildprocess.exe"),
  SK_MakeUnicode (L"dplauncher.exe"),
  SK_MakeUnicode (L"cnnlauncher.exe"),
  SK_MakeUnicode (L"gtavlauncher.exe"),
  SK_MakeUnicode (L"a17config.exe"),
  SK_MakeUnicode (L"a18config.exe"), // Atelier Firis
  SK_MakeUnicode (L"zeroescape-launcher.exe"),
  SK_MakeUnicode (L"gtavlanguageselect.exe"),
  SK_MakeUnicode (L"controllercompanion.exe"),
  SK_MakeUnicode (L"nioh_launcher.exe"),
  SK_MakeUnicode (L"rottlauncher.exe"),
  SK_MakeUnicode (L"configtool.exe"),
  SK_MakeUnicode (L"crs-uploader.exe"), // Days Gone Launcher thingy

  SK_MakeUnicode (L"coherentui_host.exe"),
  SK_MakeUnicode (L"activationui.exe"),
  SK_MakeUnicode (L"zossteamstarter.exe"),
  SK_MakeUnicode (L"eac.exe"),
  SK_MakeUnicode (L"ealink.exe"),

  SK_MakeUnicode (L"crashpad_handler.exe"),
  SK_MakeUnicode (L"clang-tidy.exe"),
  SK_MakeUnicode (L"clupdater.exe"),
  SK_MakeUnicode (L"activate.exe"),
  SK_MakeUnicode (L"werfault.exe"),
  SK_MakeUnicode (L"x64dbg.exe"),
  SK_MakeUnicode (L"locationnotificationwindows.exe"),
  SK_MakeUnicode (L"servicehub.datawarehousehost.exe"),
  SK_MakeUnicode (L"launchtm.exe"),
  SK_MakeUnicode (L"displayhdrcompliancetests.exe"),
  SK_MakeUnicode (L"scriptedsandbox64.exe"),
  SK_MakeUnicode (L"crashreportclient.exe"),
  SK_MakeUnicode (L"crs-handler.exe"),


  // Early-out for most CEF-based apps;
  //   Stuff like Spotify otherwise appears to
  //     be a game since it uses D3D shaders
  SK_MakeUnicode (L"libcef.dll"),
  SK_MakeUnicode (L"gamebar.exe"),
  SK_MakeUnicode (L"gamingservicesui.exe"),
  SK_MakeUnicode (L"pwahelper.exe"),

  // Does not reply to DLL unload requests
  SK_MakeUnicode (L"mspaint.exe"),
  SK_MakeUnicode (L"notepad.exe"),
  SK_MakeUnicode (L"explorer.exe"),
  SK_MakeUnicode (L"prevhost.exe"),

  SK_MakeUnicode (L"windowsterminal.exe"),
  SK_MakeUnicode (L"cmd.exe"),
  SK_MakeUnicode (L"steam.exe"),
  SK_MakeUnicode (L"powershell.exe"),
  SK_MakeUnicode (L"openconsole.exe"),
  SK_MakeUnicode (L"epicwebhelper.exe"),
  SK_MakeUnicode (L"steamwebhelper.exe"),
  SK_MakeUnicode (L"epicgameslauncher.exe"),
  SK_MakeUnicode (L"galaxyclient helper.exe"),


  SK_MakeUnicode (L"applicationframehost.exe"),
  SK_MakeUnicode (L"servicehub.host.clr.x86.exe"),
  SK_MakeUnicode (L"servicehub.settingshost.exe"),
  SK_MakeUnicode (L"servicehub.identityhost.exe"),
  SK_MakeUnicode (L"servicehub.threadedwaitdialog.exe"),
  SK_MakeUnicode (L"nvidia web helper.exe"),
  SK_MakeUnicode (L"tobii.eyex.engine.exe"),
  SK_MakeUnicode (L"esrv.exe"),
  SK_MakeUnicode (L"ipoint.exe"),
  SK_MakeUnicode (L"itype.exe"),
  SK_MakeUnicode (L"devenv.exe"),
  SK_MakeUnicode (L"msedge.exe"),
  SK_MakeUnicode (L"vsgraphics.exe")
};
#endif