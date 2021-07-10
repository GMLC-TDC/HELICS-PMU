/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "c37118.h"
#include <asio/detail/socket_ops.hpp>

namespace c37118
{


   static std::uint16_t calculateCRC(const std::uint8_t *data, std::uint16_t dataLength)
{
    /** CRC-CCITT Calculation
     f(x) = x^16 + x^12 + x^5 + 1
    
    Derived from IEEE Std C37.118-2005 sample code
    */
    std::uint16_t crc = 0xFFFF;  // 0xFFFF is specific for SynchroPhasor Data CRC

    for (std::uint16_t ii = 0; ii < dataLength; ii++)
    {
        std::uint16_t calc1 = (crc >> 8) ^ data[ii];
        crc <<= 8;
        std::uint16_t calc2 = calc1 ^ (calc1 >> 4);
        crc ^= calc2;
        calc2 <<= 5;
        crc ^= calc2;
        calc2 <<= 7;
        crc ^= calc2;
    }
    return crc;
}

   static void addSize(std::uint8_t *data, std::uint16_t dataSize)
{
    memcpy(data + 2, &dataSize, 2);
}

   static void addCRC(std::uint8_t *data, std::uint16_t dataSize) { auto crc = calculateCRC(data, dataSize);
       memcpy(data + dataSize, &crc, 2);
   }


   static std::int32_t interpret24bitAsInt32(std::uint32_t val)
   {
       auto val1 = static_cast<std::uint8_t>((val >> 16) & 0xFF);
       auto val2 = static_cast<std::uint8_t>((val >> 8) & 0xFF);
       auto val3 = static_cast<std::uint8_t>((val) & 0xFF);

       return ((static_cast<std::int32_t>(val1) << 24) | (static_cast<std::int32_t>(val2) << 16) |
               (static_cast<std::int32_t>(val3) << 8)) >>
              8;
   }

   /*  parse the common information from all packets and check size and CRC */
	bool parseCommon(const std::uint8_t *data, size_t dataSize, CommonFrame &frame)
{
    if (data[0] != sync_lead)
    {
        return false;
    }
    frame.type = static_cast<PmuPacketType>(data[1] & typeMask);
    frame.byteCount = data[2]*256 + data[3];
    if (dataSize < frame.byteCount)
    {
        return false;
    }

    auto crc = calculateCRC(data, frame.byteCount-2);

    std::uint16_t actualCRC = data[frame.byteCount - 2] * 256 + data[frame.byteCount - 1];
    if (actualCRC != crc)
    {
        return false;
    }
    memcpy(&frame.sourceID, data + 4, sizeof(frame.sourceID));
    memcpy(&frame.soc, data + 6, sizeof(frame.soc));
    memcpy(&frame.fracSec, data + 10, sizeof(frame.fracSec));
    
    frame.sourceID=ntohs(frame.sourceID);
    frame.soc=ntohl(frame.soc);
    frame.fracSec=ntohl(frame.fracSec);
    return true;
}

    PmuPacketType getPacketType(const std::uint8_t* data, size_t dataSize) {
        if (dataSize==0 || data[0] != sync_lead)
        {
            return PmuPacketType::unknown;
        }
        return static_cast<PmuPacketType>(data[1] & typeMask);
    }

