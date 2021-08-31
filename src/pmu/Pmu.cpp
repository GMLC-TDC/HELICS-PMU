/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "Pmu.hpp"


#include <asio/io_context.hpp>
#include <asio/dispatch.hpp>
#include <asio/strand.hpp>

#include <iostream>

namespace pmu
{
	Pmu::Pmu(const std::string& configStr) {

	}

	Pmu::Pmu(const std::string& configStr, std::shared_ptr<asio::io_context> context) {

}

	Pmu::~Pmu() {}

    void Pmu::start() {

}
void Pmu::startThread() {

}

}