# CSNZ Server
Open-source server for the Counter-Strike Nexon: Studio game. Written in C++, supports Windows, Linux (requires tests).

Don't use it for commercial purposes.

# Contributing
You can help the project by solving issues, detecting/fixing bugs, and various problems.

# Building
Currently only Windows and Linux are supported, other platforms not tested.

Clone a repository:
```
git clone https://github.com/JusicP/CSNZ_Server
git submodule init
git submodule update --depth 1
```

### Windows
* [Visual Studio 2019 or newer](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community)
* [CMake](https://www.cmake.org/download/)
* [Qt 6.5.3 (optional)](https://www.qt.io/download-qt-installer)
  
**Set `QTDIR` environment variable: `<PathToQT>\Qt\6.5.3\msvc2019_64` if you want to use GUI.**

Open project folder in Visual Studio and wait for CMake cache generation.
Then click on Build -> Build All.

### Linux
* GCC compiler
* [CMake](https://www.cmake.org/download/)
* [Qt 6.5.3 (optional)](https://www.qt.io/download-qt-installer)

```
cd PathToRepo/CSNZ_Server/src
cmake -S . -B build
cmake --build build
```

# Documentation
You can find server documentation in [DOCUMENTATION.md](docs/documentation.md) file.
