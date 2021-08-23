/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "configure.hpp"
#include "JsonProcessingFunctions.hpp"
#include "json/json.h"
#include <iostream>
#include <fstream>

namespace c37118
{
static void insertPhasorConfig(Json::Value &phz, PmuConfig &pmu)
{
    int cnt{1};
    std::string name = phz["name"].asString();
    fileops::replaceIfMember(phz, "count", cnt);
    std::string typeString{"voltage"};

    fileops::replaceIfMember(phz, "type", typeString);
    if (typeString != "voltage" && typeString != "current")
    {
        std::cerr << "Phasor type must be 'voltage' or 'current' " << typeString << "not recognized";
    }
    PhasorType type = (typeString != "voltage") ? PhasorType::current: PhasorType::voltage;
    
    int scale{0};
    fileops::replaceIfMember(phz, "scale", scale);

    if (cnt == 1)
    {
        pmu.phasorNames.push_back(name);
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);
    }
    else if (cnt == 3)
    {
        pmu.phasorNames.push_back(name +"-A");
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);

        pmu.phasorNames.push_back(name + "-B");
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);

        pmu.phasorNames.push_back(name + "-C");
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);
    }
    else if (cnt == 4)
    {
        name.append("-A");
        pmu.phasorNames.push_back(name);
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);
        name.back() = 'B';
        pmu.phasorNames.push_back(name);
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);
        name.back() = 'C';
        pmu.phasorNames.push_back(name);
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);
        name.back() = 'N';
        pmu.phasorNames.push_back(name);
        pmu.phasorType.push_back(type);
        pmu.phasorConversion.push_back(scale);
    }
    else
    {
        name.push_back('-');
        for (int ii=1;ii<=cnt;++ii)
        {
            pmu.phasorNames.push_back(name + std::to_string(ii));
            pmu.phasorType.push_back(type);
            pmu.phasorConversion.push_back(scale);
        }
    }
}

static void insertAnalogConfig(Json::Value &anlg, PmuConfig &pmu)
{
    int cnt{1};
    std::string name = anlg["name"].asString();
    fileops::replaceIfMember(anlg, "count", cnt);

    std::string typeString{"pow"};

    fileops::replaceIfMember(anlg, "type", typeString);
    if (typeString != "pow" && typeString != "peak" && typeString != "rms")
    {
        std::cerr << "Analog type must be 'pow', 'peak', or 'rms' " << typeString << "not recognized";
    }
    AnalogType type = (typeString != "pow")  ? AnalogType::single_point_on_wave :
                      (typeString != "peak") ? AnalogType::peak :
                                               AnalogType::rms;

    int scale{0};
    fileops::replaceIfMember(anlg, "scale", scale);

    if (cnt == 1)
    {
        pmu.analogNames.push_back(name);
        pmu.analogType.push_back(type);
        pmu.analogConversion.push_back(scale);
    }
    else
    {
        name.push_back('-');
        for (int ii = 1; ii <= cnt; ++ii)
        {
            pmu.analogNames.push_back(name + std::to_string(ii));
            pmu.analogType.push_back(type);
            pmu.analogConversion.push_back(scale);
        }
    }
}

static void insertDigitalConfig(Json::Value &dgtl, PmuConfig &pmu)
{
    int cnt{1};
    std::string name = dgtl["name"].asString();
    fileops::replaceIfMember(dgtl, "count", cnt);

    pmu.digitChannelNames.push_back(name);
}

static PmuConfig loadPmuConfigJson(Json::Value &jv)
{
    PmuConfig pmu;
    fileops::replaceIfMember(jv, "name", pmu.stationName);
    int data{0};
    fileops::replaceIfMember(jv, "id", data);
    fileops::replaceIfMember(jv, "idcode", data);
    pmu.sourceID = static_cast<std::uint16_t>(data);

    data = 0;
    fileops::replaceIfMember(jv, "cfgcnt", data);
    pmu.changeCount = static_cast<std::uint16_t>(data);

    fileops::replaceIfMember(jv, "lat", pmu.lat);
    fileops::replaceIfMember(jv, "lon", pmu.lon);
    fileops::replaceIfMember(jv, "elev", pmu.elev);

    std::string format{"floating_point"};
    fileops::replaceIfMember(jv, "phasor_format", format);
    pmu.phasorFormat = (format == "integer") ? integer_format : floating_point_format;

    format="floating_point";
    fileops::replaceIfMember(jv, "analog_format", format);
    pmu.analogFormat = (format == "integer") ? integer_format : floating_point_format;

    format = "floating_point";
    fileops::replaceIfMember(jv, "frequency_format", format);
    pmu.freqFormat = (format == "integer") ? integer_format : floating_point_format;

    format = "rectangular";
    fileops::replaceIfMember(jv, "phasor_coordinates", format);
    pmu.phasorCoordinates = (format == "polar") ? polar_phasor : rectangular_phasor;

    float nfreq = 60.0;
    fileops::replaceIfMember(jv, "nominal_frequency", nfreq);
    fileops::replaceIfMember(jv, "nominalfrequency", nfreq);
    fileops::replaceIfMember(jv, "fnom", nfreq);
    pmu.nominalFrequency = nfreq;

    if (jv.isMember("phasor"))
    {
        auto phasors = jv["phasor"];

        if (phasors.isArray())
        {
            for (auto &phz : phasors)
            {
                insertPhasorConfig(phz, pmu);
            }
        }
        else
        {
            insertPhasorConfig(phasors, pmu);
        }
        pmu.phasorCount = static_cast<std::uint16_t>(pmu.phasorNames.size());
    }
    if (jv.isMember("analog"))
    {
        auto analog = jv["analog"];
        if (analog.isArray())
        {
            for (auto &anlg : analog)
            {
                insertAnalogConfig(anlg, pmu);
            }
        }
        else
        {
            insertAnalogConfig(analog, pmu);
        }
        pmu.analogCount = static_cast<std::uint16_t>(pmu.analogNames.size());
    }
    if (jv.isMember("digital"))
    {
        auto digital = jv["digital"];
        if (digital.isArray())
        {
            for (auto &dgtl : digital)
            {
                insertDigitalConfig(dgtl, pmu);
            }
        }
        else
        {
            insertDigitalConfig(digital, pmu);
        }
        pmu.digitalWordCount = static_cast<std::uint16_t>(pmu.digitalNominal.size());
    }
    return pmu;
}

