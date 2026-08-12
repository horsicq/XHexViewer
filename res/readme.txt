# XHexViewer

XHexViewer is a cross-platform hexadecimal file viewer and binary analysis utility for Windows, Linux, and macOS.

## Build

The project supports Qt 5 and Qt 6 through CMake. The shared libraries are expected in the sibling `_mylibs` directory.

- Windows portable builds: `packaging/windows/build_portable_win32.bat` or `build_portable_win64.bat`
- Linux portable archive: `packaging/linux/build_linux.sh [qt-prefix-path]`
- Debian package: `packaging/debian/build_dpkg.sh [qt-prefix-path]`
- macOS packages: `packaging/macos/build_mac.sh [qt-prefix-path]`
