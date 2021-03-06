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
    // -2 to leave space for the CRC itself
    for (std::uint16_t ii = 0; ii < dataLength - 2; ii++)
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
static void addTime(std::uint8_t *data, std::uint32_t soc, std::uint32_t fracsec)
{
    soc = htonl(soc);
    fracsec = htonl(fracsec);
    memcpy(data+6,&soc,sizeof(std::uint32_t));
    memcpy(data + 10, &fracsec, sizeof(std::uint32_t));
}

static void addTime(std::uint8_t *data, std::uint32_t soc, double fracsec, std::uint8_t timeQuality, const Config &config)
{
    soc = htonl(soc);

    auto frsec = (static_cast<std::uint32_t>(fracsec * static_cast<double>(config.timeBase)) & 0x00FFFFFFU);
    frsec += (timeQuality << 24);
    frsec = htonl(frsec);
    memcpy(data + 6, &soc, sizeof(std::uint32_t));
    memcpy(data + 10, &frsec, sizeof(std::uint32_t));
}

static void addSize(std::uint8_t *data, std::uint16_t dataSize)
{
    data[2] = static_cast<std::uint8_t>(dataSize >> 8);
    data[3] = static_cast<std::uint8_t>(dataSize & 0xFF);
}

static void addCRC(std::uint8_t *data, std::uint16_t dataSize)
{
    auto crc = htons(calculateCRC(data, dataSize));
    memcpy(data + dataSize - 2, &crc, 2);
}

static std::int32_t interpret24bitAsInt32(std::uint32_t val)
{
    auto val1 = static_cast<std::uint8_t>((val >> 16) & 0xFF);
    auto val2 = static_cast<std::uint8_t>((val >> 8) & 0xFF);
    auto val3 = static_cast<std::uint8_t>((val)&0xFF);

    return ((static_cast<std::int32_t>(val1) << 24) | (static_cast<std::int32_t>(val2) << 16) |
            (static_cast<std::int32_t>(val3) << 8)) >>
           8;
}

/*  parse the common information from all packets and check size and CRC */
ParseResult parseCommon(const std::uint8_t *data, size_t dataSize, CommonFrame &frame)
{
    if (data[0] != sync_lead)
    {
        return ParseResult::invalid_sync;
    }
    frame.type = static_cast<PmuPacketType>(data[1] & typeMask);
    frame.byteCount = data[2] * 256 + data[3];
    if (dataSize < frame.byteCount)
    {
        return ParseResult::length_mismatch;
    }

    auto crc = calculateCRC(data, frame.byteCount);

    std::uint16_t actualCRC = data[frame.byteCount - 2] * 256 + data[frame.byteCount - 1];
    if (actualCRC != crc)
    {
        return ParseResult::invalid_checksum;
    }
    memcpy(&frame.sourceID, data + 4, sizeof(frame.sourceID));
    memcpy(&frame.soc, data + 6, sizeof(frame.soc));
    memcpy(&frame.fracSec, data + 10, sizeof(frame.fracSec));

    frame.sourceID = ntohs(frame.sourceID);
    frame.soc = ntohl(frame.soc);
    frame.fracSec = ntohl(frame.fracSec);
    return ParseResult::parse_complete;
}

PmuPacketType getPacketType(const std::uint8_t *data, size_t dataSize)
{
    if (dataSize < 16 || data[0] != sync_lead)
    {
        return PmuPacketType::unknown;
    }
    return static_cast<PmuPacketType>(data[1] & typeMask);
}

std::uint16_t getIdCode(const std::uint8_t* data, size_t dataSize) {

    if (dataSize < 16 || data[0] != sync_lead)
    {
        return 0;
    }
    std::uint16_t id;
    memcpy(&id, data + 4, sizeof(std::uint16_t));
    return ntohs(id);
}

std::uint16_t getPacketSize(const std::uint8_t* data, size_t dataSize)
{ 
    if (dataSize < 16 || data[0] != sync_lead)
    {
        return 0;
    }
    std::uint16_t byteCount = data[2] * 256 + data[3];
    return byteCount;
}

