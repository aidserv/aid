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

#include "android/metrics/Percentiles.h"

#include "android/metrics/proto/studio_stats.pb.h"

#include <cassert>

namespace android {
namespace metrics {

Percentiles::Percentiles(int rawDataSize, std::initializer_list<double> targets)
    : mRawDataSize(std::max<int>(rawDataSize, targets.size())),
      mTargets(targets.begin(), targets.end()),
      mBuckets(targets.size() * 2 + 3) {
    assert(mRawDataSize > 0);
    mRawData.reserve(mRawDataSize);
    std::sort(mTargets.begin(), mTargets.end());
}

void Percentiles::addSample(double val) {
    if (mCount < mRawDataSize) {
        assert((int)mRawData.size() == mCount);
        mRawData.push_back(val);
        mRawDataSorted = false;
        ++mCount;
        return;
    }

    if (mCount == mRawDataSize) {
        createBucketsFromRawData();
    }

    assert(mBuckets.size() > 2);

    ++mCount;
    if (mBuckets.front().value > val) {
        mBuckets.front().value = val;
    }
    if (mBuckets.back().value < val) {
        mBuckets.back().value = val;
    }

    for (int i = 1; i < (int)mBuckets.size() - 1; ++i) {
        mBuckets[i].optimalCount += mBuckets[i].target;
        if (mBuckets[i].value > val) {
            ++mBuckets[i].count;
        }
    }

    mBuckets.back().optimalCount += mBuckets.back().target;
    ++mBuckets.back().count;

    interpolateIfNecessary();
}

int Percentiles::targetCount() const {
    return mTargets.size();
}

base::Optional<double> Percentiles::target(int index) const {
    if (index < 0 || index >= (int)mTargets.size()) {
        return {};
    }
    return mTargets[index];
}

base::Optional<double> Percentiles::calcValueForTarget(double target) {
    return calcValueForTargetNo(
            std::find(mTargets.begin(), mTargets.end(), target) -
            mTargets.begin());
}

base::Optional<double> Percentiles::calcValueForTargetNo(int targetNo) {
    if (targetNo < 0 || targetNo >= (int)mTargets.size()) {
        return {};
    }
    if (mCount <= mRawDataSize) {
        // We didn't have enough data to interpolate yet.
        if (mCount == 0) {
            return {};
        } else {
            assert(!mRawData.empty());
            sortRawData();
            return mRawData[mCount * mTargets[targetNo]];
        }
    }

    for (const Bucket& b : mBuckets) {
        if (b.target == mTargets[targetNo]) {
            return b.value;
        }
    }
    return {};
}

void Percentiles::fillMetricsEvent(android_studio::PercentileEstimator* event) {
    assert(event);

    if (mCount <= mRawDataSize) {
        for (double val : mRawData) {
            event->add_raw_sample(val);
        }
    } else {
        for (const Bucket& b : mBuckets) {
            auto eventBucket = event->add_bucket();
            eventBucket->set_target_percentile(b.target);
            eventBucket->set_value(b.value);
            eventBucket->set_count(b.count);
        }
    }
}

void Percentiles::sortRawData() {
    if (!mRawDataSorted) {
        std::sort(mRawData.begin(), mRawData.end());
        mRawDataSorted = true;
    }
}

void Percentiles::createBucketsFromRawData() {
    sortRawData();
    mBuckets.front() = Bucket{0.0, mRawData[0], 0, mRawDataSize};
    double last = 0;
    int currentBucketIndex = 1;
    for (const double t : mTargets) {
        double target = (last + t) / 2;
        auto index = (int)(target * mRawDataSize);
        mBuckets[currentBucketIndex] =
                Bucket{target, mRawData[index], index, mRawDataSize};
        ++currentBucketIndex;
        target = t;
        index = (int)(target * mRawDataSize);
        mBuckets[currentBucketIndex] =
                Bucket{target, mRawData[index], index, mRawDataSize};
        currentBucketIndex++;
        last = t;
    }
    assert(currentBucketIndex == (int)mTargets.size() * 2 + 1);
    double target = (1.0 + last) / 2;
    auto index = (int)(target * mRawDataSize);
    mBuckets[currentBucketIndex] =
            Bucket{target, mRawData[index], index, mRawDataSize};
    ++currentBucketIndex;
    mBuckets[currentBucketIndex] =
            Bucket{1.0, mRawData.back(), mRawDataSize, mRawDataSize};
    assert(currentBucketIndex + 1 == (int)mBuckets.size());

    mRawData.clear();
    mRawData.shrink_to_fit();
    assert(mRawData.capacity() == 0);
}

void Percentiles::interpolateIfNecessary() {
    assert(mBuckets.size() > 2);
    for (int i = 1; i < (int)mBuckets.size() - 1; i++) {
        Bucket& b = mBuckets[i];
        const Bucket& prev = mBuckets[i - 1];
        const Bucket& next = mBuckets[i + 1];
        const double delta = b.optimalCount - b.count;
        if (delta < -1.0 && prev.count - b.count < -1) {
            updateBucket(&b, prev, next, -1.0);
        } else if (delta > 1.0 && next.count - b.count > 1) {
            updateBucket(&b, prev, next, 1.0);
        }
    }
}

void Percentiles::updateBucket(Percentiles::Bucket* b,
                               const Percentiles::Bucket& prev,
                               const Percentiles::Bucket& next,
                               double d) {
    // First try to update using quadratic interpolation.
    const double numerator =
            ((b->count - prev.count + d) * (next.value - b->value) /
             (next.count - b->count)) +
            ((next.count - b->count - d) * (b->value - prev.value) /
             (b->count - prev.count));
    double newValue = b->value + d * numerator / (next.count - prev.count);
    if (prev.value < newValue && newValue < next.value) {
        b->value = newValue;
    } else {
        // Linear interpolation instead...
        if (d < 0) {
            newValue = b->value -
                       (b->value - prev.value) / (b->count - prev.count);
        } else {
            newValue = b->value +
                       (next.value - b->value) / (next.count - b->count);
        }
        b->value = newValue;
    }
    b->count += (int64_t)d;
}

}  // namespace metrics
}  // namespace android
