// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/IniFile.h"

#include "android/base/testing/TestTempDir.h"
#include "android/base/ArraySize.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace android {
namespace base {

using std::endl;
using std::numeric_limits;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace {

class IniFileTest : public ::testing::Test {
public:
    void SetUp() override {
        mTempDir.reset(new TestTempDir("inifiletest"));
        mIniFilePath = mTempDir->makeSubPath("test.ini").c_str();
        mIni.reset(new IniFile(mIniFilePath));
    }

    void TearDown() override {
        mIni.reset();
        mTempDir.reset();
    }

protected:
    void writeIniFileData(const vector<string>& lines) {
        std::ofstream outFile(mIniFilePath,
                              std::ios_base::out | std::ios_base::trunc);
        ASSERT_FALSE(outFile.fail());
        for (const auto& line : lines) {
            outFile << line << endl;
        }
    }

    void verifyFileContents(const vector<string>& lines) {
        std::ifstream inFile(mIniFilePath);
        ASSERT_FALSE(inFile.fail());
        vector<string> fileLines;
        string line;
        while (std::getline(inFile, line)) {
            fileLines.push_back(line);
        }

        ASSERT_EQ(lines.size(), fileLines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            EXPECT_EQ(lines[i], fileLines[i]);
        }
    }

    // Verifies that the underlying file for |ini| is |updated| when
    // |writeIfChanged| is called.
    // Note that this changes |ini|. After this function is called, |ini| is
    // always
    // clean, and the data has been changed arbitrarily.
    void verifyFileUpdated(bool updated) {
        static int counter = 0;
        string key = "key" + std::to_string(counter++);
        vector<string> lines = {key + " = somevalue"};

        writeIniFileData(lines);
        EXPECT_TRUE(mIni->writeIfChanged());
        EXPECT_TRUE(mIni->read());

        // If the file was updated, then the new uniquely written data must have
        // disappeared, not otherwise.
        if (updated) {
            EXPECT_FALSE(mIni->hasKey(key));
        } else {
            EXPECT_TRUE(mIni->hasKey(key));
        }
    }

    unique_ptr<TestTempDir> mTempDir;
    string mIniFilePath;
    unique_ptr<IniFile> mIni;
};

}  // namespace

TEST_F(IniFileTest, readWrite) {
    static const unordered_map<string, int> intData = {
            {"zeroInt", 0},
            {"positiveInt", 1},
            {"negativeInt", -1},
            {"maxInt", numeric_limits<int>::max()},
            {"minInt", numeric_limits<int>::min()},
            {"lowestInt", numeric_limits<int>::lowest()}};
    static const unordered_map<string, int64_t> int64Data = {
            {"zeroInt64", 0ULL},
            {"positiveInt64", 1ULL},
            {"negativeInt64", -1ULL},
            {"maxInt64", numeric_limits<int64_t>::max()},
            {"minInt64", numeric_limits<int64_t>::min()},
            {"lowestInt64", numeric_limits<int64_t>::lowest()}};
    static const unordered_map<string, double> doubleData = {
            {"zeroDouble", 0.0},
            {"positiveDouble", 1.5},
            {"negativeDouble", -1.5},
            {"maxDouble", numeric_limits<double>::max()},
            // minDouble fails because of rounding errors.
            // {"minDouble", numeric_limits<double>::min()},
            {"lowestDouble", numeric_limits<double>::lowest()}};

    // This doesn't actually test the format in which values are persisted.
    // But it does test that serialize-deserialize are consistent.
    static const unordered_map<string, bool> boolData = {{"trueKey", true},
                                                         {"falseKey", false}};
    static const unordered_map<string, IniFile::DiskSize> diskSizeData = {
            {"ds0", 0ULL},
            {"ds1000B", 1000ULL},
            {"ds1K", 1024ULL},
            {"ds5k", 5 * 1024ULL},
            {"ds1M", 1024 * 1024ULL},
            {"ds3G", 3 * 1024 * 1024 * 1024ULL}};

    for (const auto& keyval : intData) {
        mIni->setInt(keyval.first, keyval.second);
    }
    for (const auto& keyval : int64Data) {
        mIni->setInt64(keyval.first, keyval.second);
    }
    for (const auto& keyval : doubleData) {
        mIni->setDouble(keyval.first, keyval.second);
    }
    for (const auto& keyval : boolData) {
        mIni->setBool(keyval.first, keyval.second);
    }
    for (const auto& keyval : diskSizeData) {
        mIni->setDiskSize(keyval.first, keyval.second);
    }

    ASSERT_TRUE(mIni->write());

    mIni.reset(new IniFile(mIniFilePath));
    ASSERT_EQ(0, mIni->size());
    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(static_cast<int>(intData.size() + int64Data.size() +
                               doubleData.size() +  boolData.size() +
                               diskSizeData.size()),
              mIni->size());

    // Mix-up the order a bit.
    for (const auto& keyval : boolData) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_EQ(keyval.second, mIni->getBool(keyval.first, 99));
    }
    for (const auto& keyval : diskSizeData) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_EQ(keyval.second, mIni->getDiskSize(keyval.first, 99));
    }
    for (const auto& keyval : int64Data) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_EQ(keyval.second, mIni->getInt64(keyval.first, 99));
    }
    for (const auto& keyval : intData) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_EQ(keyval.second, mIni->getInt(keyval.first, 99));
    }
    for (const auto& keyval : doubleData) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_EQ(keyval.second, mIni->getDouble(keyval.first, 99));
    }
}