static std::size_t parsePmuConfig(const std::uint8_t *data, PmuConfig &config)
{
    std::size_t bytes_used{0};
    config.stationName.assign(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + 16);
    bytes_used += 16;
    memcpy(&config.sourceID, data + bytes_used, sizeof(config.sourceID));
    config.sourceID = ntohs(config.sourceID);
    bytes_used += sizeof(config.sourceID);

    std::uint16_t format{0U};
    memcpy(&format, data + bytes_used, sizeof(format));
    format = ntohs(format);
    config.freqFormat = ((format & 0b1000) == 0) ? integer_format : floating_point_format;
    config.analogFormat = ((format & 0b0100) == 0) ? integer_format : floating_point_format;
    config.phasorFormat = ((format & 0b0010) == 0) ? integer_format : floating_point_format;
    config.phasorCoordinates = ((format & 0b0001) == 0) ? rectangular_phasor : polar_phasor;

    bytes_used += sizeof(format);
    /* get the counts*/
    config.phasorCount = (static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used + 1];
    bytes_used += 2;
    config.analogCount = (static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used + 1];
    bytes_used += 2;

    config.digitalWordCount = (static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used + 1];
    bytes_used += 2;
    /* get the channel names*/
    config.phasorNames.resize(config.phasorCount);
    for (int ii = 0; ii < config.phasorCount; ++ii)
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

    for (int ii = 0; ii < config.digitalWordCount * 16; ++ii)
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
        config.phasorType[ii] = static_cast<PhasorType>(data[bytes_used]);
        config.phasorConversion[ii] = (static_cast<std::uint32_t>(data[bytes_used + 1]) << 16) +
                                      (static_cast<std::uint32_t>(data[bytes_used + 2]) << 8) +
                                      static_cast<std::uint32_t>(data[bytes_used + 3]);
        bytes_used += 4;
    }

    config.analogType.resize(config.analogCount);
    config.analogConversion.resize(config.analogCount);
    for (int ii = 0; ii < config.analogCount; ++ii)
    {
        config.analogType[ii] = static_cast<AnalogType>(data[bytes_used]);
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
        config.digitalNominal[ii] =
          (static_cast<std::uint16_t>(data[bytes_used]) << 8) + static_cast<std::uint16_t>(data[bytes_used + 1]);
        config.digitalActive[ii] = (static_cast<std::uint16_t>(data[bytes_used + 2]) << 8) +
                                   static_cast<std::uint16_t>(data[bytes_used + 3]);
        bytes_used += 4;
    }
    if (data[bytes_used + 1] == 1U)
    {
        config.nominalFrequency = 50.0f;
    }
    bytes_used += 2;
    config.changeCount =
      (static_cast<std::uint16_t>(data[bytes_used]) << 8) + static_cast<std::uint16_t>(data[bytes_used + 1]);
    bytes_used += 2;
    return bytes_used;
}
ParseResult parseConfig1(const std::uint8_t *data, size_t dataSize, Config &config)
{
    CommonFrame frame;
    auto pResult = ParseResult::parse_complete;
    if ((pResult = parseCommon(data, dataSize, frame)) != ParseResult::parse_complete)
    {
        return pResult;
    }
    if (frame.type != PmuPacketType::config1 && frame.type != PmuPacketType::config2)
    {
        return ParseResult::incorrect_type;
    }
    config.idcode = frame.sourceID;
    config.fracsec = frame.fracSec;
    config.soc = frame.soc;
    config.timeBase = (static_cast<std::uint16_t>(data[15]) << 16) + (static_cast<std::uint16_t>(data[16]) << 8) +
                      static_cast<std::uint16_t>(data[17]);
    std::uint16_t numPmu = (static_cast<std::uint16_t>(data[18]) << 8) + data[19];
    config.pmus.resize(numPmu);
    std::size_t bytes_used = 20;
    for (int ii = 0; ii < numPmu; ++ii)
    {
        bytes_used += parsePmuConfig(data + bytes_used, config.pmus[ii]);
    }
    config.dataRate = static_cast<int16_t>((static_cast<std::uint16_t>(data[bytes_used]) << 8) + data[bytes_used+1]);
    return ParseResult::parse_complete;
}

