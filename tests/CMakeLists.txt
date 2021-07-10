##############################################################################
#Copyright (C) 2017-2018, Battelle Memorial Institute
#All rights reserved.

#This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.
##############################################################################

#-----------------------------------------------------------------------------
# helics pmu tests using google test
#-----------------------------------------------------------------------------


set(helics_pmu_test_sources
exeTestHelper.cpp
exeTestHelper.h
pmuInitTests.cpp
helics-pmuTests.cpp
packetParserTests.cpp
PcapPacketParser.h
PcapPacketParser.cpp
)


include(AddGoogletest)

# add a baseline library for underlying dependencies and flags for test executables
add_library(helics_pmu_test_base INTERFACE)
target_link_libraries(helics_pmu_test_base INTERFACE gtest gtest_main gmock)
target_link_libraries(helics_pmu_test_base INTERFACE compile_flags_target)


add_executable(helics_pmu-tests ${helics_pmu_test_sources})

set_target_properties (helics_pmu-tests PROPERTIES FOLDER tests)

target_link_libraries(helics_pmu-tests helics_pmu_test_base pmu)

add_test(NAME helics_pmu-tests COMMAND helics_pmu-tests)

target_compile_definitions(helics_pmu-tests PRIVATE -DTEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test_files\")