TEST_F(IniFileTest, duplicateAndMissingKeys) {
    mIni->setInt("int", 0);
    mIni->setInt("int", 1);
    mIni->setInt64("int64", 0ULL);
    mIni->setInt64("int64", 1ULL);
    mIni->setDouble("double", 0.0);
    mIni->setDouble("double", 1.1);
    mIni->setBool("bool", false);
    mIni->setBool("bool", true);
    mIni->setDiskSize("ds", 0ULL);
    mIni->setDiskSize("ds", 1ULL);

    ASSERT_TRUE(mIni->write());
    mIni.reset(new IniFile(mIniFilePath));
    ASSERT_EQ(0, mIni->size());
    ASSERT_TRUE(mIni->read());
    EXPECT_NE(0, mIni->size());

    EXPECT_EQ(1, mIni->getInt("int", 99));
    EXPECT_EQ(1LL, mIni->getInt64("int64", 99));
    EXPECT_EQ(1.1, mIni->getDouble("double", 99));
    EXPECT_EQ(true, mIni->getBool("bool", false));
    EXPECT_EQ(1ULL, mIni->getDiskSize("ds", 99ULL));

    EXPECT_EQ(-11, mIni->getInt("missing", -11));
    EXPECT_EQ(22LL, mIni->getInt64("missing", 22LL));
    EXPECT_EQ(3.3, mIni->getDouble("missing", 3.3));
    EXPECT_EQ(true, mIni->getBool("missing", true));
    EXPECT_EQ(44ULL, mIni->getDiskSize("missing", 44ULL));
}

TEST_F(IniFileTest, valueFormat) {
    static const vector<string> fileData = {
            "key1 = value with spaces", "key2 = value with trailing spaces  ",
            "key3 = \"value with redundant quotes\"", "keyAllSpaces =       "};
    writeIniFileData(fileData);

    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(fileData.size(), static_cast<size_t>(mIni->size()));
    EXPECT_EQ("value with spaces", mIni->getString("key1", ""));
    EXPECT_EQ("value with trailing spaces", mIni->getString("key2", ""));
    EXPECT_EQ("\"value with redundant quotes\"", mIni->getString("key3", ""));
    EXPECT_EQ("", mIni->getString("keyAllSpaces", "nonemptydefault"));
}

TEST_F(IniFileTest, readMalformedFile) {
    static const int defaultInt = -99;
    static const vector<string> fileData = {
            "a = 5",
            "; This comment will be skipped",
            "  b = 4",
            "   # So will this: irrelevant ; and #",
            "c=43",
            "d= 37malformedint,otherwiseOK",
            "This is actually malformed, and will be skipped with warning",
            " d = 45.6 now this becomes malformed here",
            "d = 43 ; hanging comments are not supported.",
            " ee = 546",
            "f=\"56\"",
            "f32ASDF_-.dfae3=1",
            "90=KeyMustStartWithAlpha",
            "a9%0=KeyCanNotContainPercent",
            ""};
    static const unordered_map<string, int> validEntries = {
            {"a", 5},
            {"b", 4},
            {"c", 43},
            {"d", defaultInt},
            {"ee", 546},
            {"f", defaultInt},
            {"f32ASDF_-.dfae3", 1}};

    writeIniFileData(fileData);

    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(validEntries.size(), static_cast<size_t>(mIni->size()));
    for (const auto& keyval : validEntries) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_EQ(keyval.second, mIni->getInt(keyval.first, defaultInt));
    }
}

static void formatToLines(vector<string>* lines,
                          const unordered_map<string, string>& dataMap) {
    for (const auto& keyval : dataMap) {
        lines->push_back(keyval.first + " = " + keyval.second);
    }
}

