# DPengine

An alternative experimental engine for [Desktop Ponies](https://github.com/RoosterDragon/Desktop-Ponies/) written in C and GLSL and focused on performance.

To run properly, DPE requires the [DP animation base](https://github.com/RoosterDragon/Desktop-Ponies/tree/master/Content/Ponies) directory to be present in the project root.

The source code is meant to be built via either Code::Blocks IDE or makefiles included in the project.

N.B.: to be able to build on Windows using Code::Blocks, go to Settings → Compiler and Debugger → Toolchain Executables, and change 'Make program' to 'mingw32-make.exe'. It`s how it was meant to be.

## Release notes

MacOS X support! Finally!

By now, only tested on a virtualized 10.8, but theoretically the oldest supported version is somewhere near 10.5.

OpenGL renderer is present but mostly untested, due to VM being the only Mac around.

## License

The artwork is licensed under [Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)](http://creativecommons.org/licenses/by-nc-sa/3.0/). This means you are allowed to share and alter the artwork, provided you give credit, do not use it for commercial purposes and release it under this same license.
