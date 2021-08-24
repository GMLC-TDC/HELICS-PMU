/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include "c37118.h"
#include <vector>

namespace Json
{
	class Value;
}
namespace c37118
{
	Config loadConfig(const std::string &configStr);
	/** load directly from a json value type meant for cascading */
	Config loadConfigJson(const Json::Value &jv);

	void writeConfig(const std::string &configFile, const Config &config);

	PmuDataFrame loadDataFrame(const Json::Value &jv,bool checkObject=true);

	void writeDataJson(Json::Value &df, const PmuDataFrame &data);
    void writeDataJson(Json::Value &df, const std::vector<PmuDataFrame> &data);

	void writeDataFile(const std::string &dataFile, const PmuDataFrame &data);
    void writeDataFile(const std::string &dataFile, const std::vector<PmuDataFrame> &data);

	const std::vector<PmuDataFrame> loadDataFile(const std::string &dataFile);
 }