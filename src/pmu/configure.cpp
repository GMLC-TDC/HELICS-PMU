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

    PhasorType type{PhasorType::voltage};
    if (phz.isMember("type"))
    {
        if (phz["type"].isUInt())
        {
            type = static_cast<PhasorType>(phz["type"].asUInt());
        }
        else if (phz["type"].isString())
        {
            std::string typeString = phz["type"].asString();
            if (typeString != "voltage" && typeString != "current")
            {
                std::cerr << "Phasor type must be 'voltage' or 'current' " << typeString << "not recognized";
            }
            type = (typeString != "voltage")  ? PhasorType::current :PhasorType::voltage;
        }
    }
    
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

    
    AnalogType type{AnalogType::single_point_on_wave};
    if (anlg.isMember("type"))
    {
        if (anlg["type"].isUInt())
        {
            type = static_cast<AnalogType>(anlg["type"].asUInt());
        }
        else if (anlg["type"].isString())
        {
            std::string typeString = anlg["type"].asString();
            if (typeString != "pow" && typeString != "peak" && typeString != "rms")
            {
                std::cerr << "Analog type must be 'pow', 'peak', or 'rms' " << typeString << "not recognized";
            }
            type= (typeString != "pow")  ? AnalogType::single_point_on_wave :
              (typeString != "peak") ? AnalogType::peak :
                                       AnalogType::rms;
        }
    }
   

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
void writePmuDataJson(Json::Value &jv, const PmuData &pmu) 
{ 
    if (!jv.isMember("pmu"))
    {
        jv["pmu"] = Json::arrayValue;
    }
    Json::Value pmuJ;
    pmuJ["freq"] = pmu.freq;
    pmuJ["rocof"] = pmu.rocof;
    pmuJ["stat"] = static_cast<std::uint32_t>(pmu.stat);
    if (!pmu.phasors.empty())
    {
        Json::Value phz = Json::arrayValue;
        for (const auto &phs:pmu.phasors)
        {
            phz.append(phs.real());
            phz.append(phs.imag());
        }
        pmuJ["phasor"] = std::move(phz);
    }
    if (!pmu.analog.empty())
    {
        Json::Value analog = Json::arrayValue;
        for (const auto &val : pmu.analog)
        {
            analog.append(val);
        }
        pmuJ["analog"] = std::move(analog);
    }
    if (!pmu.digital.empty())
    {
        Json::Value dig = Json::arrayValue;
        for (const auto &val : pmu.digital)
        {
            dig.append(static_cast<std::uint32_t>(val));
        }
        pmuJ["digital"] = std::move(dig);
    }
    jv["pmu"].append(std::move(pmuJ));
}

PmuData loadPmuDataJson(const Json::Value &jv)
    {
        PmuData data;
    data.freq = jv["freq"].asDouble();
    data.rocof = jv["rocof"].asDouble();

    std::uint32_t stat {0};
    fileops::replaceIfMember(jv, "stat", stat);
    data.stat = static_cast<std::uint16_t>(stat);

    bool polar{false};
    fileops::replaceIfMember(jv, "polar_coordinates", polar);
    for (Json::ArrayIndex ii = 0; ii < jv["phasor"].size();ii+=2)
    {
        if (polar)
        {
            data.phasors.push_back(
              std::polar<double>(jv["phasor"][ii].asDouble(), jv["phasor"][ii + 1].asDouble()));
        }
        else
        {
            data.phasors.emplace_back(jv["phasor"][ii].asDouble(), jv["phasor"][ii + 1].asDouble());
        }
    }
    for (auto &analog : jv["analog"])
    {
        data.analog.push_back(analog.asDouble());
    }
    for (auto &digital : jv["digital"])
    {
        data.digital.push_back(static_cast<std::uint16_t>(digital.asUInt()));
    }
    return data;
}

void addDataFrameJson(Json::Value &jv, const PmuDataFrame &pdf) 
{
    jv["id"] = static_cast<std::uint32_t>(pdf.idcode);
    jv["time_quality"] = static_cast<std::uint32_t>(pdf.timeQuality);
    jv["soc"] = pdf.soc;
    jv["fracsec"]=pdf.fracSec;
    for (const auto &pmu : pdf.pmus)
    {
        writePmuDataJson(jv,pmu);
    }
}

