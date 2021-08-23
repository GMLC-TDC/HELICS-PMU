/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include "c37118.h"

namespace c37118
{
	Config loadConfig(const std::string &configStr);

	void writeConfig(const std::string &configFile, const Config &config);
 }