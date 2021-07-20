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

TEST_F(PMU_TCP, command_test)
{ 
    auto cnt = p.packetCount();
    EXPECT_GT(cnt, 0);
    size_t cntAA{0};
    size_t bct{0};
    do
    {
        const auto &pkt = p.getPacketMatch(sync_lead, cntAA);    
        
       
        bct = pkt.size();
        if (bct>0)
        {
            ++cntAA;
        }
    } while (bct > 0);
    EXPECT_GT(cntAA, 0);

    const auto &pkt = p.getPacketMatch(sync_lead,0);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::command);
    CommonFrame cf;
    parseCommon(pkt.data(), pkt.size(), cf);
    EXPECT_EQ(cf.type, PmuPacketType::command);

    auto cmd = parseCommand(pkt.data(), pkt.size());
    EXPECT_EQ(cmd, PmuCommand::send_config2);

}

TEST_F(PMU_TCP, config1_test) 
{ 
    const auto &pkt = p.getPacketMatch(sync_lead,1);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg), ParseResult::parse_complete);
    ASSERT_EQ(cfg.pmus.size(), 1U);
    EXPECT_EQ(cfg.pmus[0].phasorCount, 4U);
}

TEST_F(PMU_TCP, data_on_test)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 2);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::command);
    auto cmd = parseCommand(pkt.data(), pkt.size());
    EXPECT_EQ(cmd, PmuCommand::data_on);

}

TEST_F(PMU_TCP, data_one_test)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 1);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg),ParseResult::parse_complete);

    const auto &pktData = p.getPacketMatch(sync_lead, 3);
    auto pktType = getPacketType(pktData.data(), pktData.size());
    EXPECT_EQ(pktType, PmuPacketType::data);
    auto pdf = parseDataFrame(pktData.data(), pktData.size(), cfg);
    EXPECT_EQ(pdf.pmus.size(), 1U);
    EXPECT_EQ(pdf.pmus[0].phasors.size(), 4U);
}

TEST_F(PMU_TCP, data_sequence_test)
{

    const auto &pkt = p.getPacketMatch(sync_lead, 1);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg),ParseResult::parse_complete);

    std::uint32_t soc{0};
    float fracSec{0};
    for (std::size_t ii=0;ii<p.packetCount()-1;++ii)
    {
        const auto &pktData = p.getPacketMatch(sync_lead, 3 + ii);
        auto pktType = getPacketType(pktData.data(), pktData.size());
        if (pktType!= PmuPacketType::data)
        {
            continue;
        }
        auto pdf = parseDataFrame(pktData.data(), pktData.size(), cfg);
        if (ii==0)
        {
            soc = pdf.soc;
            fracSec = pdf.fracSec;
        }
        else
        {
            EXPECT_TRUE(pdf.soc > soc || (pdf.soc == soc && pdf.fracSec > fracSec));
            if (pdf.soc==soc)
            {
                auto diff = pdf.fracSec - fracSec;
                EXPECT_GT(diff, 0.015);
            }
            soc = pdf.soc;
            fracSec = pdf.fracSec;
        }
    }
    
}

/* UDP packet tests*/

TEST_F(PMU_UDP, command_test)
{
    auto cnt = p.packetCount();
    EXPECT_GT(cnt, 0);
    size_t cntAA{0};
    size_t bct{0};
    do
    {
        const auto &pkt = p.getPacketMatch(sync_lead, cntAA);

        bct = pkt.size();
        if (bct > 0)
        {
            ++cntAA;
        }
    } while (bct > 0);
    EXPECT_GT(cntAA, 0);

    const auto &pkt = p.getPacketMatch(sync_lead, 1);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::command);
    CommonFrame cf;
    parseCommon(pkt.data(), pkt.size(), cf);
    EXPECT_EQ(cf.type, PmuPacketType::command);

    auto cmd = parseCommand(pkt.data(), pkt.size());
    EXPECT_EQ(cmd, PmuCommand::send_config2);
}

TEST_F(PMU_UDP, config1_test)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 2);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg),ParseResult::parse_complete);
    ASSERT_EQ(cfg.pmus.size(), 1U);
    EXPECT_EQ(cfg.pmus[0].phasorCount, 3U);
    EXPECT_EQ(cfg.pmus[0].digitalWordCount, 1U);
}

