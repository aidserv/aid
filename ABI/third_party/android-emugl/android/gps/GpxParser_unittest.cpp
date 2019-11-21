// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/testing/TestTempDir.h"
#include "android/base/testing/Utils.h"
#include "android/gps/GpxParser.h"

#include <fstream>
#include <gtest/gtest.h>

using android::base::TestTempDir;

namespace android_gps {

TEST(GpxParser, ParseFileNotFound) {
    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile("i_dont_exist.gpx", &locations, &error);
    EXPECT_FALSE(isOk);
}

TEST(GpxParser, ParseFileEmpty) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(0U, locations.size());
}

TEST(GpxParser, ParseFileEmptyRteTrk) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<rte>"
            "</rte>"
            "<trk>"
            "<trkseg>"
            "</trkseg>"
            "</trk>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(0U, locations.size());
}

TEST(GpxParser, ParseFileValid) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
                "<wpt lon=\"0\" lat=\"0\">"
                "<name>Wpt 1</name>"
                "</wpt>"
                "<wpt lon=\"0\" lat=\"0\">"
                "<name>Wpt 2</name>"
                "</wpt>"
            "<rte>"
                "<rtept lon=\"0\" lat=\"0\">"
                "<name>Rtept 1</name>"
                "</rtept>"
                "<rtept lon=\"0\" lat=\"0\">"
                "<name>Rtept 2</name>"
                "</rtept>"
            "</rte>"
            "<trk>"
            "<trkseg>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 1-1</name>"
                "</trkpt>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 1-2</name>"
                "</trkpt>"
            "</trkseg>"
            "<trkseg>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 2-1</name>"
                "</trkpt>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 2-2</name>"
                "</trkpt>"
            "</trkseg>"
            "</trk>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    ASSERT_EQ(8U, locations.size());

    EXPECT_EQ("Wpt 1", locations[0].name);
    EXPECT_EQ("Wpt 2", locations[1].name);
    EXPECT_EQ("Rtept 1", locations[2].name);
    EXPECT_EQ("Rtept 2", locations[3].name);
    EXPECT_EQ("Trkpt 1-1", locations[4].name);
    EXPECT_EQ("Trkpt 1-2", locations[5].name);
    EXPECT_EQ("Trkpt 2-1", locations[6].name);
    EXPECT_EQ("Trkpt 2-2", locations[7].name);
}


TEST(GpxParser, ParseFileNullAttribute) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
                "<wpt lon=\"0\" lat=\"0\">"
                    "<name/>"
                "</wpt>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;

    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);

    // This test only checks if GpxParser doesn't crash on null attributes
    // So if we're here it's already Ok - these tests aren't that relevant.
    EXPECT_TRUE(isOk);
    EXPECT_EQ(1U, locations.size());
    EXPECT_STREQ("", locations[0].name.c_str());
    EXPECT_TRUE(error.empty());
}


TEST(GpxParser, ParseLocationMissingLatitude) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lon=\"9.81\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_FALSE(isOk);
}

TEST(GpxParser, ParseLocationMissingLongitude) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lat=\"3.1415\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_FALSE(isOk);
}

TEST(GpxParser, ParseValidLocation) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lon=\"9.81\" lat=\"3.1415\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";
    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(1U, locations.size());
    const GpsFix& wpt = locations[0];

    EXPECT_EQ("Desc", wpt.description);
    EXPECT_FLOAT_EQ(6.02, wpt.elevation);
    EXPECT_FLOAT_EQ(3.1415, wpt.latitude);
    EXPECT_FLOAT_EQ(9.81, wpt.longitude);
    EXPECT_EQ("Name", wpt.name);
}


TEST(GpxParser, ParseValidLocationCommaLocale) {
    auto scopedCommaLocale = setScopedCommaLocale();

    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lon=\"9.81\" lat=\"3.1415\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";
    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(1U, locations.size());
    const GpsFix& wpt = locations[0];

    EXPECT_EQ("Desc", wpt.description);
    EXPECT_FLOAT_EQ(6.02, wpt.elevation);
    EXPECT_FLOAT_EQ(3.1415, wpt.latitude);
    EXPECT_FLOAT_EQ(9.81, wpt.longitude);
    EXPECT_EQ("Name", wpt.name);
}

TEST(GpxParser, ParseValidDocument) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
                "<wpt lon=\"0\" lat=\"0\">"
                "<name>Wpt 1</name>"
                "</wpt>"
                "<wpt lon=\"0\" lat=\"0\">"
                "<name>Wpt 2</name>"
                "</wpt>"
            "<rte>"
                "<rtept lon=\"0\" lat=\"0\">"
                "<name>Rtept 1</name>"
                "</rtept>"
                "<rtept lon=\"0\" lat=\"0\">"
                "<name>Rtept 2</name>"
                "</rtept>"
            "</rte>"
            "<trk>"
            "<trkseg>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 1-1</name>"
                "</trkpt>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 1-2</name>"
                "</trkpt>"
            "</trkseg>"
            "<trkseg>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 2-1</name>"
                "</trkpt>"
                "<trkpt lon=\"0\" lat=\"0\">"
                "<name>Trkpt 2-2</name>"
                "</trkpt>"
            "</trkseg>"
            "</trk>"
            "</gpx>";
    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(8U, locations.size());

    EXPECT_EQ("Wpt 1", locations[0].name);
    EXPECT_EQ("Wpt 2", locations[1].name);
    EXPECT_EQ("Rtept 1", locations[2].name);
    EXPECT_EQ("Rtept 2", locations[3].name);
    EXPECT_EQ("Trkpt 1-1", locations[4].name);
    EXPECT_EQ("Trkpt 1-2", locations[5].name);
    EXPECT_EQ("Trkpt 2-1", locations[6].name);
    EXPECT_EQ("Trkpt 2-2", locations[7].name);
}

}
