/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "PcapPacketParser.h"
#include <asio.hpp>
#include <cstdio>
#include <cstdlib>

// From https://wiki.wireshark.org/Development/LibpcapFileFormat#Global_Header

typedef struct pcap_global_header
{
    std::uint32_t magic_number; /* magic number */
    std::uint16_t version_major; /* major version number */
    std::uint16_t version_minor; /* minor version number */
    std::int32_t thiszone; /* GMT to local correction */
    std::uint32_t sigfigs; /* accuracy of timestamps */
    std::uint32_t snaplen; /* max length of captured packets, in octets */
    std::uint32_t network; /* data link type */
} pcap_global_header;


typedef struct pcap_packet_header
{
    std::uint32_t ts_sec; /* timestamp seconds */
    std::uint32_t ts_usec; /* timestamp microseconds */
    std::uint32_t incl_len; /* number of octets of packet saved in file */
    std::uint32_t orig_len; /* actual length of packet */
} pcaprec_hdr_t;

struct ether_header
{
    std::uint8_t dstMac[6];
    std::uint8_t srcMac[6];
    std::uint16_t etherType;
};


struct ipv4_header
{

    std::uint8_t internetHeaderLength : 4, ipVersion : 4;
    std::uint8_t typeOfService;
    std::uint16_t totalLength;
    std::uint16_t ipId;
    std::uint16_t fragmentOffset;
    std::uint8_t timeToLive;
    std::uint8_t protocol;
    std::uint16_t headerChecksum;
    std::uint32_t ipSrc;
    std::uint32_t ipDst;
    /*The options start here. */
};


struct tcp_header
{
    std::uint16_t portSrc;
    std::uint16_t portDst;
    std::uint32_t sequenceNumber;
    std::uint32_t ackNumber;

    std::uint16_t reserved : 4, dataOffset : 4, finFlag : 1, synFlag : 1, rstFlag : 1, pshFlag : 1, ackFlag : 1,
      urgFlag : 1, eceFlag : 1, cwrFlag : 1;

    std::uint16_t windowSize;
    std::uint16_t headerChecksum;
    std::uint16_t urgentPointer;
};

struct udp_header
{
    uint16_t portSrc;
    uint16_t portDst;
    uint16_t length;
    uint16_t headerChecksum;
};

const std::vector<std::uint8_t> PcapPacketParser::emptyBuffer;

PcapPacketParser::PcapPacketParser(const std::string &fileName)
{
    int count{0};

    /* open file */
    FILE *file = fopen(fileName.c_str(), "rb");
    if (file==nullptr)
    {
        throw(std::exception("unable to open file"));
    }
    /* read file header */
    struct pcap_global_header gheader;

    fread(&gheader, sizeof(char), sizeof(struct pcap_global_header), file);

    // if not ethernet type
    if (gheader.network != 1)
    {
        throw(std::exception("invalid pcap format"));
    }

    /* read packets */

    struct pcap_packet_header pheader;
    struct ether_header eth;
    struct ipv4_header ip;
    struct tcp_header tcp;
    struct udp_header udp;

    std::uint8_t optionsBuffer[64];
    fread(&pheader, sizeof(char), sizeof(struct pcap_packet_header), file);
    while (!feof(file))
    {
        ++count;

        auto bytes_read = fread(&eth, sizeof(char), sizeof(struct ether_header), file);

        // ip
        if (eth.etherType == 0x08)
        {
            bytes_read += fread(&ip, sizeof(char), sizeof(struct ipv4_header), file);

            // tcp
            if (ip.protocol == 0x06)
            {
                bytes_read += fread(&tcp, sizeof(char), sizeof(struct tcp_header), file);
                bytes_read +=
                  fread(&optionsBuffer, sizeof(char), tcp.dataOffset * 4 - sizeof(struct tcp_header), file);
            }
            else if (ip.protocol == 0x11)
            {
                bytes_read += fread(&udp, sizeof(char), sizeof(struct udp_header), file);
            }
        }
        if (pheader.incl_len-bytes_read>0U)
        {
            std::vector<std::uint8_t> buffer;
            buffer.resize(pheader.incl_len - bytes_read);
            // read rest of the packet
            fread(buffer.data(), sizeof(char), pheader.incl_len - bytes_read, file);
            packets.push_back(std::move(buffer));
        }
        

        // read next packet's header
        fread(&pheader, sizeof(char), sizeof(struct pcap_packet_header), file);
    }
    fclose(file);
   
}

const std::vector<std::uint8_t> &PcapPacketParser::getPacket(size_t index) const
{
    if (index < packets.size())
    {
        return packets[index];
    }
    return emptyBuffer;
}


const std::vector<std::uint8_t> &PcapPacketParser::getPacketMatch(std::uint8_t byte1,
                                                         size_t index) const
{
    size_t cnt{0};
    for (const auto &buf : packets)
    {
        if (buf.size() > 2)
        {
            if (buf[0] == byte1)
            {
                if (cnt == index)
                {
                    return buf;
                }
                ++cnt;
            }
        }
    }
    return emptyBuffer;
}

    const std::vector<std::uint8_t> &PcapPacketParser::getPacketMatch(std::uint8_t byte1,
                                                         std::uint8_t byte2,
                                                         size_t index) const
{
    size_t cnt{0};
    for (const auto &buf : packets)
    {
        if (buf.size()>2)
        {
            if (buf[0]==byte1 && buf[1]==byte2)
            {
                if (cnt == index)
                {
                    return buf;
                }
                ++cnt;
            }
        }
    }
    return emptyBuffer;
}
