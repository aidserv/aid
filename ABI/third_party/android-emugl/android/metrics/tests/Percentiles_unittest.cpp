// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/metrics/Percentiles.h"

#include "android/base/memory/LazyInstance.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/base/testing/GTestUtils.h"

#include <gtest/gtest.h>

#include <random>

using namespace android::base;
using namespace android::metrics;

static const int kNumSamples = 10000;
static const int kRawDataSize = 80;

static LazyInstance<std::mt19937> randEngine = {};

static std::vector<double> getNormalDistribution(int count, double offset = 0) {
    std::vector<double> res;
    res.reserve(count);

    std::normal_distribution<double> r;
    for (int i = 0; i < count; ++i) {
        res.push_back(r(*randEngine) + offset);
    }

    return res;
}

//------------------------------------------------------------------------------

TEST(PercentilesTest, basic) {
    auto p = Percentiles(kRawDataSize, {0.5});
    EXPECT_EQ(1, p.targetCount());
    EXPECT_EQ(0.5, p.target(0));
    EXPECT_FALSE(p.calcValueForTargetNo(0));
    EXPECT_FALSE(p.calcValueForTarget(0.5));
}

TEST(PercentilesTest, targets) {
    auto p = Percentiles(kRawDataSize, {0.25, 0.5, 0.75});
    EXPECT_EQ(3, p.targetCount());
    EXPECT_FALSE(p.target(-1));
    EXPECT_EQ(0.25, p.target(0));
    EXPECT_EQ(0.5, p.target(1));
    EXPECT_EQ(0.75, p.target(2));
    EXPECT_FALSE(p.target(3));

    EXPECT_FALSE(p.calcValueForTargetNo(0));

    p.addSample(1);
    EXPECT_FALSE(p.calcValueForTarget(0.2));
    EXPECT_DOUBLE_EQ(1, p.calcValueForTarget(0.25));
    EXPECT_DOUBLE_EQ(1, p.calcValueForTarget(0.50));
    EXPECT_DOUBLE_EQ(1, p.calcValueForTarget(0.75));
    EXPECT_FALSE(p.calcValueForTarget(0.9));

    p.addSamples({2, 3, 4, 5, 6});
    EXPECT_FALSE(p.calcValueForTarget(0.2));
    EXPECT_DOUBLE_EQ(1, p.calcValueForTarget(0.25));
    EXPECT_DOUBLE_EQ(1, p.calcValueForTarget(0.50));
    EXPECT_DOUBLE_EQ(1, p.calcValueForTarget(0.75));
    EXPECT_FALSE(p.calcValueForTarget(0.9));
}

TEST(PercentilesTest, constantDistribution) {
    auto p = Percentiles(kRawDataSize, {0.5});
    for (int i = 0; i < kNumSamples; ++i) {
        p.addSample(1.0);
    }

    EXPECT_NEAR(1.0, p.calcValueForTarget(0.5).valueOr(0), 0.01);
    EXPECT_NEAR(1.0, p.calcValueForTargetNo(0).valueOr(0), 0.01);
}

TEST(PercentilesTest, uniformDistribution) {
    std::uniform_real_distribution<double> r(0.0, 1.0);

    auto p = Percentiles(kRawDataSize, {0.25, 0.5, 0.75});
    for (int i = 0; i < kNumSamples; ++i) {
        p.addSample(r(*randEngine));
    }

    EXPECT_NEAR(0.25, p.calcValueForTarget(0.25).valueOr(0), 0.01);
    EXPECT_NEAR(0.5, p.calcValueForTarget(0.5).valueOr(0), 0.01);
    EXPECT_NEAR(0.75, p.calcValueForTarget(0.75).valueOr(0), 0.01);
    EXPECT_NEAR(0.25, p.calcValueForTargetNo(0).valueOr(0), 0.01);
    EXPECT_NEAR(0.5, p.calcValueForTargetNo(1).valueOr(0), 0.01);
    EXPECT_NEAR(0.75, p.calcValueForTargetNo(2).valueOr(0), 0.01);
}

