# DPengine

An alternative experimental engine for
[Desktop Ponies](https://github.com/RoosterDragon/Desktop-Ponies)
written in C and GLSL and focused on performance. Don\`t trust GitHub if it
shows ObjC in the list of used languages, it\`s pure C.

Win32 GUI (as seen on Windows 98 SE):
![Win32](https://www.ponychan.net/fan/src/1466830858097.png)

Linux GUI (as seen on Arch Linux 4.6):
![Linux](https://www.ponychan.net/fan/src/1466929218785.png)

MacOS GUI (as seen on 10.10 Yosemite):
![MacOS](https://www.ponychan.net/fan/src/1470521199962.png)

To run properly, DPE requires the
[DP animation base](https://github.com/RoosterDragon/Desktop-Ponies/tree/master/Content),
automatically downloading it from GitHub if possible. If for any reason DPE
keeps failing to download it, the said directory needs to be manually put to
any location on the local hard drive, after which DPE should be configured to
use it: click the 'More Options...' button on the main window and choose the
'Animation base directory' so that it points to the *parent* directory of the
animation base. After that, close DPE and execute it again. Enjoy!

The source code is meant to be built via either Code::Blocks IDE or makefiles
included in the project.

*N.B.:* to be able to build on Windows using Code::Blocks 10.05, go to
Settings → Compiler and Debugger → Toolchain Executables, and change
'Make program' to 'mingw32-make.exe'. It`s how it was meant to be.
More recent versions ship with correct settings.

*N.B.:* to build and run DPE on Linux, install the following packages:

1. make
1. gcc
1. mesa-common-dev
1. libgtk2.0-dev
1. libgtkglext1-dev
1. libcurl4-openssl-dev
1. libssl-dev

## Releases

[Binary releases](https://github.com/hidefromkgb/DPengine/releases)
are available for download. Building from source is no more necessary!

## Release notes

The task at hand is to implement the remaining DP features missing from DPE:

1. Interactions
1. Games
1. Profiles
1. Screensaver
1. Screen movement restriction, tab 2
1. Audio (in doubt over its necessity, but whatever)

Some functionality is not present in DP but would also be nice to have:

1. Add colors to speech bubbles, make default colors configurable
1. Implement the "alias method" for behaviours
1. Parallelize sprite processing in frontend
1. Move rendering options from tray menu to tab 3
1. Add OS-specific options to tab 3
1. Do something with networking on Win9x and WinXP

## License

The source code and binaries are licensed under
[Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)](http://creativecommons.org/licenses/by-nc-sa/3.0/).
This means you are allowed to share and alter them, provided you give credit,
do not use them for commercial purposes and release them under this same
license.

The DP animation base is not a part of this project;
[see here](https://github.com/RoosterDragon/Desktop-Ponies#license)
for details on its license.
