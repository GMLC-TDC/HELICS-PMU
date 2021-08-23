/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "Source.hpp"

namespace pmu
{
	class StableSource: public Source
	{
      public:
        c37118::PmuDataFrame stableData;
        c37118::Config stableConfig;

         virtual void loadConfig(const std::string &configStr) override;

        virtual void loadDataFrame(const c37118::Config &dataConfig,
                                   c37118::PmuDataFrame &frame,
                                   std::chrono::time_point<std::chrono::system_clock> current_time) override;
	};
}