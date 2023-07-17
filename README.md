### Spectral HOM Analysis Tool

This program can be used to analyze the results of a Spectral HOM experiment, with data collected from an intensified
TimePix3 camera.

To use the program, you must first install the following:
 - A modern C++ compiler (for C++20 or higher). This is already installed on Linux (gcc) and Mac OS (clang). Windows users can choose between [MSVC](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/), [MinGW](https://www.mingw-w64.org/), or [clang](https://llvm.org/) compilers.
 - [Qt 6.5+](https://www.qt.io/download-qt-installer-oss).
 - [CMake](https://cmake.org/) (the Qt online installer optionally includes this).
 - [Point Cloud Library](https://pointclouds.org/downloads/).

The code has been tested on Linux and Windows. It is written to be platform-independent, so Mac OS should be able to run it as well.

To build the executable, run the following commands (or their equivalent on your OS).
Here, `/path/to/qt6` should be replaced with the path to your Qt 6 installation
(e.g., C:/Qt/6.5.1/msvc2019_64 on my Windows machine, or the root directory of the build if you built Qt from source).
``` bash
git clone git@github.com:k-m-jordan/Spectral-HOM.git
cd Spectral-HOM
mkdir bin
cmake -B bin -DQT6_INSTALL=/path/to/qt6
cd bin
make
```
If you encounter any errors, report them to the maintainer of the repository [by email](mailto:kjordan@uottawa.ca).

Enjoy!