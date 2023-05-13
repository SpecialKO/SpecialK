[![Builds](https://github.com/SpecialKO/SpecialK/actions/workflows/build-windows.yml/badge.svg)](https://github.com/SpecialKO/SpecialK/actions/workflows/build-windows.yml)

Special K&nbsp;&nbsp;<sub>*"Lovingly referred to as the Swiss Army Knife of PC gaming, Special K does a bit of everything. It is best known for fixing and enhancing graphics, its many detailed performance analysis and correction mods, and a constantly growing palette of tools that solve a wide variety of issues affecting PC games."*</sub><hr>
>**Contents of Repository**
>
>This repository contains source code for Special K's code injection payload (`SpecialK(32|64).dll`).
>>Supporting utilities such as SKIF (**S**pecial **K** **I**njection **F**rontend) and the now deprecated SKIM (**S**pecial **K** **I**nstall **M**anger) are maintained as separate projects. Additionally, some older mods built using Special K exist as standalone plug-in DLLs; plug-ins and SKIM are no longer actively maintained, but are accessible via Kaldaien's GitHub profile.
>
><br>**Build Dependencies**
>
> All of Special K's build dependencies are included when you clone the repo beginning with 23.5.7. Older versions have an additional dependency on the June 2010 DirectX SDK.
> 
>> Special K requires Visual C++ 2022 or newer to compile due to language features not present in older compilers.
>
><br>**Platform Dependencies**
>
> Special K is not supported on anything older than Windows 8.1, though (as of 23.5.7) still builds and runs (massively feature-reduced) on Windows 7 (Platform Update).
> > It does run in WINE and is compatible with DXVK, but must be configured with `UsingWINE=true` in its per-game INI file to work on Linux.<br>
>
><br>**Miscellaneous**
>
>There is a good chance the project will not compile correctly if you use the Debug build configuration, Special K is designed to produce debuggable Release builds.
<hr>

### High-Level Overview of Special K Code Injection

Special K's DLLs are capable of injecting their code in one of two ways:

**Local Injection**&nbsp;&nbsp;&nbsp;<sub>Proxy / Wrapper DLL</sub>
1. Rename `SpecialK(32|64).dll` to `(dxgi|d3d11|d3d9|d3d8|ddraw|dinput8|OpenGL32).dll` and catch a ride via Static Imports or calls to **`LoadLibrary (...)`**.

**Global Injection**&nbsp;&nbsp;&nbsp;<sub>Win32 Global Hookchain</sub>

2. Globally Inject using CBT / Shell hooks
	>This is the preferred technique, and the DLL is capable of bootstrapping the hook without any outside assistance via **`RunDLL_InjectionManager (...)`** (**rundll32.exe** will host the DLL as a normal Win32 UI process).
	
<br>

There are many more possible ways to inject the DLLs, the two outlined above are useable without any additional tools.

Special K will happily inject into a game that is already running if you want to build your own tool using something like **`CreateRemoteThread (...)`**, but keep in mind that late injection will prevent some of Special K's features (particularly those related to D3D overrides and shader/texture mods) from working.

> CBT Hooks were chosen due to hookchain order. Since most graphics APIs on Windows need a window before they can do non-trivial initialization, a CBT hook reliably gets us into the application ***before*** D3D9/11/12 swapchain creation.