    static std::size_t parsePmuConfig(const std::uint8_t *data, PmuConfig &config)
    { std::size_t bytes_used{0};
        config.stationName.assign(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + 16);
        bytes_used += 16;
        memcpy(&config.sourceID, data + bytes_used, sizeof(config.sourceID));
        config.sourceID = ntohs(config.sourceID);
        bytes_used += sizeof(config.sourceID);
        
        std::uint16_t format{0U};
        memcpy(&format, data + bytes_used, sizeof(format));
        format = ntohs(format);
        config.freqFormat = ((format & 0b1000) == 0) ? integer_format : floating_point_format;
        config.analogFormat=((format & 0b0100) == 0) ? integer_format : floating_point_format;
        config.phasorFormat=((format & 0b0010) == 0) ? integer_format : floating_point_format;
        config.phasorCoordinates = ((format & 0b0001) == 0) ? rectangular_phasor : polar_phasor;

        bytes_used += sizeof(format);
        /* get the counts*/
        config.phasorCount = (static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used+1];
        bytes_used += 2;
        config.analogCount = (static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used + 1];
        bytes_used += 2;

        config.digitalWordCount = (static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used + 1];
        bytes_used += 2;
        /* get the channel names*/
        config.phasorNames.resize(config.phasorCount);
        for (int ii=0;ii<config.phasorCount;++ii)
        {
            config.phasorNames[ii].assign(reinterpret_cast<const char *>(data + bytes_used),
                                          reinterpret_cast<const char *>(data) + bytes_used + 16);
            bytes_used += 16;
        }

        config.analogNames.resize(config.analogCount);
        for (int ii = 0; ii < config.analogCount; ++ii)
        {
            config.analogNames[ii].assign(reinterpret_cast<const char *>(data + bytes_used),
                                          reinterpret_cast<const char *>(data) + bytes_used + 16);
            bytes_used += 16;
        }
        config.digitChannelNames.resize(config.digitalWordCount * 16);

        for (int ii = 0; ii < config.digitalWordCount*16; ++ii)
        {
            config.digitChannelNames[ii].assign(reinterpret_cast<const char *>(data + bytes_used),
                                          reinterpret_cast<const char *>(data) + bytes_used + 16);
            bytes_used += 16;
        }
        /* get the channel conversion information */
        config.phasorType.resize(config.phasorCount);
        config.phasorConversion.resize(config.phasorCount);
        for (int ii = 0; ii < config.phasorCount; ++ii)
        {
            config.phasorType[ii] = data[bytes_used];
            config.phasorConversion[ii] = (static_cast<std::uint32_t>(data[bytes_used + 1]) << 16) +
                                          (static_cast<std::uint32_t>(data[bytes_used + 2]) << 8) +
                                          static_cast<std::uint32_t>(data[bytes_used + 3]);
            bytes_used += 4;
        }

        config.analogType.resize(config.analogCount);
        config.analogConversion.resize(config.analogCount);
        for (int ii = 0; ii < config.analogCount; ++ii)
        {
            config.analogType[ii] = data[bytes_used];
            std::uint32_t conversion{0U};
            memcpy(&conversion, data + bytes_used, sizeof(std::uint32_t));
            conversion = ntohl(conversion);
            config.analogConversion[ii] = interpret24bitAsInt32(conversion);
           
            bytes_used += 4;
        }
        config.digitalNominal.resize(config.digitalWordCount);
        config.digitalActive.resize(config.digitalWordCount);

        for (int ii = 0; ii < config.digitalWordCount; ++ii)
        {
            config.digitalNominal[ii] = (static_cast<std::uint16_t>(data[bytes_used]) << 8) +
                                        static_cast<std::uint16_t>(data[bytes_used + 1]);
            config.digitalActive[ii] = (static_cast<std::uint16_t>(data[bytes_used+2]) << 8) +
                                        static_cast<std::uint16_t>(data[bytes_used + 3]);
            bytes_used += 4;
        }
        if (data[bytes_used+1]==1U)
        {
            config.nominalFrequency = 50.0f;
        }
        bytes_used += 2;
        config.changeCount =
          (static_cast<std::uint16_t>(data[bytes_used]) << 8) + static_cast<std::uint16_t>(data[bytes_used + 1]);
        bytes_used += 2;
        return bytes_used;
    }
	bool parseConfig1(const std::uint8_t *data, size_t dataSize, Config &config) 
    { 
        CommonFrame frame;
        if (!parseCommon(data,dataSize,frame))
        {
            return false;
        }
        if (frame.type != PmuPacketType::config1 && frame.type != PmuPacketType::config2)
        {
            return false;
        }
        config.idcode = frame.sourceID;
        config.fracsec = frame.fracSec;
        config.soc = frame.soc;
        config.timeBase = (static_cast<std::uint16_t>(data[15]) << 16) +
                          (static_cast<std::uint16_t>(data[16]) << 8) + static_cast<std::uint16_t>(data[17]);
        std::uint16_t numPmu = (static_cast<std::uint16_t>(data[18]) << 8) + data[19];
        config.pmus.resize(numPmu);
        std::size_t bytes_used = 20;
        for (int ii=0;ii<numPmu;++ii)
        {
            bytes_used+=parsePmuConfig(data + bytes_used, config.pmus[ii]);
        }
        config.dataRate = static_cast<int16_t>((static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used]);
        return true;
        
    }

bool parseConfig2(const std::uint8_t *data, size_t dataSize, Config &config) {
    return parseConfig1(data, dataSize, config);
}

bool parseConfig3(const std::uint8_t *data, size_t dataSize, Config &config) { return false; }


TimeQuality parseTimeQuality(std::uint8_t tq)
{ 
    TimeQuality timeQuality;
    timeQuality.reserved = (tq & 0b1000'0000) != 0 ? 1 : 0;
    timeQuality.leapSecondDirection = (tq & 0b0100'0000) != 0 ? 1 : 0;
    timeQuality.leapSecondOccurred = (tq & 0b0010'0000) != 0 ? 1 : 0;
    timeQuality.leapSecondPending = (tq & 0b0001'0000) != 0 ? 1 : 0;
    timeQuality.timeQualityCode = tq & 0b0000'1111;

    static constexpr float tqArray[16] = {1e-12f, 1e-9f, 1e-8f, 1e-7f,  1e-6f,   1e-5f, 1e-4f, 1e-3f, 1e-2f,
                                          1e-1f,  1.0f,  10.0f, 100.0f, 1000.0f, 1e4f,  1e7f};

    timeQuality.timeQuality = tqArray[timeQuality.timeQualityCode];
    return timeQuality;
}

std::string parseHeader(const std::uint8_t *data, size_t dataSize) 
{
    CommonFrame frame;
    if (!parseCommon(data, dataSize, frame))
    {
        return {};
    }
    std::string header(reinterpret_cast<const char *>(data) + 6,
                       reinterpret_cast<const char *>(data) + frame.byteCount - 2);
    return header;
}

PmuCommand parseCommand(const std::uint8_t *data, size_t dataSize) {
    CommonFrame frame;
    if (!parseCommon(data, dataSize, frame))
    {
        return PmuCommand::unknown;
    }
    if (frame.type != PmuPacketType::command)
    {
        return PmuCommand::unknown;
    }
    std::uint16_t commandCode = (static_cast<std::uint16_t>(data[14])<<8) + data[15];
    
    return static_cast<PmuCommand>(commandCode);

}

static std::size_t parsePmuData(const std::uint8_t* data, const PmuConfig& config, PmuData& pmuData) {
    std::size_t bytes_used{0U};
    memcpy(&pmuData.stat, data, 2U);
    pmuData.stat = ntohs(pmuData.stat);
    pmuData.phasors.resize(config.phasorCount);
    bytes_used += 2;
    for (int ii=0;ii<config.phasorCount;++ii)
    {
        if (config.phasorFormat==integer_format)
        {
            
            
            if (config.phasorCoordinates == rectangular_phasor)
            {
                std::int16_t val1;
                std::int16_t val2;
                std::memcpy(&val1, data + bytes_used, sizeof(std::int16_t));
                std::memcpy(&val2, data + bytes_used + sizeof(std::int16_t), sizeof(std::int16_t));
                val1 = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(val1)));
                val2 = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(val2)));
                pmuData.phasors[ii] =
                  std::complex<float>(static_cast<float>(static_cast<double>(val1) * 1e-5 *
                                                         static_cast<double>(config.phasorConversion[ii])),
                                      static_cast<float>(static_cast<double>(val2) * 1e-5 *
                                                         static_cast<double>(config.phasorConversion[ii])));
            }
            else
            {
                std::uint16_t val1;
                std::int16_t val2;
                std::memcpy(&val1, data + bytes_used, sizeof(std::uint16_t));
                std::memcpy(&val2, data + bytes_used + sizeof(std::uint16_t), sizeof(std::int16_t));
                val1 = ntohs(val1);
                val2 = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(val2)));
                pmuData.phasors[ii] =
                  std::polar<float>(static_cast<float>(static_cast<double>(val1) * 1e-5 *
                                                       static_cast<double>(config.phasorConversion[ii])),
                                    static_cast<float>(static_cast<double>(val2) / 1e4));
            }
            bytes_used += sizeof(std::uint16_t) + sizeof(std::int16_t);
        }
        else
        {
            std::uint32_t val1;
            std::uint32_t val2;
            std::memcpy(&val1, data + bytes_used, sizeof(float));
            std::memcpy(&val2, data + bytes_used + sizeof(float), sizeof(float));
            float val1f=ntohf(val1);
            float val2f = ntohf(val2);
            if (config.phasorCoordinates==rectangular_phasor)
            {
                pmuData.phasors[ii] = std::complex<float>(val1f, val2f);
            }
            else
            {
                pmuData.phasors[ii] = std::polar<float>(val1f, val2f);
            }
            bytes_used += 2*sizeof(float);
        }
    }

    if (config.freqFormat==floating_point_format)
    {
        std::uint32_t freqData;
        std::memcpy(&freqData, data + bytes_used, sizeof(float));
        pmuData.freq = ntohf(freqData);
        bytes_used += sizeof(float);
        std::memcpy(&freqData, data + bytes_used, sizeof(float));
        pmuData.rocof = ntohf(freqData);
        bytes_used += sizeof(float);
    }
    else
    {
        std::int16_t freq{0};
        std::memcpy(&freq, data + bytes_used, sizeof(freq));
        freq = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(freq)));
        bytes_used += sizeof(std::int16_t);
        pmuData.freq = static_cast<float>(static_cast<double>(freq) / 1000.0);
        /* rocof */
        std::memcpy(&freq, data + bytes_used, sizeof(freq));
        freq = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(freq)));
        bytes_used += sizeof(std::int16_t);
        pmuData.rocof = static_cast<float>(static_cast<double>(freq) / 1000.0);
    }

    pmuData.analog.resize(config.analogCount);
    for (int ii = 0; ii < config.analogCount; ++ii)
    {
        if (config.analogFormat == floating_point_format)
        {
            std::uint32_t analog;
            std::memcpy(&analog, data + bytes_used, sizeof(float));
            pmuData.analog[ii] = ntohf(analog);
            bytes_used += sizeof(float);
            
        }
        else
        {
            std::int16_t analog{0};
            std::memcpy(&analog, data + bytes_used, sizeof(analog));
            analog = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(analog)));
            bytes_used += sizeof(std::int16_t);
            pmuData.analog[ii] = static_cast<float>(analog);
        }
    }
    pmuData.digital.resize(config.digitalWordCount);
    for (int ii=0;ii<config.digitalWordCount;++ii)
    {
        std::memcpy(&pmuData.digital[ii], data + bytes_used, sizeof(std::uint16_t));
        pmuData.digital[ii] = ntohs(pmuData.digital[ii]);
        bytes_used += sizeof(std::uint16_t);
    }
    return bytes_used;
}

