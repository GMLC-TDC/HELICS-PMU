# LLNS Copyright Start
# Copyright (c) 2017, Lawrence Livermore National Security
# This work was performed under the auspices of the U.S. Department
# of Energy by Lawrence Livermore National Laboratory in part under
# Contract W-7405-Eng-48 and in part under Contract DE-AC52-07NA27344.
# Produced at the Lawrence Livermore National Laboratory.
# All rights reserved.
# For details, see the LICENSE file.
# LLNS Copyright End

set(pmu_sources
	pmu.cpp
    c37118.cpp
    tcpHelperClasses.cpp
	)

set(pmu_headers
	c37118.h
	pmu.hpp
    tcpHelperClasses.h
    fmt_ostream.h
    fmt_format.h
    GuardedTypes.hpp
	)



add_library(pmu ${pmu_sources} ${pmu_headers})

target_link_libraries(pmu helics_pmu_base)

