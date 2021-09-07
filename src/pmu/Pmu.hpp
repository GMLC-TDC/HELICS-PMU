/*
Copyright (c) 2017-2020,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"
#include "AsioContextManager.h"

#include <chrono>
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
    std::shared_ptr<Source> mSource;
    
    double TimeMultiplier{1.0};

    std::vector<std::function<void(const c37118::PmuDataFrame &pdf)>> callbacks;

  private:
    std::shared_ptr<asio::io_context> mContext;
    std::thread executionThread;
    asio::steady_timer mTimer;
    decltype(std::chrono::steady_clock::now()) start_time;
    std::chrono::nanoseconds clock_time;
    std::shared_ptr<AsioContextManager> contextPtr;  //!< context manager to for handling real time operations
    decltype(contextPtr->startContextLoop()) loopHandle;  //!< loop controller for async real time operations
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

  protected:
    virtual std::chrono::nanoseconds getClockTime();
};
}  // namespace pmu
