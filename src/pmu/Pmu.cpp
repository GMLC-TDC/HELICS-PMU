/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "Pmu.hpp"
#include "asio/steady_timer.hpp"
#include "JsonProcessingFunctions.hpp"

#include <asio/dispatch.hpp>
#include <asio/strand.hpp>
#include "date/tz.h"
#include <iostream>

namespace pmu
{
Pmu::Pmu(const std::string &configStr):mContext(std::make_shared<asio::io_context>()),mTimer(*mContext) { 
		mSource = generateSource(configStr);
	}

	Pmu::Pmu(const std::string& configStr, std::shared_ptr<asio::io_context> context):mTimer(*context) {
        mSource = generateSource(configStr);
	}

	Pmu::~Pmu() {}

    void Pmu::start() { 
		start_time = std::chrono::steady_clock::now();
        clock_time = getClockTime();

		
	}

void Pmu::startThread() {}

std::chrono::nanoseconds getClockTime() { 
	return std::chrono::system_clock::now().time_since_epoch();
}

}