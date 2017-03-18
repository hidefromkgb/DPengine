# DPengine

An alternative experimental engine for [Desktop Ponies](https://github.com/RoosterDragon/Desktop-Ponies/) written in C and GLSL and focused on performance. Don\`t trust GitHub if it shows ObjC in the list of used languages, it\`s pure C.

To run properly, DPE requires the [DP animation base](https://github.com/RoosterDragon/Desktop-Ponies/tree/master/Content) directory to be present in the project root. In case it\`s not there, DPE tries to download it from GitHub.

The source code is meant to be built via either Code::Blocks IDE or makefiles included in the project.

N.B.: to be able to build on Windows using Code::Blocks, go to Settings → Compiler and Debugger → Toolchain Executables, and change 'Make program' to 'mingw32-make.exe'. It`s how it was meant to be.

## Release notes

All three major desktop operating systems (Windows, Linux and OS X) finally got their native GUI frameworks, and now it\`s high time to add the remaining DP functionality missing from DPE:

1. ~~Extended behaviours~~ __DONE (some directional follow glitches left)__
1. Interactions
1. Speech
1. Games
1. Profiles
1. Screensaver
1. Screen movement restriction (tab 2)
1. ~~[!DP] Port Win32 file dialogs to Win9x~~ __DONE__
1. [!DP] Parallelize sprite processing in frontend
1. [!DP] Move rendering options from tray menu to tab 3
1. [!DP] Add OS-specific options to tab 3
1. [!DP] Do something with networking on Win9x and WinXP

## License

The artwork is licensed under [Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)](http://creativecommons.org/licenses/by-nc-sa/3.0/). This means you are allowed to share and alter the artwork, provided you give credit, do not use it for commercial purposes and release it under this same license.

The same license applies to the source code.
