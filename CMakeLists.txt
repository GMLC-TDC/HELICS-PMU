# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Copyright (c) 2021, Battelle Memorial Institute; Lawrence Livermore
# National Security, LLC; Alliance for Sustainable Energy, LLC.
# See the top-level NOTICE for additional details.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cmake_minimum_required(VERSION 3.15...3.20)

project(HELICS-PMU VERSION 0.0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set (HELICS-PMU_VERSION_BUILD )
set (HELICS-PMU_DATE "07-21-21")
set(
  HELICS-PMU_VERSION_UNDERSCORE
  "${HELICS-PMU_VERSION_MAJOR}_${HELICS-PMU_VERSION_MINOR}_${HELICS-PMU_VERSION_PATCH}"
)
if(HELICS-PMU_VERSION_BUILD)
  set(HELICS-PMU_VERSION "${HELICS-PMU_VERSION}-${HELICS-PMU_VERSION_BUILD}")
  set(HELICS-PMU_VERSION_UNDERSCORE "${HELICS-PMU_VERSION_UNDERSCORE}-${HELICS-PMU_VERSION_BUILD}")
endif()
set(HELICS-PMU_VERSION_STRING "${HELICS-PMU_VERSION} (${HELICS-PMU_DATE})")

# -----------------------------------------------------------------------------
# set the module path and include some common macros
# -----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/config/cmake/")
include(extraMacros)
include(CMakeDependentOption)
include(CTest)

FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/logs)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(WIN32)
    if(MSYS)
      set(
        CMAKE_INSTALL_PREFIX "/usr/local/helics-pmu_${HELICS-PMU_VERSION_UNDERSCORE}/"
        CACHE PATH "default install path"
        FORCE
      )
		else(MSYS)
      set(
        CMAKE_INSTALL_PREFIX "C:/local/helics-pmu_${HELICS-PMU_VERSION_UNDERSCORE}/"
        CACHE PATH "default install path"
        FORCE
      )
		endif(MSYS)
	endif(WIN32)
endif()

# Prohibit in-source build
IF(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source build is not supported. Please, use an empty directory for building the project.")
ENDIF()

# Set the build output paths
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH
                                                                           "Archive output dir."
        )
    endif()
    if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH
                                                                           "Library output dir."
        )
    endif()
    if(NOT CMAKE_PDB_OUTPUT_DIRECTORY)
        set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            CACHE PATH "PDB (MSVC debug symbol)output dir."
        )
    endif()
    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            CACHE PATH "Executable/dll output dir."
        )
    endif()
endif()


add_library(helics_pmu_base INTERFACE)

if (NOT TARGET compile_flags_target)
   add_library(compile_flags_target INTERFACE)
endif()


if(NOT TARGET build_flags_target)
    add_library(build_flags_target INTERFACE)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    mark_as_advanced(BUILD_TESTING)
    include(compiler_flags)

endif()

target_link_libraries(helics_pmu_base INTERFACE compile_flags_target)

cmake_dependent_advanced_option(
  STATIC_STANDARD_LIB
  "Link against a static standard lib"
  OFF
  "NOT USE_LIBCXX"
  OFF
)

include(ucm)
if(STATIC_STANDARD_LIB)
	ucm_set_runtime(STATIC)
else()
	ucm_set_runtime(DYNAMIC)
endif()

if(MSVC)
	option(EMBEDDED_DEBUG_INFO "embed debug info into obj files" OFF)
	mark_as_advanced(EMBEDDED_DEBUG_INFO)
  if(EMBEDDED_DEBUG_INFO)
		ucm_set_embedded_debug(EMBEDDED)
	else()
		ucm_set_embedded_debug(EXTERNAL)
	endif()
endif(MSVC)

# -----------------------------------------------------------------------------
# Setup configure.h file for accessing configure options
# -----------------------------------------------------------------------------
configure_file(
    "config/helics-pmu-config.h.in"
    "${PROJECT_BINARY_DIR}/generated_includes/helics-pmu-config.h"
)