PmuDataFrame loadDataFrame(const Json::Value &jv,bool checkObject)
{ 
        if (checkObject && jv.isMember("data"))
        {
            return loadDataFrame(jv["data"], false);
        }
        PmuDataFrame pdf;
        if (!jv.isObject())
        {
            pdf.parseResult = ParseResult::not_parsed;
        }
    
    int data{0};
    fileops::replaceIfMember(jv, "id", data);
    fileops::replaceIfMember(jv, "idcode", data);
    pdf.idcode = static_cast<std::uint16_t>(data);

    
    fileops::replaceIfMember(jv, "timequality", data);
    fileops::replaceIfMember(jv, "time_quality", data);
    pdf.timeQuality = static_cast<std::uint8_t>(data);
    if (jv.isMember("time"))
    {
        double df = jv["time"].asDouble();
        pdf.soc = static_cast<std::uint32_t>(std::floor(df));
        pdf.fracSec = df - std::floor(df);
    }
    else if (jv.isMember("soc"))
    {
        pdf.soc = jv["soc"].asInt();
        if (jv.isMember("fracsec"))
        {
            pdf.fracSec = jv["fracsec"].asDouble();
        }
    }
    
     auto pmu = jv["pmu"];
    if (pmu.isArray())
    {
        for (auto &pmucfg : pmu)
        {
            pdf.pmus.push_back(loadPmuDataJson(pmucfg));
        }
    }
    else
    {
       pdf.pmus.push_back(loadPmuDataJson(pmu));
    }
    pdf.parseResult = ParseResult::parse_complete;
    return pdf;
}


/** return new current count*/
static int insertDigitalConfig(Json::Value &dgtl, PmuConfig &pmu, int currentCnt)
{
    int cnt{0};
    int newTotal{currentCnt};
    auto nm=dgtl["name"];
    fileops::replaceIfMember(dgtl, "count", cnt);

    if (nm.isArray()&& cnt==0)
    {
        cnt = static_cast<int>(nm.size());
    }
    bool active{true};
    bool activeIsBool{true};
    std::uint16_t activeVals{0x00};
    if (dgtl.isMember("active"))
    {
        if (dgtl["active"].isBool())
        {
            active = dgtl["active"].asBool();
        }
        else
        {
            activeIsBool = false;
            activeVals = static_cast<std::uint16_t>(dgtl["active"].asInt());
        }
    }

    bool nominal{true};
    bool nominalIsBool{true};
    std::uint16_t nominalVals{0x00};
    if (dgtl.isMember("nominal"))
    {
        if (dgtl["nominal"].isBool())
        {
            active = dgtl["nominal"].asBool();
        }
        else
        {
            nominalIsBool = false;
            nominalVals = static_cast<std::uint16_t>(dgtl["nominal"].asUInt());
        }
    }

    if (cnt==16)
    {
        if (currentCnt%16!=0)
        {
            currentCnt = (currentCnt / 16 + 1) * 16;
        }
        if (nm.isArray())
        {
            for (int jj=0;jj<16;++jj)
            {
                pmu.digitChannelNames.push_back(nm[jj].asString());
            }
        }
        else
        {
            for (int jj = 0; jj < 16; ++jj)
            {
                pmu.digitChannelNames.push_back(nm.asString()+std::to_string(jj));
            }
        }
        if (activeIsBool)
        {
            pmu.digitalActive.push_back(active ? 0xFF : 0x00);
        }
        else
        {
            pmu.digitalActive.push_back(activeVals);
        }

        if (nominalIsBool)
        {
            pmu.digitalNominal.push_back(nominal ? 0xFF : 0x00);
        }
        else
        {
            pmu.digitalNominal.push_back(nominalVals);
        }
        newTotal = currentCnt + 16;
    }
    else
    {
    
    }
    return newTotal;
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
    int cnt{0};
    if (jv.isMember("digital"))
    {
        auto digital = jv["digital"];
        if (digital.isArray())
        {
            for (auto &dgtl : digital)
            {
                cnt=insertDigitalConfig(dgtl, pmu,cnt);
            }
        }
        else
        {
            cnt=insertDigitalConfig(digital, pmu,cnt);
        }
        pmu.digitalWordCount = static_cast<std::uint16_t>(pmu.digitalNominal.size());
    }
    return pmu;
}

static Config loadConfigJson(const std::string &configStr)
{
    
    auto jv = fileops::loadJson(configStr);
    return loadConfigJson(jv);
}

