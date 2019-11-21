// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/Optional.h"

#include <cstdint>
#include <algorithm>
#include <initializer_list>
#include <vector>

namespace android_studio {
class PercentileEstimator;
}

namespace android {
namespace metrics {

//
// This class is a C++ implementation of a corresponding Android Studio code from
//    https://android.googlesource.com/platform/tools/analytics-library/+/studio-master-dev/tracker/src/main/java/com/android/tools/analytics
//
// Percentiles class creates an estimation of the value at target percentiles
// from a data stream. It is based on the P-square algorithm found at:
//    http://pierrechainais.ec-lille.fr/Centrale/Option_DAD/IMPACT_files/Dynamic%20quantiles%20calcultation%20-%20P2%20Algorythm.pdf
// with extensions to allow monitoring multiple percentiles.
//

class Percentiles {
public:
    // Constructor.
    // |rawDataSize| - number of samples before interpolating.
    // |targets| - percentiles to monitor.
    explicit Percentiles(int rawDataSize,
                         std::initializer_list<double> targets);

    // Adds a sample to the estimator, updates buckets if necessary.
    void addSample(double val);

    // Range versions to add multiple samples at once.
    template <class Range>
    void addSamples(const Range& range) {
        std::for_each(std::begin(range), std::end(range),
                      [this](double val) { addSample(val); });
    }
    template <class T>
    void addSamples(const std::initializer_list<T>& range) {
        addSamples<std::initializer_list<T>>(range);
    }

    // Returns the number of tracked percentiles.
    int targetCount() const;
    // Returns the value of tracked percentile #|index|, or empty optional if
    // |index| is out of range.
    base::Optional<double> target(int index) const;

    // Calculates the estimated value at a percentile |target| (first overload)
    // or target(|targetNo|) (second overload). Returns empty optional if the
    // passed percentile is not being tracked, or if there is no samples added
    // to the object yet.
    base::Optional<double> calcValueForTarget(double target);
    base::Optional<double> calcValueForTargetNo(int targetNo);

    // Fills in a metrics event with the current state.
    void fillMetricsEvent(android_studio::PercentileEstimator* event);

private:
    // A single tracked bucket.
    struct Bucket {
        Bucket() = default;
        Bucket(double target, double value, int64_t count, int optimalCount)
            : target(target),
              value(value),
              count(count),
              optimalCount(optimalCount) {}

        double target = 0;
        double value = 0;
        int64_t count = 0;
        double optimalCount = 0;
    };

    void sortRawData();
    void createBucketsFromRawData();
    void interpolateIfNecessary();

    static void updateBucket(Bucket* b,
                             const Bucket& prev,
                             const Bucket& next,
                             double d);

    int mRawDataSize;
    int mCount = 0;
    std::vector<double> mTargets;
    std::vector<Bucket> mBuckets;
    std::vector<double> mRawData;
    bool mRawDataSorted = false;
};

}  // namespace metrics
}  // namespace android