ParseResult parseConfig2(const std::uint8_t *data, size_t dataSize, Config &config)
{
    return parseConfig1(data, dataSize, config);
}

ParseResult parseConfig3(const std::uint8_t *data, size_t dataSize, Config &config)
{
    return ParseResult::not_implemented;
}

static constexpr float tqArray[16] = {1e-12f, 1e-9f, 1e-8f, 1e-7f, 1e-6f,  1e-5f,   1e-4f, 1e-3f,
                                      1e-2f,  1e-1f, 1.0f,  10.0f, 100.0f, 1000.0f, 1e4f,  FLT_MAX};

TimeQuality parseTimeQuality(std::uint8_t tq)
{
    TimeQuality timeQuality;
    timeQuality.reserved = (tq & 0b1000'0000) != 0 ? 1 : 0;
    timeQuality.leapSecondDirection = (tq & 0b0100'0000) != 0 ? 1 : 0;
    timeQuality.leapSecondOccurred = (tq & 0b0010'0000) != 0 ? 1 : 0;
    timeQuality.leapSecondPending = (tq & 0b0001'0000) != 0 ? 1 : 0;
    timeQuality.timeQualityCode = tq & 0b0000'1111;

    timeQuality.timeQuality = tqArray[timeQuality.timeQualityCode];
    return timeQuality;
}

std::uint8_t getTimeQualityCode(float tolerance)
{
    std::uint8_t code{0};
    while (tqArray[code] < tolerance)
    {
        ++code;
    }
    return code;
}

static constexpr int command_location{14};
std::string parseHeader(const std::uint8_t *data, size_t dataSize)
{
    CommonFrame frame;
    if (parseCommon(data, dataSize, frame) != ParseResult::parse_complete)
    {
        return {};
    }
    std::string header(reinterpret_cast<const char *>(data) + command_location,
                       reinterpret_cast<const char *>(data) + frame.byteCount - 2);
    return header;
}

PmuCommand parseCommand(const std::uint8_t *data, size_t dataSize)
{
    CommonFrame frame;
    if (parseCommon(data, dataSize, frame) != ParseResult::parse_complete)
    {
        return PmuCommand::unknown;
    }
    if (frame.type != PmuPacketType::command)
    {
        return PmuCommand::unknown;
    }
    std::uint16_t commandCode = (static_cast<std::uint16_t>(data[command_location]) << 8) + data[command_location+1];

    return static_cast<PmuCommand>(commandCode);
}

std::string getExtendedData(const std::uint8_t* data, size_t dataSize) {
    CommonFrame frame;
    if (parseCommon(data, dataSize, frame) != ParseResult::parse_complete)
    {
        return {};
    }
    if (frame.type != PmuPacketType::command)
    {
        return {};
    }
    std::uint16_t commandCode = (static_cast<std::uint16_t>(data[command_location]) << 8) + data[command_location+1];
    if (commandCode!=extended_frame_command)
    {
        return {};
    }
    std::string ed(reinterpret_cast<const char *>(data) + common_frame_size,
                       reinterpret_cast<const char *>(data) + frame.byteCount - 2);
    return ed;
}

static std::size_t getExpectedDataPacketLength(const Config &config)
{
    std::size_t dataSize{common_frame_size+2U};
    for (const auto &pmu : config.pmus)
    {
        dataSize += 2U;
        dataSize +=
          2 * ((pmu.phasorFormat == integer_format) ? sizeof(std::uint16_t) : sizeof(float)) * pmu.phasorCount;

        dataSize += 2 * ((pmu.freqFormat == integer_format) ? sizeof(std::uint16_t) : sizeof(float));

        dataSize +=
          ((pmu.analogFormat == integer_format) ? sizeof(std::uint16_t) : sizeof(float)) * pmu.analogCount;

        dataSize += pmu.digitalWordCount * sizeof(std::uint16_t);
    }
    return dataSize;
}

static std::size_t parsePmuData(const std::uint8_t *data, const PmuConfig &config, PmuData &pmuData)
{
    std::size_t bytes_used{0U};
    memcpy(&pmuData.stat, data, 2U);
    pmuData.stat = ntohs(pmuData.stat);
    pmuData.phasors.resize(config.phasorCount);
    bytes_used += 2;
    for (int ii = 0; ii < config.phasorCount; ++ii)
    {
        if (config.phasorFormat == integer_format)
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
                  std::complex<double>(static_cast<double>(val1) * 1e-5 *
                                                         static_cast<double>(config.phasorConversion[ii]),
                                      static_cast<double>(val2) * 1e-5 *
                                                         static_cast<double>(config.phasorConversion[ii]));
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
                  std::polar<double>(static_cast<double>(val1) * 1e-5 *
                                                       static_cast<double>(config.phasorConversion[ii]),
                                    static_cast<double>(val2) / 1e4);
            }
            bytes_used += sizeof(std::uint16_t) + sizeof(std::int16_t);
        }
        else
        {
            std::uint32_t val1;
            std::uint32_t val2;
            std::memcpy(&val1, data + bytes_used, sizeof(float));
            std::memcpy(&val2, data + bytes_used + sizeof(float), sizeof(float));
            float val1f = ntohf(val1);
            float val2f = ntohf(val2);
            if (config.phasorCoordinates == rectangular_phasor)
            {
                pmuData.phasors[ii] = std::complex<double>(val1f, val2f);
            }
            else
            {
                pmuData.phasors[ii] = std::polar<double>(val1f, val2f);
            }
            bytes_used += 2 * sizeof(float);
        }
    }

    if (config.freqFormat == floating_point_format)
    {
        std::uint32_t freqData;
        std::memcpy(&freqData, data + bytes_used, sizeof(float));
        pmuData.freq = static_cast<double>(ntohf(freqData));
        bytes_used += sizeof(float);
        std::memcpy(&freqData, data + bytes_used, sizeof(float));
        pmuData.rocof = static_cast<double>(ntohf(freqData));
        bytes_used += sizeof(float);
    }
    else
    {
        std::int16_t freq{0};
        std::memcpy(&freq, data + bytes_used, sizeof(freq));
        freq = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(freq)));
        bytes_used += sizeof(std::int16_t);
        pmuData.freq = static_cast<double>(freq) / 1000.0;
        /* rocof */
        std::memcpy(&freq, data + bytes_used, sizeof(freq));
        freq = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(freq)));
        bytes_used += sizeof(std::int16_t);
        pmuData.rocof = static_cast<double>(freq) / 1000.0;
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
    for (int ii = 0; ii < config.digitalWordCount; ++ii)
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
    if ((pdf.parseResult = parseCommon(data, dataSize, frame)) != ParseResult::parse_complete)
    {
        return pdf;
    }
    if (frame.type != PmuPacketType::data)
    {
        pdf.parseResult = ParseResult::incorrect_type;
        return pdf;
    }
    pdf.idcode = frame.sourceID;
    if (pdf.idcode != config.idcode)
    {
        pdf.parseResult = ParseResult::id_mismatch;
    }
    pdf.timeQuality = static_cast<std::uint8_t>(frame.fracSec >> 24);
    pdf.fracSec =
      static_cast<double>(frame.fracSec & 0x00FFFFFFU) / static_cast<double>(config.timeBase);
    pdf.soc = frame.soc;
    pdf.pmus.resize(config.pmus.size());
    std::size_t bytes_used{common_frame_size};
    auto expectedSize = getExpectedDataPacketLength(config);
    if (expectedSize > dataSize)
    {
        pdf.parseResult = ParseResult::config_mismatch;
        return pdf;
    }
    for (std::size_t ii = 0; ii < config.pmus.size(); ++ii)
    {
        bytes_used += parsePmuData(data + bytes_used, config.pmus[ii], pdf.pmus[ii]);
    }
    return pdf;
}

