# Contributors
This file describes the contributors to the HELICS-FMI library and the software used as part of this project
If you would like to contribute to any of the HELICS related projects see [CONTRIBUTING](CONTRIBUTING.md)
## Individual contributors

### Lawrence Livermore National Lab
-   Philip Top*

`*` currently active

## Used Libraries or Code

### [helics](https://github.com/GMLC-TDC/HELICS)
  The library is based on HELICS and interacts with the library

### [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
  JsonCPP is used for parsing json files, it was chosen for easy inclusion in the project and its support for comments. Jsoncpp is licensed under public domain or MIT in case public domain is not recognized [LICENSE](https://github.com/open-source-parsers/jsoncpp/blob/master/LICENSE)

### [fmt](http://fmtlib.net/latest/index.html)
FMT replaces boost::format for internal logging and message printing.  The library is included in the source code.  The CMAKE scripts were modified so they don't trigger a bunch of unnecessary checks and warnings as nearly all checks are already required for building HELICS based on minimum compiler support.  HELICS uses the header only library for the time being.  FMT is licensed under [BSD 2 clause](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)

### [date](https://github.com/HowardHinnant/date)
The date library is a library for manipulation of dates in C++ includes some ability to handle time zones and leap seconds.  The header source for time zone data is included in the repository.  Date is licensed under [MIT](https://github.com/HowardHinnant/date/blob/master/LICENSE.txt)

### CMake scripts
Several CMake scripts came from other sources and were either used or modified for use in HELICS.
-   Lars Bilke [CodeCoverage.cmake](https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake)
-   clang-format, clang-tidy scripts were created using tips from [Emmanuel Fleury](http://www.labri.fr/perso/fleury/posts/programming/using-clang-tidy-and-clang-format.html)
-   Viktor Kirilov, useful CMake macros [ucm](https://github.com/onqtam/ucm)  particularly for the set_runtime macro to use static runtime libraries
