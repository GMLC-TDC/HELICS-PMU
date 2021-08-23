/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include <gtest/gtest.h>
#include "PcapPacketParser.h"
#include "../src/pmu/c37118.h"
#include "../src/pmu/configure.hpp"
#include <filesystem>

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

struct PMU2_TCP : public ::testing::Test
{
  public:
    PcapPacketParser p;
    PMU2_TCP() : p(TEST_DIR "/C37.118_2PMUsInSync_TCP.pcap") {}
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

TEST_F(PMU_TCP, data_generation)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 1);
   
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg), ParseResult::parse_complete);

    const auto &pktData = p.getPacketMatch(sync_lead, 3);
    auto pktType = getPacketType(pktData.data(), pktData.size());
    EXPECT_EQ(pktType, PmuPacketType::data);
    auto pdf = parseDataFrame(pktData.data(), pktData.size(), cfg);
    EXPECT_EQ(pdf.pmus.size(), 1U);
    EXPECT_EQ(pdf.pmus[0].phasors.size(), 4U);

    std::vector<std::uint8_t> buffer;
    buffer.resize(1024);
    //this is too small to generate
    auto size = generateDataFrame(buffer.data(), 22, cfg, pdf);

    EXPECT_EQ(size, 0U);
    // now actually try to generate it
    size = generateDataFrame(buffer.data(), 1024, cfg, pdf);
    EXPECT_EQ(size, pktData.size());
    buffer.resize(size);
    bool match = true;
    for (size_t ii = 0; ii < std::min(buffer.size(), pktData.size()); ++ii)
    {
        if (buffer[ii] != pktData[ii])
        {
            std::cout << " byte [" << std::dec << ii << "] does not match pkt=" << std::hex
                      << static_cast<std::uint32_t>(pktData[ii])
                      << " buffer=" << static_cast<std::uint32_t>(buffer[ii]) << std::dec << std::endl;
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

TEST_F(PMU4_TCP, data_generation)
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

    auto &pktData = p.getPacket(7);
    pktType = getPacketType(pktData.data(), pktData.size());
    ASSERT_EQ(pktType, PmuPacketType::data);

    auto data = parseDataFrame(pktData.data(), pktData.size(), cfg);

    std::vector<std::uint8_t> dataBuffer;
    dataBuffer.resize(4096);

    auto size = generateDataFrame(dataBuffer.data(), 4096, cfg, data);
    EXPECT_EQ(size, pktData.size());
    dataBuffer.resize(size);
    bool match = true;
    for (size_t ii = 0; ii < std::min(dataBuffer.size(), pktData.size()); ++ii)
    {
        if (dataBuffer[ii] != pktData[ii])
        {
            std::cout << " byte [" << std::dec << ii << "] does not match pkt=" << std::hex
                      << static_cast<std::uint32_t>(pktData[ii])
                      << " buffer=" << static_cast<std::uint32_t>(dataBuffer[ii]) << std::dec << std::endl;
            match = false;
        }
    }

    EXPECT_TRUE(match);
}


TEST_F(PMU2_TCP, data_generation)
{
    const auto &pkt1 = p.getPacketMatch(sync_lead, 1);
    auto pktType = getPacketType(pkt1.data(), pkt1.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg1;
    auto result = parseConfig2(pkt1.data(), pkt1.size(), cfg1);
    ASSERT_EQ(result, ParseResult::parse_complete);

    const auto &pkt2 = p.getPacketMatch(sync_lead, 4);
    pktType = getPacketType(pkt2.data(), pkt2.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg2;
    result = parseConfig2(pkt2.data(), pkt2.size(), cfg2);
    ASSERT_EQ(result, ParseResult::parse_complete);

    int jj = 5;
    std::size_t psize{10};
    while (psize!=0)
    {
        const auto &pktData = p.getPacketMatch(sync_lead, jj++);
        psize = pktData.size();
        if (psize==0)
        {
            continue;
        }
        pktType = getPacketType(pktData.data(), pktData.size());
        if (pktType != PmuPacketType::data)
        {
            continue;
        }
        auto id = getIdCode(pktData.data(), pktData.size());
        const Config &cfg = (id == cfg1.idcode) ? cfg1 : cfg2;
        auto data = parseDataFrame(pktData.data(), pktData.size(), cfg);

        std::vector<std::uint8_t> dataBuffer;
        dataBuffer.resize(4096);

        auto sz = getPacketSize(pktData.data(), pktData.size());
        auto size = generateDataFrame(dataBuffer.data(), 4096, cfg, data);
        EXPECT_EQ(size, sz);
        dataBuffer.resize(size);
        bool match = true;
        for (size_t ii = 0; ii < std::min(dataBuffer.size(), static_cast<size_t>(sz)); ++ii)
        {
            if (dataBuffer[ii] != pktData[ii])
            {
                std::cout << " byte [" << std::dec << ii << "] does not match pkt=" << std::hex
                          << static_cast<std::uint32_t>(pktData[ii])
                          << " buffer=" << static_cast<std::uint32_t>(dataBuffer[ii]) << std::dec << std::endl;
                match = false;
            }
        }
        EXPECT_TRUE(match);
        if (!match)
        {
 
            std::cout << "packet " << jj << " failed config " << ((id == cfg1.idcode) ? 1 : 2) << "\n";
            psize = 0;
        }
            
    }
    
}


TEST_F(PMU_TCP, json_config_generation)
{
    const auto &pkt = p.getPacketMatch(sync_lead, 1);
    auto pktType = getPacketType(pkt.data(), pkt.size());
    EXPECT_EQ(pktType, PmuPacketType::config2);
    Config cfg;
    EXPECT_EQ(parseConfig2(pkt.data(), pkt.size(), cfg), ParseResult::parse_complete);

    std::string fileName{"configTest.json"};
    writeConfig(fileName, cfg);

    EXPECT_TRUE(std::filesystem::exists(fileName));

    Config cfg2 = loadConfig(fileName);
    cfg2.soc = cfg.soc;
    cfg2.fracsec = cfg.fracsec;

    

    std::vector<std::uint8_t> buffer;
    buffer.resize(1024);
    //make sure the json store and load matches the original config
    auto size = generateConfig2(buffer.data(), 1024, cfg2);
    EXPECT_EQ(size, pkt.size());
    buffer.resize(size);
    bool match = true;
    for (size_t ii = 0; ii < std::min(buffer.size(), pkt.size()); ++ii)
    {
        if (buffer[ii] != pkt[ii])
        {
            std::cout << " byte [" << std::dec << ii << "] of " << pkt.size() << " does not match pkt =" << std::hex
                      << static_cast<std::uint32_t>(pkt[ii])
                      << " buffer=" << static_cast<std::uint32_t>(buffer[ii]) << std::dec << std::endl;
            match = false;
        }
    }

    EXPECT_TRUE(match);

    std::filesystem::remove(fileName);
}