static void generateCommonFrame(std::uint8_t *data, std::uint16_t dataSize, uint16_t idCode, PmuPacketType type)
{
    if (dataSize < min_packet_size)
    {
        return;
    }
    data[0] = sync_lead;
    data[1] = (static_cast<std::uint8_t>(type) & typeMask) + version2005;
    idCode = htons(idCode);
    memcpy(data + 4, &idCode, 2);
}

static void
generateCommonFrame(std::uint8_t *data, std::uint16_t dataSize, const Config &config, PmuPacketType type)
{
    if (dataSize < min_packet_size)
    {
        return;
    }
    data[0] = sync_lead;
    data[1] = (static_cast<std::uint8_t>(type) & typeMask) + version2005;
    std::uint16_t idCode = htons(config.idcode);
    memcpy(data + 4, &idCode, 2);
}

static std::uint16_t getPmuConfigSize(const PmuConfig &config, bool activeOnly)
{
    std::uint16_t byte_count{30U};
    byte_count += (channel_name_size+sizeof(std::uint32_t)) * config.phasorCount;
    byte_count += (channel_name_size + sizeof(std::uint32_t)) * config.analogCount;
    byte_count += (16U * channel_name_size + 2*sizeof(std::uint16_t)) * config.digitalWordCount;
    return byte_count;
}