Config loadConfigJson(const Json::Value& jv) {
    Config newConfig;
    auto &base = (jv.isMember("config")) ? jv["config"] : jv;
    int data{0};
    fileops::replaceIfMember(base, "id", data);
    fileops::replaceIfMember(base, "idcode", data);
    newConfig.idcode = static_cast<std::uint16_t>(data);

    data = default_data_rate;
    fileops::replaceIfMember(base, "datarate", data);
    fileops::replaceIfMember(base, "data_rate", data);
    newConfig.dataRate = static_cast<std::int16_t>(data);

    data = default_time_base;
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
            switch (pmu.phasorType[ii])
            {
            case PhasorType::voltage:
                phasor["type"] = "voltage";
                break;
            case PhasorType::current:
                phasor["type"] = "current";
                break;
            default:
                phasor["type"] = static_cast<std::uint32_t>(pmu.phasorType[ii]);
                break;
            }
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
            analog["scale"] = pmu.analogConversion[ii];
            switch (pmu.analogType[ii])
            {
            case AnalogType::single_point_on_wave:
                analog["type"] = "pow";
                break;
            case AnalogType::rms:
                analog["type"] = "rms";
                break;
            case AnalogType::peak:
                analog["type"] = "peak";
                break;
            default:
                analog["type"] = static_cast<std::uint32_t>(pmu.analogType[ii]);
                break;
            }
            
            pbase["analog"].append(analog);
        }
    }
    if (pmu.digitalWordCount > 0)
    {
        pbase["digital"] = Json::arrayValue;
        for (int ii = 0; ii < pmu.digitalWordCount; ++ii)
        {
            Json::Value digital;
            digital["name"] = Json::arrayValue;
            for (int jj=0;jj<16;++jj)
            {
                digital["name"].append(pmu.digitChannelNames[ii * 16 + jj]);
            }
            digital["active"] = pmu.digitalActive[ii];
            digital["nominal"] = pmu.digitalNominal[ii];
            pbase["digital"].append(digital);
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

void writeDataJson(Json::Value &df, const PmuDataFrame &data)
{
    if (!df.isMember("data"))
    {
        df["data"] = Json::arrayValue;
    }
        Json::Value dv = Json::objectValue;
    addDataFrameJson(dv, data);
        df["data"].append(std::move(dv));
}

void writeDataJson(Json::Value& df, const std::vector<PmuDataFrame>& data) {
    if (!df.isMember("data"))
    {
        df["data"] = Json::arrayValue;
    }
    for (auto & pd:data)
    {
        Json::Value dv = Json::objectValue;
        addDataFrameJson(dv, pd);
        df["data"].append(std::move(dv));
    } 
}

void writeDataFile(const std::string& dataFile, const PmuDataFrame& data) {
    if (fileops::hasJsonExtension(dataFile))
    {
        Json::Value doc = Json::objectValue;
        writeDataJson(doc, data);
        std::ofstream out(dataFile);
        if (out)
        {
            out << fileops::generateJsonString(doc) << std::endl;
        }
    }
    else
    {
        auto ext = dataFile.substr(dataFile.length() - 3);
        if (ext == "csv" || ext == "CSV")
        {
            // writeConfigCSV(configFile, config);
        }
        else
        {
            Json::Value doc = Json::objectValue;
            writeDataJson(doc, data);
            std::ofstream out(dataFile);
            if (out)
            {
                out << fileops::generateJsonString(doc) << std::endl;
            }
        }
    }
}

void writeDataFile(const std::string& dataFile, const std::vector<PmuDataFrame>& data) {
    if (fileops::hasJsonExtension(dataFile))
    {
        Json::Value doc = Json::objectValue;
        writeDataJson(doc, data);
        std::ofstream out(dataFile);
        if (out)
        {
            out << fileops::generateJsonString(doc) << std::endl;
        }
    }
    else
    {
        auto ext = dataFile.substr(dataFile.length() - 3);
        if (ext == "csv" || ext == "CSV")
        {
            // writeConfigCSV(configFile, config);
        }
        else
        {
            Json::Value doc = Json::objectValue;
            writeDataJson(doc, data);
            std::ofstream out(dataFile);
            if (out)
            {
                out << fileops::generateJsonString(doc) << std::endl;
            }
        }
    }
}


const std::vector<PmuDataFrame> loadDataFile(const std::string &dataFile)
{
    std::vector<PmuDataFrame> dataV;
    if (fileops::hasJsonExtension(dataFile))
    {
        auto jv = fileops::loadJson(dataFile);
        auto data = jv["data"];
        if (data.isArray())
        {
            for (auto &dvsection : data)
            {
                dataV.push_back(loadDataFrame(dvsection, false));
            }
        }
        else
        {
            dataV.push_back(loadDataFrame(data, false));
        }
    }
    return dataV;
}
}  // namespace c37118
