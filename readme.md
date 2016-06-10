# DPengine

An alternative experimental engine for [Desktop Ponies](https://github.com/RoosterDragon/Desktop-Ponies/) written in C and GLSL and focused on performance.

To run properly, DPE requires the [DP animation base](https://github.com/RoosterDragon/Desktop-Ponies/tree/master/Content/Ponies) directory to be present in the project root.

The source code is meant to be built via either Code::Blocks IDE or makefiles included in the project.

N.B.: to be able to build on Windows using Code::Blocks, go to Settings → Compiler and Debugger → Toolchain Executables, and change 'Make program' to 'mingw32-make.exe'. It`s how it was meant to be.

## Release notes

Major refactoring effort is currently underway, along with adding features previously ignored, like sprite effects and categories. The first draft of GUI from the master branch is discarded, however bringing up some very important points that will be considered in a new iteration of GUI architecture.

MacOS X branch is on hiatus till there is access to a machine powerful enough to run a Mac VM faster than 5 FPS on idle. Sad but true.

## License

The artwork is licensed under [Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)](http://creativecommons.org/licenses/by-nc-sa/3.0/). This means you are allowed to share and alter the artwork, provided you give credit, do not use it for commercial purposes and release it under this same license.

The same license applies to the source code.