TEST(PercentilesTest, bimodalDistributionTest) {
    std::uniform_real_distribution<double> r(0.0, 1.0);

    auto p = Percentiles(kRawDataSize, {0.5});
    for (int i = 0; i < kNumSamples; ++i) {
        double d = r(*randEngine);
        double v;
        if (d < 0.45) {
            v = 1.0;
        } else if (d < 0.55) {
            v = 2.0;
        } else {
            v = 3.0;
        }
        p.addSample(v);
    }

    EXPECT_NEAR(2.0, p.calcValueForTarget(0.5).valueOr(0), 0.05);
    EXPECT_NEAR(2.0, p.calcValueForTargetNo(0).valueOr(0), 0.05);
}

TEST(PercentilesTest, normalDistributionTest) {
    auto p = Percentiles(kRawDataSize, {0.5});
    p.addSamples(getNormalDistribution(kNumSamples, 6.0));
    EXPECT_NEAR(6.0, p.calcValueForTarget(0.5).valueOr(0), 0.03);
    EXPECT_NEAR(6.0, p.calcValueForTargetNo(0).valueOr(0), 0.03);
}

TEST(PercentilesTest, normalMultiplePointsTest) {
    auto p = Percentiles(kRawDataSize, {0.4, 0.5, 0.6});
    auto entries = getNormalDistribution(kNumSamples, 6.0);
    p.addSamples(entries);

    std::sort(entries.begin(), entries.end());
    const double actual40th = entries[(int)(0.4 * entries.size())];
    const double actual50th = entries[(int)(0.5 * entries.size())];
    const double actual60th = entries[(int)(0.6 * entries.size())];

    EXPECT_NEAR(actual40th, p.calcValueForTarget(0.4).valueOr(actual40th - 100),
                0.03);
    EXPECT_NEAR(actual50th, p.calcValueForTarget(0.5).valueOr(actual50th - 100),
                0.03);
    EXPECT_NEAR(actual60th, p.calcValueForTarget(0.6).valueOr(actual60th - 100),
                0.03);
    EXPECT_NEAR(actual40th, p.calcValueForTargetNo(0).valueOr(actual40th - 100),
                0.03);
    EXPECT_NEAR(actual50th, p.calcValueForTargetNo(1).valueOr(actual50th - 100),
                0.03);
    EXPECT_NEAR(actual60th, p.calcValueForTargetNo(2).valueOr(actual60th - 100),
                0.03);
}

TEST(PercentilesTest, tailQuantile) {
  // Test accuracy at the tails, which will be generally worse.
  auto p = Percentiles(kRawDataSize, {0.01, 0.99});
  auto entries = getNormalDistribution(kNumSamples);
  p.addSamples(entries);

  std::sort(entries.begin(), entries.end());
  const double actual1st = entries[(int)(0.01 * entries.size())];
  const double actual99th = entries[(int)(0.99 * entries.size())];

  EXPECT_NEAR(actual1st, p.calcValueForTarget(0.01).valueOr(actual1st - 100),
              0.4);
  EXPECT_NEAR(actual99th, p.calcValueForTarget(0.99).valueOr(actual99th - 100),
              0.4);
  EXPECT_NEAR(actual1st, p.calcValueForTargetNo(0).valueOr(actual1st - 100),
              0.4);
  EXPECT_NEAR(actual99th, p.calcValueForTargetNo(1).valueOr(actual99th - 100),
              0.4);
}

TEST(PercentilesTest, exportTest) {
    {
        auto p = Percentiles(kRawDataSize, {0.5});
        p.addSamples(getNormalDistribution(kNumSamples));

        android_studio::PercentileEstimator event;
        p.fillMetricsEvent(&event);

        EXPECT_EQ(0, event.raw_sample_size());
        EXPECT_EQ(5, event.bucket_size());
        EXPECT_TRUE(event.bucket(2).has_value());
        EXPECT_EQ(p.calcValueForTargetNo(0), event.bucket(2).value());
        EXPECT_TRUE(event.bucket(2).has_target_percentile());
        EXPECT_EQ(p.target(0), event.bucket(2).target_percentile());
        EXPECT_TRUE(event.bucket(2).has_count());
    }

    {
        auto p = Percentiles(kRawDataSize, {0.5});
        auto entries = getNormalDistribution(kRawDataSize);
        p.addSamples(entries);

        android_studio::PercentileEstimator event;
        p.fillMetricsEvent(&event);

        EXPECT_EQ(0, event.bucket_size());
        EXPECT_EQ(kRawDataSize, event.raw_sample_size());
        EXPECT_TRUE(RangesMatch(entries, event.raw_sample()));
    }
}