PmuDataFrame parseDataFrame(const std::uint8_t *data, size_t dataSize, const Config &config)
{
    PmuDataFrame pdf;
    CommonFrame frame;
    if (!parseCommon(data, dataSize, frame))
    {
        return pdf;
    }
    if (frame.type != PmuPacketType::data)
    {
        return pdf;
    }
    pdf.idcode = frame.sourceID;
    pdf.timeQuality = static_cast<std::uint8_t>(frame.fracSec >> 24);
    pdf.fracSec = static_cast<float>(static_cast<double>(frame.fracSec&0x00FFFFFFU) /
                                     static_cast<double>(config.timeBase));
    pdf.soc = frame.soc;
    pdf.pmus.resize(config.pmus.size());
    std::size_t bytes_used = 14U;
    for (std::size_t ii = 0; ii < config.pmus.size(); ++ii)
    {
        bytes_used += parsePmuData(data + bytes_used, config.pmus[ii], pdf.pmus[ii]);
    }
    return pdf;
}


static void generateCommonFrame(std::uint8_t *data, std::uint16_t dataSize, const Config &config, PmuPacketType type) 
{
    data[0] = sync_lead;
    data[1] = static_cast<std::uint8_t>(type);
    memcpy(data + 4, &config.idcode, 2);
}
std::uint16_t generateConfig1(std::uint8_t *data, size_t dataSize, const Config &config) { return 0; }

std::uint16_t generateConfig2(std::uint8_t *data, size_t dataSize, const Config &config) { return 0; }

std::uint16_t generateConfig3(std::uint8_t *data, size_t dataSize, const Config &config) { return 0; }

std::uint16_t generateHeader(std::uint8_t *data, size_t dataSize, const std::string &header, const Config &config)
{
    return 0;
}

std::uint16_t generateCommand(std::uint8_t *data, size_t dataSize, PmuCommand command, const Config &config)
{
    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), config, PmuPacketType::command);
    // add the actual command
    memcpy(data + 14, &command, 2);
    addSize(data, 16);
    addCRC(data, 16);
    return 18;
}

}
