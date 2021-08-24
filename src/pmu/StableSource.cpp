/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "StableSource.hpp"
#include "JsonProcessingFunctions.hpp"
#include "configure.hpp"

namespace pmu
{

    void StableSource::loadConfig(const std::string &configStr) { 
        auto jv = c37118::fileops::loadJson(configStr);
        config=c37118::loadConfigJson(jv);
        if (jv.isMember("default"))
        {
            stableData = c37118::loadDataFrame(jv["default"],false);
        }
        else if (jv.isMember("data"))
        {
            stableData = c37118::loadDataFrame(jv["data"],false);
        }
        
    }

    void StableSource::loadDataFrame(const c37118::Config &dataConfig,
                                     c37118::PmuDataFrame &frame,
                                     std::chrono::time_point<std::chrono::system_clock> current_time)
    {
        frame = stableData;
        auto tc = c37118::generateTimeCodes(current_time, dataConfig);
        frame.soc = tc.first;
        frame.idcode = tc.second;
        
    }

}  // namespace pmu