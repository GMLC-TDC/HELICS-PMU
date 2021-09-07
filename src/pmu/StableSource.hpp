/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "Source.hpp"

namespace pmu
{
	class StableSource: public Source
	{
      protected:
        c37118::PmuDataFrame mStableData;

      public:
        void setData(const c37118::PmuDataFrame &data) { mStableData = data; }
         virtual void loadConfig(const std::string &configStr) override;

        virtual void loadDataFrame(const c37118::Config &dataConfig,
                                   c37118::PmuDataFrame &frame,
                                    std::chrono::nanoseconds current_time) override;
	};
}