static std::uint16_t getConfigSize(const Config &config, bool activeOnly)
{ 
    std::uint16_t byte_count{24U};
    for (const auto &pmu:config.pmus)
    {
        byte_count += getPmuConfigSize(pmu, activeOnly);
    }
    return byte_count;
}

static constexpr std::uint16_t low_byte_mask{0xFF};
static std::uint16_t
generatePMUConfig(std::uint8_t *data, size_t dataSize, const PmuConfig &config,
                                           bool activeOnly)
    {
    std::uint16_t bytes_used{0U};
    if (dataSize < getPmuConfigSize(config,activeOnly))
    {
        return 0;
    }
    auto nameSize = std::min(config.stationName.size(), static_cast<std::size_t>(channel_name_size));
    memcpy(data, config.stationName.data(), nameSize);
    if (nameSize < channel_name_size)
    {
        memset(data + nameSize, 0, channel_name_size - nameSize);
    }
    bytes_used = static_cast<std::uint16_t>(channel_name_size);
    auto sid = htons(config.sourceID);
    memcpy(data + bytes_used, &sid, sizeof(config.sourceID));

    bytes_used += sizeof(config.sourceID);

    

    std::uint16_t format{0U};
    format += (config.freqFormat == floating_point_format) ? 0b1000 : 0U;
    format += (config.analogFormat == floating_point_format) ? 0b0100 : 0U;
    format += (config.phasorFormat == floating_point_format) ? 0b0010 : 0U;
    format += (config.phasorCoordinates == polar_phasor) ? 0b0001 : 0U;
    format = htons(format);
    memcpy(data + bytes_used, &format, sizeof(format));
    bytes_used += sizeof(format);

    data[bytes_used] = static_cast<std::uint8_t>((config.phasorCount >> 8) & low_byte_mask);
    data[bytes_used + 1] = static_cast<std::uint8_t>((config.phasorCount) & low_byte_mask);
    bytes_used += 2;

    data[bytes_used] = static_cast<std::uint8_t>((config.analogCount >> 8) & low_byte_mask);
    data[bytes_used + 1] = static_cast<std::uint8_t>((config.analogCount) & low_byte_mask);
    bytes_used += 2;


    data[bytes_used] = static_cast<std::uint8_t>((config.digitalWordCount >> 8) & low_byte_mask);
    data[bytes_used + 1] = static_cast<std::uint8_t>((config.digitalWordCount) & low_byte_mask);
    bytes_used += 2;

    for (const auto& pn : config.phasorNames)
    {
        nameSize = std::min(pn.size(), static_cast<std::size_t>(channel_name_size));
        memcpy(data+bytes_used, pn.data(), nameSize);
        if (nameSize < channel_name_size)
        {
            memset(data + bytes_used + nameSize, 0, channel_name_size - nameSize);
        }
        bytes_used += channel_name_size;
    }

    for (const auto &an : config.analogNames)
    {
        nameSize = std::min(an.size(), static_cast<std::size_t>(channel_name_size));
        memcpy(data + bytes_used, an.data(), nameSize);
        if (nameSize < channel_name_size)
        {
            memset(data + bytes_used + nameSize, 0, channel_name_size - nameSize);
        }
        bytes_used += channel_name_size;
    }

    for (const auto &an : config.digitChannelNames)
    {
        nameSize = std::min(an.size(), static_cast<std::size_t>(channel_name_size));
        memcpy(data + bytes_used, an.data(), nameSize);
        if (nameSize < channel_name_size)
        {
            memset(data + bytes_used + nameSize, 0, channel_name_size - nameSize);
        }
        bytes_used += channel_name_size;
    }

    for (int ii = 0; ii < config.phasorCount; ++ii)
    {
        
        data[bytes_used] = static_cast<std::uint8_t>(config.phasorType[ii]);

        data[bytes_used + 1] = static_cast<std::uint8_t>((config.phasorConversion[ii] >> 16) & low_byte_mask);
        data[bytes_used + 2] = static_cast<std::uint8_t>((config.phasorConversion[ii] >> 8) & low_byte_mask);
        data[bytes_used + 3] = static_cast<std::uint8_t>((config.phasorConversion[ii]) & low_byte_mask);

        bytes_used += sizeof(std::uint32_t);
    }


    for (int ii = 0; ii < config.analogCount; ++ii)
    {
        
        std::uint32_t conversion = htonl(config.analogConversion[ii]);
        memcpy(data + bytes_used, &conversion, sizeof(conversion));
        data[bytes_used] = static_cast<std::uint8_t>(config.analogType[ii]);


        bytes_used += sizeof(std::uint32_t);
    }

    for (int ii = 0; ii < config.digitalWordCount; ++ii)
    {
        data[bytes_used] = static_cast<std::uint8_t>((config.digitalNominal[ii] >> 8) & low_byte_mask);
        data[bytes_used + 1] = static_cast<std::uint8_t>((config.digitalNominal[ii]) & low_byte_mask);
        // for data rate
        bytes_used += 2;

        data[bytes_used] = static_cast<std::uint8_t>((config.digitalActive[ii] >> 8) & low_byte_mask);
        data[bytes_used + 1] = static_cast<std::uint8_t>((config.digitalActive[ii]) & low_byte_mask);
        // for data rate
        bytes_used += 2;
    }
    data[bytes_used] = 0U;
    data[bytes_used + 1] = (config.nominalFrequency == 50.0f) ? 1U : 0U;
    bytes_used += 2;
    data[bytes_used] = static_cast<std::uint8_t>((config.changeCount >> 8) & low_byte_mask);
    data[bytes_used + 1] = static_cast<std::uint8_t>((config.changeCount) & low_byte_mask);
    bytes_used += 2;

    return bytes_used;
}

