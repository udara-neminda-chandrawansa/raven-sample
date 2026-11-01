# Raven Sample

A small program on top of [Raven](https://github.com/oscar-soirey/Raven): a text
input, a counter button, a slider driving a progress bar, a switch, and French
and English strings. The window is described in `assets/main.xml`; the C code in
`src/main.c` only binds the actions the XML names.

```
raven-sample/
├── CMakeLists.txt        finds, or downloads, Raven and builds the program
├── src/main.c            the program
├── assets/main.xml       the interface
├── assets/strings.csv    translations (en, fr)
└── .vscode/              settings, launch (F5) and extension recommendations
```

## Run it in VS Code

1. Install a C compiler and CMake:
   - **Windows:** Visual Studio 2022 with *Desktop development with C++* (or MSYS2 MinGW-w64), plus Git.
   - **Linux:** `sudo apt install build-essential cmake git libx11-dev ninja-build`
   - **macOS:** `xcode-select --install` and `brew install cmake`
2. Install the **CMake Tools** and **C/C++** extensions (VS Code offers them on opening the folder).
3. Open this folder in VS Code. When asked, pick a kit (Visual Studio Community 2022 Release - amd64 on Windows, GCC or Clang elsewhere).
4. Press **F5** and choose the debugger that matches your kit.
   The first configure downloads and builds Raven, which takes a few minutes; later builds are quick.

The program runs in `assets/`, so `main.xml` and `strings.csv` are found. **Save
`main.xml` while the program runs and the window follows**, with no rebuild.

You need a Vulkan driver at run time (any recent GPU driver on Windows and
Linux; MoltenVK on macOS). In a virtual machine, WSL without GPU support, or a
remote container with no GPU, Raven cannot start and prints
`Cannot start Raven. Is a Vulkan driver installed?`.

## Where Raven comes from

`CMakeLists.txt` tries three things, in order:

1. An installed package: configure with `-DCMAKE_PREFIX_PATH=<Raven install folder>`.
2. A local checkout: configure with `-DRAVEN_SOURCE_DIR=<path to Raven>`.
3. Otherwise it downloads Raven from GitHub (`main` branch) with `FetchContent`.

In VS Code, set (1) or (2) in `.vscode/settings.json` (see the commented lines).

## Make a .deb (Linux) or a .exe (Windows)

Packaging is done by CPack, configured at the end of `CMakeLists.txt`. Build in
**Release** first. You must build on the system you are packaging for: the
`.deb` on Linux, the `.exe` on Windows.

**Linux, `.deb`** (needs `dpkg-dev`):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && cpack                       # raven-sample_1.0.0_amd64.deb
sudo apt install ./raven-sample_1.0.0_amd64.deb
```

The package installs `/usr/bin/raven_sample`, its files in
`/usr/share/raven-sample` and a menu entry. It depends on `libvulkan1`.

**Windows, the program and a zip** (from *Developer PowerShell for VS 2022*):

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
cd build
cpack -C Release                        # raven-sample-1.0.0-win64.zip
```

The `.exe` itself is `build\Release\raven_sample.exe`; it needs `assets\`
next to it, which is why the zip holds both. For a real installer
(`raven-sample-1.0.0-win64.exe`), install [NSIS](https://nsis.sourceforge.io)
and run `cpack -C Release -G NSIS`.

### Linux AppImage (one file for most distributions)

An AppImage is a single file that runs on most Linux distributions without
installing anything: `chmod +x` it and double-click or run it. Unlike the
`.deb`, it also works on Fedora, Arch and others. It still needs a Vulkan
driver on the machine, and a glibc at least as new as the system it was built on.

Build in **Release**, install into an `AppDir` folder, and let
[linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) pack it:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# one-time: download the tools next to the project
mkdir -p tools && cd tools
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage
chmod +x *.AppImage
ln -sf linuxdeploy-plugin-appimage-x86_64.AppImage linuxdeploy-plugin-appimage
cd ..

# every release
rm -rf AppDir
cmake --install build --prefix AppDir/usr --component app
export PATH="$PWD/tools:$PATH" ARCH=x86_64 LINUXDEPLOY_OUTPUT_VERSION=1.0.0
./tools/linuxdeploy-x86_64.AppImage --appdir AppDir \
    -d AppDir/usr/share/applications/raven-sample.desktop \
    -i AppDir/usr/share/icons/hicolor/256x256/apps/raven-sample.png \
    --output appimage
```

This writes `Raven_Sample-1.0.0-x86_64.AppImage` in the current folder. If
your system has no FUSE (some containers, newer Ubuntu without `libfuse2`),
run it with `APPIMAGE_EXTRACT_AND_RUN=1 ./Raven_Sample-1.0.0-x86_64.AppImage`,
or install `libfuse2` (`sudo apt install libfuse2`; `libfuse2t64` on Ubuntu 24.04).

What goes inside: the program, `main.xml`, `strings.csv`, the icon and the small
libraries linuxdeploy finds. It leaves out `libX11`, `libxcb` and the Vulkan
driver, which every desktop already has and which must match the machine's
graphics hardware. Add `tools/` and `AppDir/` to `.gitignore` so the download
and the build folder are not committed.

Before you publish: change `CPACK_PACKAGE_VENDOR` and
`CPACK_DEBIAN_PACKAGE_MAINTAINER` in `CMakeLists.txt`. A `.deb` built on a recent
distribution needs a recent glibc, so build on the oldest system you want to
support.

## From a terminal

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cd assets && ../build/raven_sample        # Windows (MSVC): ..\build\Debug\raven_sample.exe
```
