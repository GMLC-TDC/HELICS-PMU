
/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include <gtest/gtest.h>

#include "../src/pmu/StableSource.cpp"

static c37118::Config testConfig1(std::uint16_t code)
{
    c37118::Config cfg1;
    cfg1.dataRate = 30;
    cfg1.timeBase = 1000000;
    cfg1.idcode = code;

    c37118::PmuConfig pmu1;
    pmu1.sourceID = code;
    pmu1.phasorCount = 3;
    pmu1.phasorNames = {"V1-A", "V1-B", "V2-B"};
    pmu1.phasorFormat = c37118::rectangular_phasor;
    pmu1.phasorConversion = {1, 1, 1};
    pmu1.changeCount = 1;
    pmu1.phasorType = {c37118::PhasorType::voltage, c37118::PhasorType::voltage, c37118::PhasorType::voltage};
    pmu1.analogCount = 0;
    pmu1.digitalWordCount = 0;
    pmu1.freqFormat = c37118::floating_point_format;
    pmu1.phasorFormat = c37118::floating_point_format;
    pmu1.stationName = "testPMU1";
    pmu1.active = true;
    cfg1.pmus.push_back(std::move(pmu1));
    return cfg1;
}

static c37118::PmuDataFrame testDataFrame(std::uint16_t code)
{
    c37118::PmuDataFrame pdf;
    pdf.idcode = code;
    
    c37118::PmuData pd;
    pd.phasors = {std::polar<double>(120.0, 0), std::polar<double>(120.0, 2.0 * 3.14159 / 3.0),
                  std::polar<double>(120.0, 4.0 * 3.14159 / 3.0)};
    pd.freq = 60.0;
    pd.rocof = 0.0;
    pd.stat = 0;
    pdf.pmus.push_back(pd);
    return pdf;
}

TEST(stable_source, test1)
 { pmu::StableSource ssrc;
     auto testConfig = testConfig1(10);
     EXPECT_EQ(testConfig.idcode, 10U);
     ssrc.setConfig(testConfig);
     auto testData = testDataFrame(10);
     EXPECT_EQ(testData.idcode, 10U);

     ssrc.setData(testData);

     auto clk = std::chrono::system_clock::now();

     c37118::PmuDataFrame pdf;
     ssrc.fillDataFrame(pdf, clk.time_since_epoch());
    
     EXPECT_EQ(pdf.idcode, testData.idcode);
     ASSERT_EQ(pdf.pmus.size(), testData.pmus.size());

     ASSERT_EQ(pdf.pmus[0].phasors.size(), testData.pmus[0].phasors.size());

     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[0].real(), testData.pmus[0].phasors[0].real());
     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[0].imag(), testData.pmus[0].phasors[0].imag());

     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[1].real(), testData.pmus[0].phasors[1].real());
     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[1].imag(), testData.pmus[0].phasors[1].imag());

     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[2].real(), testData.pmus[0].phasors[2].real());
     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[2].imag(), testData.pmus[0].phasors[2].imag());

     EXPECT_FLOAT_EQ(pdf.pmus[0].freq, testData.pmus[0].freq);
     EXPECT_FLOAT_EQ(pdf.pmus[0].rocof, testData.pmus[0].rocof);

     EXPECT_EQ(pdf.soc, std::chrono::duration_cast<std::chrono::seconds>(clk.time_since_epoch()).count());

     ssrc.fillDataFrame(pdf, clk.time_since_epoch()+std::chrono::seconds(2));

     EXPECT_EQ(pdf.idcode, testData.idcode);
     ASSERT_EQ(pdf.pmus.size(), testData.pmus.size());

     ASSERT_EQ(pdf.pmus[0].phasors.size(), testData.pmus[0].phasors.size());

     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[0].real(), testData.pmus[0].phasors[0].real());
     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[0].imag(), testData.pmus[0].phasors[0].imag());

     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[1].real(), testData.pmus[0].phasors[1].real());
     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[1].imag(), testData.pmus[0].phasors[1].imag());

     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[2].real(), testData.pmus[0].phasors[2].real());
     EXPECT_FLOAT_EQ(pdf.pmus[0].phasors[2].imag(), testData.pmus[0].phasors[2].imag());

     EXPECT_FLOAT_EQ(pdf.pmus[0].freq, testData.pmus[0].freq);
     EXPECT_FLOAT_EQ(pdf.pmus[0].rocof, testData.pmus[0].rocof);

     EXPECT_EQ(pdf.soc, std::chrono::duration_cast<std::chrono::seconds>(clk.time_since_epoch()).count()+2);
 }
