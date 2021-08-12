/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "Receiver.hpp"
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
        
        auto size=c37118::generateCommand(buffer.data(), 65536, c37118::PmuCommand::send_config2, idCode);
        connection->send(buffer.data(), size);
        auto sz=connection->receive(buffer.data(), 65536);
        c37118::ParseResult pr;
        for (pr=c37118::parseConfig2(buffer.data(), sz, config);pr==c37118::ParseResult::length_mismatch;)
        {
            auto sz2 = connection->receive(buffer.data()+sz, 65536);
            sz += sz2;
        }
        return (pr == c37118::ParseResult::parse_complete);
	}

    void Receiver::startData() {
        auto size = c37118::generateCommand(buffer.data(), 65536, c37118::PmuCommand::data_on, idCode);
        connection->send(buffer.data(), size);
    }

    void Receiver::stopData()
    {
        auto size = c37118::generateCommand(buffer.data(), 65536, c37118::PmuCommand::data_off, idCode);
        connection->send(buffer.data(), size);
    }

    }