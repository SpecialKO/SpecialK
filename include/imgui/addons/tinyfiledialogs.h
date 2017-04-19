/*
 _________
/         \ tinyfiledialogs.c 
|tiny file| Unique code file of "tiny file dialogs" created [November 9, 2014]
| dialogs | Copyright (c) 2014 - 2016 Guillaume Vareille http://ysengrin.com
\____  ___/ http://tinyfiledialogs.sourceforge.net
     \|           	                     mailto:tinfyfiledialogs@ysengrin.com

            git://git.code.sf.net/p/tinyfiledialogs/code

 Please
	1) let me know
	- if you are including tiny file dialogs,
	  I'll be happy to add your link to the list of projects using it.
	- If you are using it on different hardware / OS / compiler.
	2) Be the first to leave a review on Sourceforge. Thanks.

tiny file dialogs (cross-platform C C++)
InputBox PasswordBox MessageBox ColorPicker
OpenFileDialog SaveFileDialog SelectFolderDialog
Native dialog library for WINDOWS MAC OSX GTK+ QT CONSOLE & more
v2.3.8 [May 10, 2016] zlib licence.

A single C file (add it to your C or C++ project) with 6 modal function calls:
- message box & question box
- input box & password box
- save file dialog
- open file dialog & multiple files
- select folder dialog
- color picker.

Complement to OpenGL GLFW GLUT GLUI
VTK SFML SDL Ogre Unity CEGUI ION MathGL
CPW GLOW GLT NGL STB & GUI less programs

NO INIT & NO MAIN LOOP

The dialogs can be forced into console mode

On Windows:
- native code & some vbs create the graphic dialogs
- enhanced console mode can use dialog.exe from
  http://andrear.altervista.org/home/cdialog.php
- basic console input.

On Unix (command line call attempts):
- applescript
- zenity
- kdialog
- Xdialog
- python2 tkinter
- dialog (opens a console if needed)
- whiptail, gdialog, gxmessage
- basic console input.
The same executable can run across desktops & distributions.

tested with C & C++ compilers
on Visual Studio MinGW OSX LINUX FREEBSD ILLUMOS SOLARIS MINIX RASPBIAN
using Gnome Kde Enlightenment Mate Cinnamon Unity
Lxde Lxqt Xfce WindowMaker IceWm Cde Jds OpenBox
 
- License -

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.  If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

// The only modifications added to the original tinyfiledialog repository are:
// -> changed .c file into .cpp
// -> added namespace tfd
// -> stripped prefix: "tinyfd_" to all methods

#ifndef IMGUITINYFILEDIALOGS_H
#define IMGUITINYFILEDIALOGS_H

namespace tfd	{

int messageBox (
  char const * const aTitle , /* "" */
  char const * const aMessage , /* "" may contain \n \t */
  char const * const aDialogType , /* "ok" "okcancel" "yesno" */
  char const * const aIconType , /* "info" "warning" "error" "question" */
  int const aDefaultButton ) ; /* 0 for cancel/no , 1 for ok/yes */
	/* returns 0 for cancel/no , 1 for ok/yes */

char const * inputBox (
	char const * const aTitle , /* "" */
	char const * const aMessage , /* "" may NOT contain \n \t on windows */
	char const * const aDefaultInput ) ;  /* "" , if NULL it's a passwordBox */
	/* returns NULL on cancel */

char const * saveFileDialog (
  char const * const aTitle , /* "" */
  char const * const aDefaultPathAndFile , /* "" */
  int const aNumOfFilterPatterns , /* 0 */
  char const * const * const aFilterPatterns , /* NULL | {"*.jpg","*.png"} */
  char const * const aSingleFilterDescription ) ; /* NULL | "text files" */
	/* returns NULL on cancel */

char const * openFileDialog (
  char const * const aTitle , /* "" */
  char const * const aDefaultPathAndFile , /* "" */
  int const aNumOfFilterPatterns , /* 0 */
  char const * const * const aFilterPatterns , /* NULL {"*.jpg","*.png"} */
  char const * const aSingleFilterDescription , /* NULL | "image files" */
  int const aAllowMultipleSelects ) ; /* 0 or 1 */
	/* in case of multiple files, the separator is | */
	/* returns NULL on cancel */

char const * selectFolderDialog (
	char const * const aTitle , /* "" */
  char const * const aDefaultPath ) ; /* "" */
	/* returns NULL on cancel */

char const * colorChooser(
	char const * const aTitle , /* "" */
	char const * const aDefaultHexRGB , /* NULL or "#FF0000" */
	unsigned char const aDefaultRGB[3] , /* { 0 , 255 , 255 } */
	unsigned char aoResultRGB[3] ) ; /* { 0 , 0 , 0 } */
	/* returns the hexcolor as a string "#FF0000" */
	/* aoResultRGB also contains the result */
	/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
	/* aDefaultRGB and aoResultRGB can be the same array */
	/* returns NULL on cancel */

/* not cross platform - zenity only */
char const * arrayDialog (
	char const * const aTitle , /* "" */
	int const aNumOfColumns , /* 2 */
	char const * const * const aColumns , /* {"Column 1","Column 2"} */
	int const aNumOfRows , /* 2*/
	char const * const * const aCells ) ;
		/* {"Row1 Col1","Row1 Col2","Row2 Col1","Row2 Col2"} */


