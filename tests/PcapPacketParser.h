/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include <vector>
#include <string>


class PcapPacketParser
{
  public:
    /** read a pcap file to get at payload data*/
    PcapPacketParser(const std::string &file);

    const std::vector<std::uint8_t> &getPacket(size_t index) const;
    const std::vector<std::uint8_t> &getPacketMatch(std::uint8_t byte1, std::uint8_t byte2, size_t index) const;
    const std::vector<std::uint8_t> &getPacketMatch(std::uint8_t byte1, size_t index) const;
    size_t packetCount() const { return packets.size(); }
  private:
    std::vector<std::vector<std::uint8_t>> packets;
    static const std::vector<std::uint8_t> emptyBuffer;
};