target_include_directories(
    helics_pmu_base INTERFACE $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated_includes/>
)
#########################################################################################

include(GNUInstallDirs)

# -------------------------------------------------------------
# setting the RPATH
# -------------------------------------------------------------
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_MACOSX_RPATH ON)
set(
  CMAKE_BUILD_RPATH
  "./;${AUTOBUILD_INSTALL_PATH}/bin;${AUTOBUILD_INSTALL_PATH}/lib;${AUTOBUILD_INSTALL_PATH}/lib64"
)

# add the automatically determined parts of the RPATH which point to directories outside
# the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(
  FIND
    CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" isSystemDir
)
if("${isSystemDir}" STREQUAL "-1")
  list(
    APPEND
      CMAKE_INSTALL_RPATH CMAKE_INSTALL_RPATH
      "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
  )
endif()

list(
  FIND
    CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir
)
if("${isSystemDir}" STREQUAL "-1")
    list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif()


# add the local directory to the rpath
list(APPEND CMAKE_INSTALL_RPATH "./")


# -------------------------------------------------------------
# global include directories
# -------------------------------------------------------------
# -------------------------------------------------------------
# global include directories
# -------------------------------------------------------------
target_include_directories(
    helics_pmu_base
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
              $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generate_include>
              $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ThirdParty/concurrency>
              $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ThirdParty/utilities>
)

target_compile_definitions(helics_pmu_base INTERFACE "-DASIO_STANDALONE")
    target_include_directories(
        helics_pmu_base SYSTEM
        INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ThirdParty/asio/asio/include>
    )
    
# -------------------------------------------------------------
# load the required subdirectories
# -------------------------------------------------------------
include(updateGitSubmodules)
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/ThirdParty/fmtlib/CMakeLists.txt")
    submod_update(ThirdParty/fmtlib)
endif()


if(NOT EXISTS "${PROJECT_SOURCE_DIR}/ThirdParty/jsoncpp/CMakeLists.txt")
    submod_update(ThirdParty/jsoncpp)
endif()


if(NOT EXISTS "${PROJECT_SOURCE_DIR}/ThirdParty/utilities/gmlc/utilities/stringOps.h")
    submod_update(ThirdParty/utilities)
endif()


if(NOT EXISTS "${PROJECT_SOURCE_DIR}/ThirdParty/concurrency/gmlc/concurrency/Barrier.hpp")
        submod_update(ThirdParty/concurrency)
    endif()
    
include(GNUInstallDirs)

# -------------------------------------------------------------
# Enable HELICS executable
# -------------------------------------------------------------

SHOW_VARIABLE(HELICS_INSTALL_PATH PATH "path to the helics installation" "${PROJECT_BINARY_DIR}/libs")

include(addHELICS)


set(GMLC_UTILITIES_OBJECT_LIB OFF CACHE INTERNAL "")
set(GMLC_UTILITIES_STATIC_LIB ON CACHE INTERNAL "")
set(GMLC_UTILITIES_INCLUDE_BOOST OFF CACHE INTERNAL "")

add_subdirectory(${PROJECT_SOURCE_DIR}/ThirdParty/utilities ${PROJECT_BINARY_DIR}/ThirdParty/utilities)

hide_variable(GMLC_UTILITIES_GENERATE_DOXYGEN_DOC)
hide_variable(GMLC_UTILITIES_USE_BOOST_SPIRIT)
hide_variable(GMLC_UTILITIES_WITH_CMAKE_PACKAGE)

set_target_properties(gmlc_utilities PROPERTIES FOLDER Extern)


add_subdirectory(src/pmu)

add_subdirectory(src/helicsPMU)

option (HELICS_PMU_GENERATE_DOXYGEN_DOC "Generate Doxygen doc target" OFF)

