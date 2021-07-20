# HELICS-PMU

**NOTE  this is a work in progress and is not fully functional**

- [x] C37.118-2011 packet parsing
- [ ] config3 parsing
- [x] c37.118-2011 packet generation
  - [x] command generation
  - [x] config generation
  - [x] data generation
- [ ] config3 generation
- [ ] tcp receiver
- [ ] udp receiver
- [ ] tcp transmission
- [ ] udp transmission
- [ ] HELICS publication
- [ ] HELICS input
- [ ] file archiving
- [ ] file player
- [ ] configuration parsing
- [ ] documentation


Executable to allow  [HELICS](https://github.com/GMLC-TDC/HELICS) to interact with PMU's namely C37.118 compliant devices.
This program can act as a receiver for PMU's and as a generator for C37.118 from a HELICS co-simulation.  Also can just record the PMU data to file or play records as a PMU.

## Building

HELICS-PMU uses CMake 3.16+ for build system generation.  It requires HELICS 3.0 or greater to operate.  And a C++17 compiler to build, including <filesystem> header.  Specifically GCC 8, clang 7, and Visual Studio 15.7 or newer.  

### Windows
For building with Visual Studio the cmake-gui is recommended.  
Set the build directory to an empty path, for example HELICS-PMU/build, and the source directory to the HELICS-PMU directory.  
HELICS-PMU will attempt to locate an existing HELICS installation and use those files.  If one does not exist HELICS will automatically download and build it.

### Linux and others
The process is the same as for Windows, with the exception that HELICS will not automatically build unless the AUTOBUILD_HELICS option is enabled.  

## Source Repo

The HELICS-PMU source code is hosted on GitHub: [https://github.com/GMLC-TDC/HELICS-PMU](https://github.com/GMLC-TDC/HELICS-PMU)

## Release
HELICS is distributed under the terms of the BSD-3 clause license. All new
contributions must be made under this license. [LICENSE](LICENSE)

SPDX-License-Identifier: BSD-3-Clause

Portions of the code written by LLNL with release number
LLNL-CODE-780177
