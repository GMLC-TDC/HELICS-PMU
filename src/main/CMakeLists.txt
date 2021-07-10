# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Copyright (c) 2021, Battelle Memorial Institute; Lawrence Livermore
# National Security, LLC; Alliance for Sustainable Energy, LLC.
# See the top-level NOTICE for additional details.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(helics-pmu_sources
	helics-pmu-main.cpp
	)

set(helics-pmu_headers
	)

add_executable(helics-pmu ${helics-pmu_sources} ${helics-pmu_headers})


target_link_libraries(helics-pmu helicsPMU)

foreach(keyfile IN LISTS KEY_LIBRARY_FILES)
add_custom_command(TARGET helics-pmu POST_BUILD        # Adds a post-build event to helics-pmu
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${keyfile}"      # <--this is in-file
        $<TARGET_FILE_DIR:helics-pmu>)                 # <--this is out-file path
endforeach(keyfile)

INSTALL(TARGETS helics-pmu RUNTIME DESTINATION bin)