TEST_F(PMU_UDP, data_on_test)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 3);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::command);
    auto cmd = parseCommand(pkt.data(), pkt.size());
    EXPECT_EQ(cmd, PmuCommand::data_on);
}

TEST_F(PMU_UDP, data_one_test)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 2);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg),ParseResult::parse_complete);

    const auto &pktData = p.getPacketMatch(sync_lead, 4);
    auto pktType = getPacketType(pktData.data(), pktData.size());
    EXPECT_EQ(pktType, PmuPacketType::data);
    auto pdf = parseDataFrame(pktData.data(), pktData.size(), cfg);
    EXPECT_EQ(pdf.pmus.size(), 1U);
    EXPECT_EQ(pdf.pmus[0].phasors.size(), 3U);
    EXPECT_EQ(pdf.pmus[0].digital.size(), 1U);
}

TEST_F(PMU_UDP, data_sequence_test)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 2);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg), ParseResult::parse_complete);

    std::uint32_t soc{0};
    float fracSec{0};
    for (std::size_t ii = 0; ii < p.packetCount() - 1; ++ii)
    {
        const auto &pktData = p.getPacketMatch(sync_lead, 4 + ii);
        auto pktType = getPacketType(pktData.data(), pktData.size());
        if (pktType != PmuPacketType::data)
        {
            continue;
        }
        auto pdf = parseDataFrame(pktData.data(), pktData.size(), cfg);
        if (ii == 0)
        {
            soc = pdf.soc;
            fracSec = pdf.fracSec;
        }
        else
        {
            EXPECT_TRUE(pdf.soc > soc || (pdf.soc == soc && pdf.fracSec > fracSec));
            if (pdf.soc == soc)
            {
                auto diff = pdf.fracSec - fracSec;
                EXPECT_GT(diff, 0.015);
            }
            soc = pdf.soc;
            fracSec = pdf.fracSec;
        }
    }
}


TEST_F(PMU4_TCP, config1_test)
{
    const auto &pkt = p.getPacket( 4);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    auto result = parseConfig2(pkt.data(), pkt.size(), cfg);
    if (result==ParseResult::length_mismatch)
    {
        std::vector<std::uint8_t> buffer(pkt.begin(), pkt.end());
        buffer.insert(buffer.end(),p.getPacket(5).begin(), p.getPacket(5).end());
        result = parseConfig2(buffer.data(), buffer.size(), cfg);
    }
    ASSERT_EQ(cfg.pmus.size(), 4U);
    EXPECT_EQ(cfg.pmus[0].phasorCount, 3U);
    EXPECT_EQ(cfg.pmus[0].digitalWordCount, 1U);
}

TEST_F(PMU4_TCP, data_test)
{
    const auto &pkt = p.getPacket(4);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    auto result = parseConfig2(pkt.data(), pkt.size(), cfg);
    if (result == ParseResult::length_mismatch)
    {
        std::vector<std::uint8_t> buffer(pkt.begin(), pkt.end());
        buffer.insert(buffer.end(), p.getPacket(5).begin(), p.getPacket(5).end());
        result = parseConfig2(buffer.data(), buffer.size(), cfg);
    }
    ASSERT_EQ(cfg.pmus.size(), 4U);
    EXPECT_EQ(cfg.pmus[0].phasorCount, 3U);
    EXPECT_EQ(cfg.pmus[0].digitalWordCount, 1U);

    auto &pktData = p.getPacket(7);
    pktType = getPacketType(pktData.data(), pktData.size());
    ASSERT_EQ(pktType, PmuPacketType::data);

    auto data = parseDataFrame(pktData.data(), pktData.size(), cfg);
    EXPECT_NE(data.parseResult,ParseResult::length_mismatch);
}

TEST(header, headerGeneration)
{
    Config cfg;
    cfg.idcode = 786;
    std::vector<std::uint8_t> buffer;
    buffer.resize(600);

    std::string headerString = "this is a header string lalala!!!";

    auto bsize = generateHeader(buffer.data(), 600, headerString, cfg);
    EXPECT_GT(bsize, 10 + headerString.size());

    auto id = getIdCode(buffer.data(), bsize);
    EXPECT_EQ(id, 786);

    auto type = getPacketType(buffer.data(),bsize);
    EXPECT_EQ(type, PmuPacketType::header);

    auto str = parseHeader(buffer.data(), bsize);
    EXPECT_EQ(str, headerString);
}