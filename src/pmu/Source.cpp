/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "Source.hpp"

#include "JsonProcessingFunctions.hpp"
#include "configure.hpp"
#include <iostream>
#include "StableSource.hpp"

namespace pmu
{
void Source::loadConfig(const std::string &configStr) { mConfig = c37118::loadConfig(configStr); }

void Source::fillDataFrame(c37118::PmuDataFrame &frame, std::chrono::nanoseconds frame_time)
{
    loadDataFrame(mConfig, frame, frame_time);
}

std::unique_ptr<Source> generateSource(const std::string &configFile)
{
    auto jv = c37118::fileops::loadJsonStr(configFile);

    std::string type = "stable";
    if (jv.isMember("source"))
    {
        c37118::fileops::replaceIfMember(jv["source"], "type", type);
    }
    return generateSource(type, configFile);
}

std::unique_ptr<Source> generateSource(const std::string &type, const std::string &configFile) { 
    std::unique_ptr<Source> src;
    if (type == "stable")
    {
        src= std::make_unique<StableSource>();
    }
    else if (type == "modulated")
    {
    
    }
    if (src)
    {
        src->loadConfig(configFile);
    }
    
    return src;
}

}  // namespace pmu
