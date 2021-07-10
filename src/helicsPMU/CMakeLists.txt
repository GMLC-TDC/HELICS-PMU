# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Copyright (c) 2021, Battelle Memorial Institute; Lawrence Livermore
# National Security, LLC; Alliance for Sustainable Energy, LLC.
# See the top-level NOTICE for additional details.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(helicsPMU_sources
	helicsPMU.cpp
	)

set(helicsPMU_headers
	helicsPMU.hpp
	)

add_library(helicsPMU STATIC ${helicsPMU_sources} ${helicsPMU_headers})

target_link_libraries(helicsPMU HELICS::helicscpp pmu helics_pmu_base)