TEST_F(IniFileTest, boolFormat) {
    static const unordered_map<string, string> validTrues = {{"true1", "yes"},
                                                             {"true2", "YES"},
                                                             {"true3", "true"},
                                                             {"true4", "TRUE"},
                                                             {"true5", "1"}};
    static const unordered_map<string, string> validFalses = {
            {"false1", "no"},
            {"false2", "NO"},
            {"false3", "false"},
            {"flase4", "FALSE"},
            {"false5", "0"}};
    static const unordered_map<string, string> invalidTrues = {
            {"true12", "blah"}, {"true13", "\"1\""}};
    static const unordered_map<string, string> invalidFalses = {
            {"false12", "blah"}, {"false13", "\"0\""}};

    vector<string> lines;
    formatToLines(&lines, validTrues);
    formatToLines(&lines, invalidTrues);
    formatToLines(&lines, validFalses);
    formatToLines(&lines, invalidFalses);
    writeIniFileData(lines);

    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(lines.size(), static_cast<size_t>(mIni->size()));
    for (const auto& keyval : validTrues) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_TRUE(mIni->getBool(keyval.first, false));
    }
    for (const auto& keyval : validFalses) {
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_FALSE(mIni->getBool(keyval.first, true));
    }
    for (const auto& keyval : invalidTrues) {
        // The keyval exists, it's just not a valid bool value.
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_FALSE(mIni->getBool(keyval.first, false));
    }
    for (const auto& keyval : invalidFalses) {
        // The keyval exists, it's just not a valid bool value.
        EXPECT_TRUE(mIni->hasKey(keyval.first));
        EXPECT_TRUE(mIni->getBool(keyval.first, true));
    }
}

TEST_F(IniFileTest, diskSizeFormat) {
    static const unordered_map<string, string> validDiskSizes = {
            {"ThirtyB", "30"}, {"OneKilo", "1k"},  {"FiveKilo", "5K"},
            {"OneMega", "1m"}, {"FiveMega", "5M"}, {"OneGiga", "1g"},
            {"FiveGiga", "5G"}};
    static const unordered_map<string, string> invalidDiskSizes = {
            {"WrongUnit", "30hertz"},
            {"FractionalNumber", "2.14142135423"},
            {"FractionalKilo", "3.14K"},
            {"smiley_really", ";-)"}};

    vector<string> lines;
    formatToLines(&lines, validDiskSizes);
    formatToLines(&lines, invalidDiskSizes);
    writeIniFileData(lines);

    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(lines.size(), static_cast<size_t>(mIni->size()));
    EXPECT_EQ(30ULL, mIni->getDiskSize("ThirtyB", 99));
    EXPECT_EQ(1024ULL, mIni->getDiskSize("OneKilo", 99));
    EXPECT_EQ(1024 * 1024ULL, mIni->getDiskSize("OneMega", 99));
    EXPECT_EQ(1024 * 1024 * 1024ULL, mIni->getDiskSize("OneGiga", 99));
    EXPECT_EQ(5 * 1024ULL, mIni->getDiskSize("FiveKilo", 99));
    EXPECT_EQ(5 * 1024 * 1024ULL, mIni->getDiskSize("FiveMega", 99));
    EXPECT_EQ(5 * 1024 * 1024 * 1024ULL, mIni->getDiskSize("FiveGiga", 99));

    EXPECT_EQ(99ULL, mIni->getDiskSize("WrongUnit", 99));
    EXPECT_EQ(99ULL, mIni->getDiskSize("FractionalNumber", 99));
    EXPECT_EQ(99ULL, mIni->getDiskSize("FractionalKilo", 99));
    EXPECT_EQ(99ULL, mIni->getDiskSize("smiley_really", 99));
}

TEST_F(IniFileTest, discardEmpty) {
    mIni->setString("nonEmpty", "someValue");
    mIni->setString("empty", "");

    ASSERT_TRUE(mIni->write());
    mIni.reset(new IniFile(mIniFilePath));
    ASSERT_EQ(0, mIni->size());
    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(2, mIni->size());
    EXPECT_EQ("someValue", mIni->getString("nonEmpty", "defaultString"));
    EXPECT_EQ("", mIni->getString("empty", "defaultString"));

    EXPECT_TRUE(mIni->writeDiscardingEmpty());
    mIni.reset(new IniFile(mIniFilePath));
    ASSERT_EQ(0, mIni->size());
    ASSERT_TRUE(mIni->read());
    EXPECT_EQ(1, mIni->size());
    EXPECT_EQ("someValue", mIni->getString("nonEmpty", "defaultString"));
    EXPECT_EQ("defaultString", mIni->getString("empty", "defaultString"));
}

