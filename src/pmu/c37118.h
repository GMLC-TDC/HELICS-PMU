/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <complex>
#include <chrono>

namespace c37118
{
static constexpr std::uint8_t sync_lead{0xAA};
static constexpr std::uint8_t data_frame_code{0b0000'0000};
static constexpr std::uint8_t config1_code{0b0010'0000};
static constexpr std::uint8_t config2_code{0b0011'0000};
static constexpr std::uint8_t config3_code{0b0101'0000};
static constexpr std::uint8_t header_code{0b0001'0000};
static constexpr std::uint8_t command_code{0b0100'0000};

static constexpr std::uint8_t typeMask{0b0111'0000};

static constexpr std::uint8_t versionMask{0b0000'1111};

static constexpr std::uint8_t version2005{0b0001};
static constexpr std::uint8_t version2011{0b0010};

static constexpr std::uint16_t data_off_command{0b0001};
static constexpr std::uint16_t data_on_command{0b0010};
static constexpr std::uint16_t send_header_command{0b0011};
static constexpr std::uint16_t send_config1_command{0b0100};
static constexpr std::uint16_t send_config2_command{0b0101};
static constexpr std::uint16_t send_config3_command{0b0110};
static constexpr std::uint16_t extended_frame_command{0b1000};
enum class PmuPacketType : uint8_t
{
    data = data_frame_code,
    header = header_code,
    config1 = config1_code,
    config2 = config2_code,
    config3 = config3_code,
    command =command_code,
    unknown = 0xFFU,

    
};
enum class PmuCommand : uint16_t
{
    unknown=0U,
    data_off = 1U,
    data_on = 2U,
    send_header = 3U,
    send_config1 = 4U,
    send_config2 = 5U,
    send_config3 = 6U,
    extended = 8U,
    
};

enum class ParseResult : std::int32_t
{
    invalid_sync = -16,
    invalid_checksum = -12,
    id_mismatch = -9,
    incorrect_type = -6,
    not_implemented = -4,
    length_mismatch = -3,
    config_mismatch = -1,
    parse_complete = 0,
    not_parsed = 2
};

class CommonFrame
{
  public:
    PmuPacketType type;
    std::uint16_t sourceID;
    std::uint16_t chkSum;
    std::uint16_t byteCount{0U};
    std::uint32_t soc;
    std::uint32_t fracSec;
};

static constexpr std::uint8_t integer_format{0U};
static constexpr std::uint8_t floating_point_format{1U};
static constexpr std::uint8_t rectangular_phasor{0U};
static constexpr std::uint8_t polar_phasor{1U};

static constexpr std::uint16_t common_frame_size{14U};
static constexpr std::uint16_t min_packet_size{18U};
static constexpr std::uint16_t channel_name_size{16U};

enum class PhasorType : std::uint8_t
{
    voltage = 0, 
    current = 1, 
    voltage_disabled = 0b1000'0000,
    current_disabled=0b1000'0001
};

enum class AnalogType : std::uint8_t
{
    single_point_on_wave = 0,
    rms = 1,
    peak = 2,
    single_point_on_wave_disabled = 0b1000'0000,
    rms_disabled = 0b1000'0001,
    peak_disabled = 0b1000'0010
};

class PmuConfig
{
  public:
    std::uint16_t sourceID;
    std::uint8_t pmuClass;
    std::uint8_t phasorFormat : 1;
    std::uint8_t analogFormat : 1;
    std::uint8_t freqFormat : 1;
    std::uint8_t phasorCoordinates : 1;
    std::uint16_t phasorCount{0U};
    std::uint16_t analogCount{0U};
    std::uint16_t digitalWordCount{0U};
    std::vector<std::string> phasorNames;
    std::vector<std::string> analogNames;
    std::vector<std::string> digitChannelNames;
    std::vector<PhasorType> phasorType;
    std::vector<std::uint32_t> phasorConversion;
    std::vector<AnalogType> analogType;
    std::vector<std::int32_t> analogConversion;
    std::vector<std::uint16_t> digitalNominal;
    std::vector<std::uint16_t> digitalActive;
    float nominalFrequency{60.0};
    
    float lat;
    float lon;
    float elev;

    std::uint16_t changeCount;
    bool active{true};
    std::uint32_t window;
    std::uint32_t grpDelay;
    std::string stationName;
};

class Config
{
  public:
    std::uint16_t idcode;
    std::uint16_t dataRate;
    std::uint32_t soc;
    std::uint32_t fracsec;
    std::uint32_t timeBase;
    std::vector<PmuConfig> pmus;
};

class TimeQuality
{
  public:
    std::uint8_t reserved : 1;
    std::uint8_t leapSecondDirection : 1;
    std::uint8_t leapSecondOccurred : 1;
    std::uint8_t leapSecondPending : 1;
    std::uint8_t timeQualityCode : 4;
    float timeQuality;

};

TimeQuality parseTimeQuality(std::uint8_t tq);

class PmuData
{
  public:
   
    std::uint16_t stat;
    double freq;
    double rocof;
    std::vector<std::complex<double>> phasors;
    std::vector<double> analog;
    std::vector<std::uint16_t> digital;

};

class PmuDataFrame
{
  public:
    std::uint16_t idcode;
    std::uint8_t timeQuality;
    ParseResult parseResult{ParseResult::not_parsed};
    std::uint32_t soc;
    double fracSec;
    
    std::vector<PmuData> pmus;
};

PmuPacketType getPacketType(const std::uint8_t *data, size_t dataSize);

std::uint16_t getIdCode(const std::uint8_t *data, size_t dataSize);

std::uint16_t getPacketSize(const std::uint8_t *data, size_t dataSize);

ParseResult parseCommon(const std::uint8_t *data, size_t dataSize, CommonFrame &frame);
    
ParseResult parseConfig1(const std::uint8_t *data, size_t dataSize, Config &config);

ParseResult parseConfig2(const std::uint8_t *data, size_t dataSize, Config &config);

ParseResult parseConfig3(const std::uint8_t *data, size_t dataSize, Config &config);

std::string parseHeader(const std::uint8_t *data, size_t dataSize);

PmuCommand parseCommand(const std::uint8_t *data, size_t dataSize);

std::string getExtendedData(const std::uint8_t *data, size_t dataSize);

PmuDataFrame parseDataFrame(const std::uint8_t *data, size_t dataSize, const Config &config);

std::uint16_t generateConfig1(std::uint8_t *data, size_t dataSize, const Config &config);

std::uint16_t generateConfig2(std::uint8_t *data, size_t dataSize, const Config &config);

std::uint16_t generateConfig3(std::uint8_t *data, size_t dataSize, const Config &config);

std::uint16_t generateHeader(std::uint8_t *data, size_t dataSize, const std::string &header, const Config &config);

std::uint16_t generateCommand(std::uint8_t *data, size_t dataSize, PmuCommand command, uint16_t idCode);

std::uint16_t generateDataFrame(std::uint8_t *data, size_t dataSize, const Config &config, const PmuDataFrame &frame);

std::pair<std::uint32_t, std::uint32_t> generateTimeCodes(std::chrono::time_point<std::chrono::system_clock> tp,
                                                          std::uint32_t timeBase = 10'000'000,
                                                          float tolerance = 0.0f );

  inline std::pair<std::uint32_t, std::uint32_t> generateTimeCodes(std::chrono::time_point<std::chrono::system_clock> tp,
                                                          const Config &config,
                                                          double tolerance = 0.0f)
{
    return generateTimeCodes(tp, config.timeBase, tolerance);
}
}