static Config loadConfigJson(const std::string &configStr)
{
    Config newConfig;
    auto jv = fileops::loadJson(configStr);
    auto &base = (jv.isMember("config")) ? jv["config"] : jv;
    int data{0};
    fileops::replaceIfMember(base, "id", data);
    fileops::replaceIfMember(base, "idcode", data);
    newConfig.idcode = static_cast<std::uint16_t>(data);

    data = 30;
    fileops::replaceIfMember(base, "datarate", data);
    fileops::replaceIfMember(base, "data_rate", data);
    newConfig.dataRate = static_cast<std::int16_t>(data);

    data = 1000000;
    fileops::replaceIfMember(base, "time_base", data);
    fileops::replaceIfMember(base, "timebase", data);
    newConfig.timeBase = static_cast<std::uint32_t>(data);

    
    auto pmu = base["pmu"];
    if (pmu.isArray())
    {
        for (auto &pmucfg : pmu)
        {
            newConfig.pmus.push_back(loadPmuConfigJson(pmucfg));
        }
    }
    else
    {
        auto pmucfg = loadPmuConfigJson(pmu);
        if (pmucfg.sourceID == 0)
        {
            pmucfg.sourceID = newConfig.idcode;
        }
        newConfig.pmus.push_back(std::move(pmucfg));
    }
    
    return newConfig;
}

static Config loadConfigCSV(const std::string &configStr)
{
    Config config;
    return config;
}

Config loadConfig(const std::string &configStr)
{
    Config config;
    if (configStr.size() < 5)
    {
        // there is no known config which could be less than 5 characters  `a.jsn` is the shortest
        return config;
    }
    if (fileops::hasJsonExtension(configStr) || configStr.front() == '{')
    {
        config = loadConfigJson(configStr);
    }
    auto ext = configStr.substr(configStr.length() - 3);
    if (ext == "csv" || ext == "CSV")
    {
        config = loadConfigCSV(configStr);
    }
    else
    {
    }
    return config;
}

static void addPmuJson(const PmuConfig &pmu, Json::Value &jv)
{
    if (!jv.isMember("pmu"))
    {
        jv["pmu"] = Json::arrayValue;
    }
    Json::Value pbase;
    pbase["name"] = pmu.stationName;
    pbase["idcode"] = pmu.sourceID;
    pbase["cfgcnt"] = pmu.changeCount;
    pbase["phasor_format"] = pmu.phasorFormat == integer_format ? "integer" : "floating_point";
    pbase["analog_format"] = pmu.analogFormat == integer_format ? "integer" : "floating_point";
    pbase["frequency_format"] = pmu.freqFormat == integer_format ? "integer" : "floating_point";
    pbase["phasor_coordinates"] = pmu.phasorCoordinates == polar_phasor ? "polar" : "rectangular";
    pbase["nominal_frequency"] = pmu.nominalFrequency;
    if (pmu.phasorCount>0)
    {
        pbase["phasor"] = Json::arrayValue;
        for (int ii = 0; ii < pmu.phasorCount; ++ii)
        {
            Json::Value phasor;
            phasor["name"] = pmu.phasorNames[ii];
            phasor["scale"] = pmu.phasorConversion[ii];
            phasor["type"] = pmu.phasorType[ii] == PhasorType::voltage ? "voltage" : "current";
            pbase["phasor"].append(phasor);
        }
    }
    if (pmu.analogCount > 0)
    {
        pbase["analog"] = Json::arrayValue;
        for (int ii = 0; ii < pmu.analogCount; ++ii)
        {
            Json::Value analog;
            analog["name"] = pmu.analogNames[ii];
            analog["scale"] = pmu.analogType[ii] == AnalogType::single_point_on_wave ? "pow" :
                              pmu.analogType[ii] == AnalogType::rms ? "rms" :
                                                                       "peak";
            pbase["analog"].append(analog);
        }
    }
    jv["pmu"].append(std::move(pbase));
}

static void writeConfigJson(const std::string &configFile, const Config &config) { 
    Json::Value base;
    base["idcode"] = static_cast<int>(config.idcode);
    base["data_rate"] = static_cast<int>(config.dataRate);
    base["time_base"] = static_cast<int>(config.timeBase);

    for (auto pmu:config.pmus)
    {
        addPmuJson(pmu, base);
    }

    Json::Value doc;
    doc["config"] = base;


    std::ofstream out(configFile);
    if (out)
    {
        out << fileops::generateJsonString(doc) << std::endl;
    }
    
}

static void writeConfigCSV(const std::string &configFile, const Config &config) {}


void writeConfig(const std::string &configFile,const Config &config)
{
    if (fileops::hasJsonExtension(configFile))
    {
        writeConfigJson(configFile,config);
        return;
    }
    auto ext = configFile.substr(configFile.length() - 3);
    if (ext == "csv" || ext == "CSV")
    {
        writeConfigCSV(configFile,config);
    }
    else
    {
        writeConfigJson(configFile, config);
    }
}

}  // namespace c37118