TEST_F(IniFileTest, writeIfChanged) {
    // Initially, we must treat the object as dirty.
    // Hence, we'll clear the lines we'd written previously.
    mIni.reset(new IniFile(mIniFilePath));
    verifyFileUpdated(true);

    mIni->write();
    // Now we consider the object as clean, so write shouldn't modify the
    // underlying file.
    verifyFileUpdated(false);

    // Now let's change the data.
    mIni->setString("random", "yippeeee");
    verifyFileUpdated(true);

    mIni->read();
    // No changes, so write shouldn't do anything.
    verifyFileUpdated(false);
    // But resetting the file path means we should flush.
    mIni->setBackingFile(mIniFilePath);
    verifyFileUpdated(true);
}

TEST_F(IniFileTest, noBackingFile) {
    mIni.reset(new IniFile());
    ASSERT_FALSE(mIni->read());

    mIni->setBackingFile(mIniFilePath);
    ASSERT_FALSE(mIni->read());
    ASSERT_TRUE(mIni->write());
    ASSERT_TRUE(mIni->read());
}

TEST_F(IniFileTest, iterator) {
    mIni->setString("firstKey", "firstValue");
    mIni->setString("secondKey", "secondValue");

    // Const iterators, also verify order.
    const IniFile& cIni = *mIni;
    vector<string> keys = {"firstKey", "secondKey"};
    ASSERT_EQ(keys.size(), static_cast<size_t>(mIni->size()));
    size_t i = 0;
    for (const auto& key : cIni) {
        EXPECT_EQ(keys[i], key);
        ++i;
    }
}

TEST_F(IniFileTest, diskFileOrder) {
    vector<string> lines = {
            "first = some valid value",  "second line is invalid",
            "; Third line is a comment", "fourth = is valid",
            "# Fifth is also a comment", "",
            ";Sixth was a comment",      "3p0 = is an invalid key",
            "lets = finish with valid"};
    vector<string> rewritten_lines = {"first = some valid value",
                                      "; Third line is a comment",
                                      "fourth = is valid",
                                      "# Fifth is also a comment",
                                      "",
                                      ";Sixth was a comment",
                                      "lets = finish with valid"};

    writeIniFileData(lines);
    ASSERT_TRUE(mIni->read());
    ASSERT_TRUE(mIni->write());
    verifyFileContents(rewritten_lines);

    // This is order indpendent. Let's try the reverse
    std::reverse(std::begin(lines), std::end(lines));
    std::reverse(std::begin(rewritten_lines), std::end(rewritten_lines));
    writeIniFileData(lines);
    ASSERT_TRUE(mIni->read());
    ASSERT_TRUE(mIni->write());
    verifyFileContents(rewritten_lines);

    // Extra keys are appended to the file.
    mIni->setString("extraKey", "extraValue");
    ASSERT_TRUE(mIni->write());
    rewritten_lines.push_back("extraKey = extraValue");
    verifyFileContents(rewritten_lines);

    // Test order of iteration in this complicated case.
    // Remeber we reversed the lines.
    vector<string> keys = {"lets", "fourth", "first", "extraKey"};
    ASSERT_EQ(keys.size(), static_cast<size_t>(mIni->size()));
    size_t i = 0;
    for (const auto& key : *mIni) {
        EXPECT_EQ(keys[i], key);
        ++i;
    }
}

TEST_F(IniFileTest, strDefaultValues) {
    ASSERT_EQ(0, mIni->size());
    EXPECT_TRUE(mIni->getBool("missingKey", "yes"));
    EXPECT_FALSE(mIni->getBool("missingKey", "no"));
    EXPECT_EQ(1024ULL, mIni->getDiskSize("missingKey", "1k"));
}

TEST_F(IniFileTest, makeValidKey) {
    EXPECT_STREQ("_key", IniFile::makeValidKey("key").c_str());
    EXPECT_STREQ("_", IniFile::makeValidKey("").c_str());
    EXPECT_STREQ("_.3D", IniFile::makeValidKey("=").c_str());
    EXPECT_STREQ("_a.20sign.20.23", IniFile::makeValidKey("a sign #").c_str());
    EXPECT_STREQ("_some.20number.2010.20within",
                 IniFile::makeValidKey("some number 10 within").c_str());
}

TEST(IniFileTest2, parseInMemory) {
    static const char data[] =
R"(key1=val1
key2=1011
key3=false
)";

    IniFile ini;
    ASSERT_TRUE(ini.readFromMemory(data));
    EXPECT_STREQ("", ini.getBackingFile().c_str());

    ASSERT_EQ(3, ini.size());
    EXPECT_STREQ("val1", ini.getString("key1", "").c_str());
    EXPECT_EQ(1011, ini.getInt64("key2", 1));
    EXPECT_FALSE(ini.getBool("key3", true));

    IniFile ini2(data, stringLiteralLength(data));
    ASSERT_EQ(3, ini2.size());
}

}  // namespace base
}  // namespace android
