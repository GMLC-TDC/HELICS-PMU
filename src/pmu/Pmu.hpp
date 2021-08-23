/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "TcpHelperClasses.h"
#include <memory>
#include "Source.hpp"

namespace pmu
{


	class TcpPmu
{
      protected:
        std::string interface;
        std::string port{"4712"};

        std::shared_ptr<Source> source;

        std::chrono::time_point<std::chrono::system_clock> start_time;
        double TimeMultiplier{1.0};

      private:
        helics::tcp::TcpConnection::pointer connection;

        std::vector<std::uint8_t> buffer;

      public:
       void startServer(asio::io_context &io_context);
	};
}
