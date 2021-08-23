/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "Source.hpp"

#include <iostream>
#include "configure.hpp"

namespace pmu
{

    void Source::loadConfig(const std::string &configStr) { 
        config=c37118::loadConfig(configStr);
    }
    
}