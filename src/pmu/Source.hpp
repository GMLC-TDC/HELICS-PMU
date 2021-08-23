/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "c37118.h"

#include <chrono>

namespace pmu
{

    class Source
    {
      protected:
        c37118::Config config;

      public:
        /** load the system configuration*/
        virtual void loadConfig(const std::string &configStr);

        virtual void loadDataFrame(const c37118::Config &dataConfig,
                                   c37118::PmuDataFrame &frame,
                                   std::chrono::time_point<std::chrono::system_clock> current_time) = 0;
    };
	
}
