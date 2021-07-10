/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "c37118.h"
#include "TcpHelperClasses.h"

namespace pmu
{
	class Receiver
{
      public:
        std::string address;
        std::string port;

        c37118::Config config;

        helics::tcp::TcpConnection::pointer connection;

        std::vector<std::uint8_t> buffer;

        bool connect(asio::io_context &io_context);
        bool getConfig();
	};
}