static std::uint16_t generateConfig(std::uint8_t *data, size_t dataSize, const Config &config,
                                        bool activeOnly)
 {
    // common frame already generated
     if (dataSize < getConfigSize(config,activeOnly))
     {
         return 0;
     }
     addTime(data, config.soc, config.fracsec);
     //time base
     data[15] = static_cast<std::uint8_t>((config.timeBase >> 16) & low_byte_mask);
     data[16] = static_cast<std::uint8_t>((config.timeBase >> 8) & low_byte_mask);
     data[17] = static_cast<std::uint8_t>((config.timeBase) & low_byte_mask);
    
     auto pmuCnt=config.pmus.size();
     data[18] = static_cast<std::uint8_t>((pmuCnt >> 8) & low_byte_mask);
     data[19] = static_cast<std::uint8_t>((pmuCnt)&low_byte_mask);
     std::uint16_t bytes_used{20};

    for (const auto &pmu:config.pmus)
    {
        if (pmu.active || !activeOnly)
        {
            bytes_used += generatePMUConfig(data + bytes_used, dataSize - bytes_used, pmu, activeOnly);
        }
    }

     data[bytes_used] = static_cast<std::uint8_t>((config.dataRate >> 8) & low_byte_mask);
    data[bytes_used + 1] = static_cast<std::uint8_t>((config.dataRate) & low_byte_mask);
    // for data rate
    bytes_used += 2;
    // for checksum
    bytes_used += 2;
    addSize(data, bytes_used);
    addCRC(data, bytes_used);
    return bytes_used;

}


std::uint16_t
  generateConfig1(std::uint8_t *data, size_t dataSize, const Config &config)
{
    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), config, PmuPacketType::config1);
    return generateConfig(data, dataSize, config, false);
}

