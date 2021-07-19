/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "pmu.hpp"
namespace pmu
{

	bool Receiver::connect(asio::io_context &io_context)
{
		connection = helics::tcp::TcpConnection::create(io_context, address, port, 65536);
		connection->waitUntilConnected(std::chrono::milliseconds(4000));
		buffer.resize(65536);
        return true;
	}

	bool Receiver::getConfig() {
        
        auto size=c37118::generateCommand(buffer.data(), 65536, c37118::PmuCommand::send_config2, 345);
        connection->send(buffer.data(), size);
        auto sz=connection->receive(buffer.data(), 65536);
        c37118::Config cfg;
        return c37118::parseConfig2(buffer.data(), sz, config) == c37118::ParseResult::parse_complete;
	}
    }