if (HELICS_PMU_GENERATE_DOXYGEN_DOC)
find_package(Doxygen)
if(DOXYGEN_FOUND)

	SHOW_VARIABLE(DOXYGEN_OUTPUT_DIR PATH "location to put Doxygen docs" "${PROJECT_BINARY_DIR}/docs")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc
	${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${DOXYGET_OUTPUT_DIR}
	COMMENT "Generating API documentation with Doxygen" VERBATIM
)
else(DOXYGEN_FOUND)
	message(FATAL_ERROR "unable to find doxygen for generating documentation")
endif()
endif (HELICS_PMU_GENERATE_DOXYGEN_DOC)


FILE(GLOB KEY_LIBRARY_FILES  ${PROJECT_BINARY_DIR}/libs/bin/*)
message(STATUS "key files ${KEY_LIBRARY_FILES}")


add_subdirectory(src/main)

# -------------------------------------------------------------
# Enable clang analysis and formatting tools
# -------------------------------------------------------------

OPTION(ENABLE_CLANG_TOOLS "if clang is found enable some custom targets for clang formatting and tidy" OFF)

if (ENABLE_CLANG_TOOLS)
include(clang-cxx-dev-tools)
endif(ENABLE_CLANG_TOOLS)



INSTALL(FILES ${KEY_LIBRARY_FILES} DESTINATION bin)


CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/config/helics-pmu-config.h.in ${PROJECT_BINARY_DIR}/libs/include/helics-pmu/helics-pmu-config.h)

#-----------------------------------------------------------------------------
# Setup CTEST environment
#-----------------------------------------------------------------------------
OPTION(HELICS_PMU_BUILD_TESTS "Enable the test Executables to be built" OFF)
# enable testing
if (HELICS_PMU_BUILD_TESTS AND BUILD_TESTING)
add_subdirectory (tests)
endif()


# -------------------------------------------------------------
# Future Additions
# -------------------------------------------------------------

#adding dlls
# INSTALL(FILES ${LOCATION_OF_FILES} DESTINATION bin)
#FILE(GLOB docs "docs/manuals/*")
#INSTALL(FILES ${docs} DESTINATION docs)



# -------------------------------------------------------------
# CPack for NSIS Installer
# -------------------------------------------------------------
OPTION(HELICS_PMU_ENABLE_PACKAGE_BUILD "Add projects for making packages and installers for HELICS-PMU" OFF)

IF(HELICS_PMU_ENABLE_PACKAGE_BUILD)
set(CPACK_PACKAGE_NAME "HELICS-pmu")
set(CPACK_PACKAGE_VENDOR "Lawrence Livermore National Security")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Helics-pmu Installer")
set(CPACK_PACKAGE_VERSION ${HELICS-PMU_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${HELICS-PMU_VERISON_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HELICS-PMU_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HELICS-PMU_VERSION_PATCH})

if (WIN32)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}\\\\LICENSE")
else(WIN32)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif(WIN32)

 if (WIN32)
#	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\docs\\\\img\\\\HELICS.ico")
#	set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/docs/img/HELICS.ico")
#	set(CPACK_NSIS_INSTALL_ROOT "C:\\\\local\\\\")
	set(CPACK_NSIS_URL_INFO_ABOUT "https://www.github.com/GMLC-TDC/Helics-pmu")
set(CPACK_NSIS_MENU_LINKS
	"https://www.github.com/GMLC-TDC/Helics-pmu" "source code"
	"https://helics.readthedocs.io/en/latest/" "Helics Documentation"
	"https://www.helics-pmu.org" "Helics PMU Web page"
	"https://www.youtube.com/channel/UCPa81c4BVXEYXt2EShTzbcg" "TDC YouTube channel")
else (WIN32)
	#set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/docs/img/HELICS.ico")
endif(WIN32)


set(CPACK_SOURCE_IGNORE_FILES  "/Build*/;/build*/;/.git/")

#THIS LINE MUST BE LAST
include(CPack)

ENDIF(HELICS_PMU_ENABLE_PACKAGE_BUILD)
