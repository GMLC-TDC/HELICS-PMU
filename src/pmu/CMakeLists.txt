# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Copyright (c) 2021, Battelle Memorial Institute; Lawrence Livermore
# National Security, LLC; Alliance for Sustainable Energy, LLC.
# See the top-level NOTICE for additional details.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(pmu_sources
	Source.cpp
    Receiver.cpp
    c37118.cpp
    tcpHelperClasses.cpp
    StableSource.cpp
	)

set(pmu_headers
	c37118.h
	Source.hpp
    Receiver.hpp
    tcpHelperClasses.h
    fmt_ostream.h
    fmt_format.h
    GuardedTypes.hpp
    StableSource.hpp
	)



add_library(pmu ${pmu_sources} ${pmu_headers})

target_link_libraries(pmu helics_pmu_base)