std::uint16_t generateConfig2(std::uint8_t *data, size_t dataSize, const Config &config)
{
    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), config, PmuPacketType::config2);
    return generateConfig(data, dataSize, config, true);
}

std::uint16_t generateConfig3(std::uint8_t *data, size_t dataSize, const Config &config) { return 0; }

static std::uint16_t generatePmuDataFrame(std::uint8_t *data,
                                const PmuConfig &config,
                                const PmuData &pmuData)
{
    std::uint16_t bytes_used{0U};
    auto stat = htons(pmuData.stat);
    memcpy(data, &stat, sizeof(std::uint16_t));
    bytes_used += 2;
    for (size_t ii=0;ii<config.phasorCount;++ii)
    {
        if (config.phasorFormat == integer_format)
        {
            if (config.phasorCoordinates == rectangular_phasor)
            {
                std::int16_t real = static_cast<int16_t>(static_cast<double>(pmuData.phasors[ii].real()) * 1e5 /
                             static_cast<double>(config.phasorConversion[ii]));
                std::int16_t imag = static_cast<int16_t>(static_cast<double>(pmuData.phasors[ii].imag()) * 1e5 /
                             static_cast<double>(config.phasorConversion[ii]));

                real = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(real)));
                imag = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(imag)));
                std::memcpy(data + bytes_used, &real,sizeof(std::int16_t));
                
            }
            else
            {
                std::uint16_t mag = static_cast<uint16_t>(static_cast<double>(std::abs(pmuData.phasors[ii])) * 1e5 /
                                                         static_cast<double>(config.phasorConversion[ii]));
                std::int16_t angle = static_cast<int16_t>(static_cast<double>(std::arg(pmuData.phasors[ii])) * 1e4);

                mag = ntohs(mag);
                angle = static_cast<std::int16_t>(ntohs(static_cast<std::uint16_t>(angle)));
                std::memcpy(data + bytes_used, &mag, sizeof(std::uint16_t));
                std::memcpy(data + bytes_used + sizeof(std::uint16_t), &angle, sizeof(std::int16_t));

            }
            bytes_used += 2*sizeof(std::int16_t);
        }
        else
        {
            if (config.phasorCoordinates == rectangular_phasor)
            {
                auto real = htonf(pmuData.phasors[ii].real());

                auto imag = htonf(pmuData.phasors[ii].imag());
                std::memcpy(data+bytes_used, &real, sizeof(float));
                std::memcpy(data + bytes_used + sizeof(float) , &imag, sizeof(float));
            }
            else
            {
                auto mag = htonf(std::abs(pmuData.phasors[ii]));

                auto angle = htonf(std::arg(pmuData.phasors[ii]));
                std::memcpy(data + bytes_used, &mag, sizeof(float));
                std::memcpy(data + bytes_used + sizeof(float), &angle, sizeof(float));
            }
            bytes_used += 2 * sizeof(float);
        }
    }
    if (config.freqFormat==floating_point_format)
    {
        auto freq = htonf(pmuData.freq);
        auto rocof = htonf(pmuData.rocof);
        std::memcpy(data + bytes_used, &freq, sizeof(float));
        std::memcpy(data + bytes_used + sizeof(float), &rocof, sizeof(float));
        bytes_used += 2 * sizeof(float);
    }
    else
    {
        std::int16_t freq = static_cast<int16_t>(static_cast<double>(pmuData.freq) * 1000.0);
        std::int16_t rocof = static_cast<int16_t>(static_cast<double>(pmuData.rocof) * 1000.0);
        
        freq = static_cast<std::int16_t>(htons(static_cast<std::uint16_t>(freq)));
        rocof = static_cast<std::int16_t>(htons(static_cast<std::uint16_t>(rocof)));

        std::memcpy(data + bytes_used, &freq, sizeof(freq));

        std::memcpy(data + bytes_used+sizeof(std::int16_t), &rocof, sizeof(rocof));
        
        bytes_used += 2*sizeof(std::int16_t);
    }

    for (int ii = 0; ii < config.analogCount; ++ii)
    {
        if (config.analogFormat == floating_point_format)
        {
            auto analog = htonf(pmuData.analog[ii]);
            std::memcpy(data + bytes_used, &analog, sizeof(float));
            bytes_used += sizeof(float);
        }
        else
        {
            std::int16_t analog = static_cast<std::int16_t>(pmuData.analog[ii]);
            analog = static_cast<std::int16_t>(htons(static_cast<std::uint16_t>(analog)));
              
            std::memcpy(data + bytes_used,&analog,sizeof(analog));
            
            bytes_used += sizeof(analog);
        }
    }

    for (int ii = 0; ii < config.digitalWordCount; ++ii)
    {
        
        auto dword = htons(pmuData.digital[ii]);
        std::memcpy(data + bytes_used,&dword, sizeof(std::uint16_t));
        bytes_used += sizeof(std::uint16_t);
    }
    return bytes_used;
}

    std::uint16_t
