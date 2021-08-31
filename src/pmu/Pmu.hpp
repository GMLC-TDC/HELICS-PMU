/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "asio/io_context.hpp"
#include <memory>
#include "Source.hpp"
#include <vector>
#include <functional>
#include <thread>

namespace pmu
{
class Pmu
{
  protected:
    std::shared_ptr<Source> source;
    std::chrono::time_point<std::chrono::system_clock> start_time;
    double TimeMultiplier{1.0};

    std::vector<std::function<void(const c37118::PmuDataFrame &pdf)>> callbacks;

    std::chrono::time_point<std::chrono::system_clock> start_time;
    double TimeMultiplier{1.0};

  private:
    std::shared_ptr<asio::io_context> mContext;
    std::thread executionThread;

  public:
    Pmu();
    explicit Pmu(const std::string &configStr);
    Pmu(const std::string &configStr, std::shared_ptr<asio::io_context> context);

    virtual ~Pmu();

    std::size_t addCallback(std::function<void(const c37118::PmuDataFrame &pdf)> cback)
    {
        callbacks.push_back(std::move(cback));
        return callbacks.size() - 1;
    }

    asio::io_context &getContext() { return *mContext; }

    void start();
    void startThread();
};
}  // namespace pmu
