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
        c37118::Config mConfig;

      public:
        virtual ~Source() = default;
        /** load the system configuration*/
        void setConfig(const c37118::Config &config) { mConfig = config; }
        virtual void loadConfig(const std::string &configStr);

        void fillDataFrame(c37118::PmuDataFrame &frame, std::chrono::nanoseconds frame_time);

      protected:
        virtual void loadDataFrame(const c37118::Config &dataConfig,
                                   c37118::PmuDataFrame &frame,
                                   std::chrono::nanoseconds frame_time) = 0;
    };
	
    std::unique_ptr<Source> generateSource(const std::string &configFile);

    std::unique_ptr<Source> generateSource(const std::string &type, const std::string &configFile);
    }