generateDataFrame(std::uint8_t *data, size_t dataSize, const Config &config, const PmuDataFrame &frame)
{
    std::uint16_t bytes_used{common_frame_size};
    if (dataSize < getExpectedDataPacketLength(config))
    {
        return 0;
    }
    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), config, PmuPacketType::data);
    addTime(data, frame.soc, frame.fracSec, frame.timeQuality, config);
    for (size_t ii=0;ii<frame.pmus.size();++ii)
    {
        bytes_used +=
          generatePmuDataFrame(data + bytes_used, config.pmus[ii], frame.pmus[ii]);
    }
    bytes_used += 2;
    addSize(data, bytes_used);
    addCRC(data, bytes_used);
    return bytes_used;
}

std::uint16_t generateHeader(std::uint8_t *data, size_t dataSize, const std::string &header, const Config &config)
{
    std::uint16_t bytes_used{common_frame_size};

    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), config, PmuPacketType::header);
    addSize(data, bytes_used + static_cast<std::uint16_t>(header.size())+2U);
    memcpy(data + bytes_used, header.data(), header.size());
    bytes_used += static_cast<std::uint16_t>(header.size()) + 2U;
    addCRC(data, bytes_used);
    return bytes_used;
}

std::uint16_t
generateExtendedFrame(std::uint8_t *data, size_t dataSize, const std::string &frameData, const uint16_t idCode)
{
    std::uint16_t bytes_used{common_frame_size};

    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), idCode, PmuPacketType::command);

    // add the actual command
    auto cmd = htons(static_cast<std::uint16_t>(extended_frame_command));

    memcpy(data + bytes_used, &cmd, 2);
    bytes_used += 2;
    addSize(data, bytes_used + static_cast<std::uint16_t>(frameData.size()) + 2U);
    memcpy(data + bytes_used, frameData.data(), frameData.size());
    bytes_used += static_cast<std::uint16_t>(frameData.size()) + 2U;
    addCRC(data, bytes_used);
    return bytes_used;
}

std::uint16_t generateCommand(std::uint8_t *data, size_t dataSize, PmuCommand command, const uint16_t idCode)
{
    static constexpr std::uint16_t commandSize{18U};
    generateCommonFrame(data, static_cast<std::uint16_t>(dataSize), idCode, PmuPacketType::command);
    // add the actual command
    auto cmd = htons(static_cast<std::uint16_t>(command));

    memcpy(data + common_frame_size, &cmd, 2);
    addSize(data, commandSize);
    addCRC(data, commandSize);
    return commandSize;
}

std::pair<std::uint32_t, std::uint32_t> generateTimeCodes(std::chrono::nanoseconds tp,
                                                          std::uint32_t timeBase,
                                                          float tolerance)
{
    auto secondsUTC = std::chrono::duration_cast<std::chrono::seconds>(tp).count();
    std::pair<std::uint32_t, std::uint32_t> res;
    res.first = static_cast<std::uint32_t>(secondsUTC);

    auto frac = tp - std::chrono::duration_cast<std::chrono::seconds>(tp);

    double frsec = static_cast<double>(frac.count()) / 1e9;
    res.second = (static_cast<std::uint32_t>(frsec * static_cast<double>(timeBase)) & 0x00FFFFFFU);

    auto tqcode = getTimeQualityCode(tolerance);

    res.second += (static_cast<std::uint32_t>(tqcode) << 24);

    return res;
}

}  // namespace c37118