extern int tinyfd_forceConsole ;  /* 0 (default) or 1
can be modified at run time.
for unix & windows: 0 (graphic mode) or 1 (console mode).
0: try to use a graphic solution, if it fails then it uses console mode.
1: forces all dialogs into console mode even when the X server is present.
   it will use the package dialog or dialog.exe if installed.
on windows it only make sense for console applications */

/* #define TINYFD_WIN_CONSOLE_ONLY //*/
/* On windows, Define this if you don't want to include the code
creating the GUI dialogs.
Then you don't need link against Comdlg32.lib and Ole32.lib */
}


#endif /* IMGUITINYFILEDIALOGS_H */


/*
- This is not for android nor ios.
- The code is pure C, perfectly compatible with C++.
- AVOID USING " AND ' IN TITLES AND MESSAGES.
- There's one file filter only, it may contain several patterns.
- If no filter description is provided,
  the list of patterns will become the description.
- char const * filterPatterns[3] = { "*.obj" , "*.stl" , "*.dxf" } ;
- On windows, inputbox and passwordbox are not as smooth as they should be:
  they open a console window for a few seconds.
- On visual studio:
      set Properties/Configuration Properties/General
			Character Set to "Multi-Byte" or "Not Set"
- On windows link against Comdlg32.lib and Ole32.lib
  This linking is not compulsary for console mode (see above).
- On unix: it tries command line calls, so no such need.
- On unix you need applescript, zenity, kdialog, Xdialog, python2/tkinter
  or dialog (will open a terminal if running without console);
- One of those is already included on most (if not all) desktops.
- In the absence of those it will use gdialog, gxmessage or whiptail
  with a textinputbox.
- If nothing is found, it switches to basic console input,
  it opens a console if needed.
- Use windows separators on windows and unix separators on unix.
- String memory is preallocated statically for all the returned values.
- File and path names are tested before return, they are valid.
- If you pass only a path instead of path + filename,
  make sure it ends with a separator.
- tinyfd_forceConsole=1; at run time, forces dialogs into console mode.
- On windows, console mode only make sense for console applications.
- Mutiple selects are not allowed in console mode.
- The package dialog must be installed to run in enhanced console mode.
  It is already installed on most unix systems.
- On osx, the package dialog can be installed via http://macports.org
- On windows, for enhanced console mode,
  dialog.exe should be copied somewhere on your executable path.
  It can be found at the bottom of the following page:
  http://andrear.altervista.org/home/cdialog.php
- If dialog is missing, it will switch to basic console input.

- Here is the Hello World (and a bit more):
    if a console is missing, it will use graphic dialogs
    if a graphical display is absent, it will use console dialogs
*/

/* hello.c
#include <stdio.h>
#include "tinyfiledialogs.h"
int main()
{
	char const * lThePassword;
	char const * lTheSaveFileName;
	char const * lTheOpenFileName;
	FILE * lIn;
	char lBuffer[1024];

  tinyfd_forceConsole = tinyfd_messageBox("Hello World",
    "force dialogs into console mode?\
    \n\t(it is better if dialog is installed)",
    "yesno", "question", 0);

  lThePassword =  tinyfd_inputBox(
    "a password box","your password will be revealed",NULL);

  lTheSaveFileName = tinyfd_saveFileDialog (
	"let us save this password",
    "passwordFile.txt",
    0,
    NULL,
    NULL );

#pragma warning(disable:4996) // silences warning about fopen
	lIn = fopen(lTheSaveFileName, "w");
#pragma warning(default:4996)
	if (!lIn)
	{
		tinyfd_messageBox(
			"Error",
			"Can not open this file in writting mode",
			"ok",
			"error",
			1 );
		return(1);
	}
	fputs(lThePassword, lIn);
	fclose(lIn);

    lTheOpenFileName = tinyfd_openFileDialog (
		"let us read this password back",
		"",
		0,
		NULL,
		NULL,
		0);

#pragma warning(disable:4996) // silences warning about fopen
	lIn = fopen(lTheOpenFileName, "r");
#pragma warning(default:4996)
	if (!lIn)
	{
		tinyfd_messageBox(
			"Error",
			"Can not open this file in reading mode",
			"ok",
			"error",
			1 );
		return(1);
	}
	fgets(lBuffer, sizeof(lBuffer), lIn);
	fclose(lIn);

  if ( *lBuffer )
    tinyfd_messageBox("your password is", lBuffer, "ok", "info", 1);
}

OSX :
$ gcc -o hello.app hello.c tinyfiledialogs.c
 
UNIX :
$ gcc -o hello hello.c tinyfiledialogs.c

MinGW :
> gcc -o hello.exe hello.c tinyfiledialogs.c -LC:/mingw/lib -lcomdlg32 -lole32
 
VisualStudio :
  Create a console application project,
	it links against Comdlg32.lib & Ole32.lib.
	Right click on your Project, select Properties.
	Configuration Properties/General
	Character Set to "Multi-Byte" or "Not Set"
*/
