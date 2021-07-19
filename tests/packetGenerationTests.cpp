/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include <gtest/gtest.h>
#include "PcapPacketParser.h"
#include "../src/pmu/c37118.h"

using namespace c37118;


struct PMU_TCP : public ::testing::Test
{
  public:
    PcapPacketParser p;
    PMU_TCP() : p(TEST_DIR "/C37.118_1PMU_TCP.pcap") {}

};

struct PMU_UDP : public ::testing::Test
{
  public:
    PcapPacketParser p;
    PMU_UDP() : p(TEST_DIR "/C37.118_1PMU_UDP.pcap") {}
};

struct PMU4_TCP : public ::testing::Test
{
  public:
    PcapPacketParser p;
    PMU4_TCP() : p(TEST_DIR "/C37.118_4in1PMU_TCP.pcap") {}
};

TEST_F(PMU_TCP, command_generation)
{ 

    const auto &pkt = p.getPacketMatch(sync_lead,0);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::command);
    CommonFrame cf;
    parseCommon(pkt.data(), pkt.size(), cf);
    EXPECT_EQ(cf.type, PmuPacketType::command);

    auto cmd = parseCommand(pkt.data(), pkt.size());
    EXPECT_EQ(cmd, PmuCommand::send_config2);

    std::vector<std::uint8_t> buffer;
    buffer.resize(1024);

    auto size = generateCommand(buffer.data(), 1024, PmuCommand::send_config2, cf.sourceID);

    EXPECT_EQ(size, pkt.size());
    buffer.resize(size);
    EXPECT_EQ(pkt, buffer);
}


TEST_F(PMU_TCP, config2_generation)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 1);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg), ParseResult::parse_complete);
    

    std::vector<std::uint8_t> buffer;
    buffer.resize(1024);

    auto size = generateConfig2(buffer.data(), 1024, cfg);
    EXPECT_EQ(size, pkt.size());
    buffer.resize(size);
    bool match = true;
    for (size_t ii=0;ii<std::min(buffer.size(),pkt.size());++ii)
    {
        if (buffer[ii]!=pkt[ii])
        {
            std::cout << " byte [" <<std::dec<< ii << "] does not match pkt=" <<std::hex<< static_cast<std::uint32_t>(pkt[ii]) << " buffer=" << static_cast<std::uint32_t>(buffer[ii])
                      << std::dec<<std::endl;
            match = false;
        }
    }
    
    EXPECT_TRUE(match);

}


TEST_F(PMU4_TCP, config2_generation4)
{
    const auto &pkt = p.getPacket(4);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    auto result = parseConfig2(pkt.data(), pkt.size(), cfg);
    std::vector<std::uint8_t> buffer(pkt.begin(), pkt.end());
    if (result == ParseResult::length_mismatch)
    {
        buffer.insert(buffer.end(), p.getPacket(5).begin(), p.getPacket(5).end());
        result = parseConfig2(buffer.data(), buffer.size(), cfg);
    }
    ASSERT_EQ(cfg.pmus.size(), 4U);
    EXPECT_EQ(cfg.pmus[0].phasorCount, 3U);
    EXPECT_EQ(cfg.pmus[0].digitalWordCount, 1U);

    std::vector<std::uint8_t> buffer2;
    buffer2.resize(2*buffer.size());

    auto size = generateConfig2(buffer2.data(), 2048, cfg);
    EXPECT_EQ(size, 0U);

    size = generateConfig2(buffer2.data(), 2*buffer.size(), cfg);
    EXPECT_EQ(size, buffer.size());

    buffer2.resize(size);
    bool match = true;
    for (size_t ii = 0; ii < std::min(buffer2.size(), buffer.size()); ++ii)
    {
        if (buffer2[ii] != buffer[ii])
        {
            std::cout << " byte [" << std::dec << ii << "] does not match pkt=" << std::hex
                      << static_cast<std::uint32_t>(buffer[ii])
                      << " buffer=" << static_cast<std::uint32_t>(buffer2[ii]) << std::dec << std::endl;
            match = false;
        }
    }

    EXPECT_TRUE(match);
}