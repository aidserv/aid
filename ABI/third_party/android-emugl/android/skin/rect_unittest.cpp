/* Copyright (C) 2014 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/skin/rect.h"

#include <gtest/gtest.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof((x)[0]))

namespace android_skin {

TEST(rect, skin_pos_rotate) {
    static const struct {
        SkinPos expected;
        SkinPos pos;
        SkinRotation rotation;
    } kData[] = {
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_0 },
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_90 },
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_180 },
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_270 },

        { { 100, 0 }, { 100, 0 }, SKIN_ROTATION_0 },
        { { 0, -100 }, { 100, 0 }, SKIN_ROTATION_90 },
        { { -100, 0 }, { 100, 0 }, SKIN_ROTATION_180 },
        { { 0, 100 }, { 100, 0 }, SKIN_ROTATION_270 },

        { { 0, 100 }, { 0, 100 }, SKIN_ROTATION_0 },
        { { 100, 0 }, { 0, 100 }, SKIN_ROTATION_90 },
        { { 0, -100 }, { 0, 100 }, SKIN_ROTATION_180 },
        { { -100, 0 }, { 0, 100 }, SKIN_ROTATION_270 },

        { { -100, 0 }, { -100, 0 }, SKIN_ROTATION_0 },
        { { 0, 100 }, { -100, 0 }, SKIN_ROTATION_90 },
        { { 100, 0 }, { -100, 0 }, SKIN_ROTATION_180 },
        { { 0, -100 }, { -100, 0 }, SKIN_ROTATION_270 },

        { { 0, -100 }, { 0, -100 }, SKIN_ROTATION_0 },
        { { -100, 0 }, { 0, -100 }, SKIN_ROTATION_90 },
        { { 0, 100 }, { 0, -100 }, SKIN_ROTATION_180 },
        { { 100, 0 }, { 0, -100 }, SKIN_ROTATION_270 },

        { { 100, 200 }, { 100, 200 }, SKIN_ROTATION_0 },
        { { 200, -100 }, { 100, 200 }, SKIN_ROTATION_90 },
        { { -100, -200 }, { 100, 200 }, SKIN_ROTATION_180 },
        { { -200, 100 }, { 100, 200 }, SKIN_ROTATION_270 },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        SkinPos result = kData[n].pos;
        // Note: this checks that the function works when |src == dst|.
        skin_pos_rotate(&result, &result, kData[n].rotation);
        EXPECT_EQ(kData[n].expected.x, result.x) << "#" << n;
        EXPECT_EQ(kData[n].expected.y, result.y) << "#" << n;
    }
}

TEST(rect, skin_size_contains) {
    static const struct {
        bool expected;
        SkinSize size;
        int x;
        int y;
    } kData[] = {
        { false, { 0, 0 }, 0, 0 },
        { true, { 1, 1 }, 0, 0 },
        { false, { 1, 1 }, 1, 1 },
        { true, { 10, 20 }, 0, 0 },
        { false, { 10, 20 }, 10, 0 },
        { false, { 10, 20 }, 0, 20 },
        { false, { 10, 20 }, 10, 20 },
        { true, { 10, 20 }, 9, 19 },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(kData[n].expected,
                  skin_size_contains(&kData[n].size,
                                     kData[n].x,
                                     kData[n].y)) << "#" << n;
    }
}

TEST(rect, skin_size_rotate) {
    static const struct {
        SkinSize expected;
        SkinSize size;
        SkinRotation rotation;
    } kData[] = {
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_0 },
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_90 },
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_180 },
        { { 0, 0 }, { 0, 0 }, SKIN_ROTATION_270 },

        { { 100, 0 }, { 100, 0 }, SKIN_ROTATION_0 },
        { { 0, 100 }, { 100, 0 }, SKIN_ROTATION_90 },
        { { 100, 0 }, { 100, 0 }, SKIN_ROTATION_180 },
        { { 0, 100 }, { 100, 0 }, SKIN_ROTATION_270 },

        { { 0, 100 }, { 0, 100 }, SKIN_ROTATION_0 },
        { { 100, 0 }, { 0, 100 }, SKIN_ROTATION_90 },
        { { 0, 100 }, { 0, 100 }, SKIN_ROTATION_180 },
        { { 100, 0 }, { 0, 100 }, SKIN_ROTATION_270 },

        { { 100, 200 }, { 100, 200 }, SKIN_ROTATION_0 },
        { { 200, 100 }, { 100, 200 }, SKIN_ROTATION_90 },
        { { 100, 200 }, { 100, 200 }, SKIN_ROTATION_180 },
        { { 200, 100 }, { 100, 200 }, SKIN_ROTATION_270 },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        SkinSize result = kData[n].size;
        // Note: this checks that the function works when |src == dst|.
        skin_size_rotate(&result, &result, kData[n].rotation);
        EXPECT_EQ(kData[n].expected.w, result.w) << "#" << n;
        EXPECT_EQ(kData[n].expected.h, result.h) << "#" << n;
    }
}

TEST(rect, skin_rect_init) {
    SkinRect r;
    skin_rect_init(&r, 10, 20, 30, 40);
    EXPECT_EQ(10, r.pos.x);
    EXPECT_EQ(20, r.pos.y);
    EXPECT_EQ(30, r.size.w);
    EXPECT_EQ(40, r.size.h);

    skin_rect_init(&r, 10, 20, -30, 40);
    EXPECT_EQ(0, r.pos.x);
    EXPECT_EQ(0, r.pos.y);
    EXPECT_EQ(0, r.size.w);
    EXPECT_EQ(0, r.size.h);

    skin_rect_init(&r, 10, 20, 30, -40);
    EXPECT_EQ(0, r.pos.x);
    EXPECT_EQ(0, r.pos.y);
    EXPECT_EQ(0, r.size.w);
    EXPECT_EQ(0, r.size.h);

    skin_rect_init(&r, 10, 20, -30, -40);
    EXPECT_EQ(0, r.pos.x);
    EXPECT_EQ(0, r.pos.y);
    EXPECT_EQ(0, r.size.w);
    EXPECT_EQ(0, r.size.h);
}

TEST(rect, skin_rect_translate) {
    SkinRect r;
    skin_rect_init(&r, 10, 30, 50, 70);
    skin_rect_translate(&r, +5, -5);
    EXPECT_EQ(15, r.pos.x);
    EXPECT_EQ(25, r.pos.y);
    EXPECT_EQ(50, r.size.w);
    EXPECT_EQ(70, r.size.h);
}

TEST(rect, skin_rect_rotate) {
    static const struct {
        SkinRect expected;
        SkinRect rect;
        SkinRotation rotation;
    } kData[] = {
        { { { 10, 5 }, { 30, 70 } }, { { 10, 5 }, { 30, 70 } }, SKIN_ROTATION_0 },
        { { { -75, 10 }, { 70, 30 } }, { { 10, 5 }, { 30, 70 } }, SKIN_ROTATION_90 },
        { { { -40, -75 }, { 30, 70 } }, { { 10, 5 }, { 30, 70 } }, SKIN_ROTATION_180 },
        { { { 5, -40 }, { 70, 30 } }, { { 10, 5 }, { 30, 70 } }, SKIN_ROTATION_270 },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        SkinRect result = kData[n].rect;
        SkinRect expected = kData[n].expected;
        skin_rect_rotate(&result, &result, kData[n].rotation);
        EXPECT_EQ(expected.pos.x, result.pos.x) << "#" << n;
        EXPECT_EQ(expected.pos.y, result.pos.y) << "#" << n;
        EXPECT_EQ(expected.size.w, result.size.w) << "#" << n;
        EXPECT_EQ(expected.size.h, result.size.h) << "#" << n;
    }
}

TEST(rect, skin_rect_contains) {
    static const struct {
        bool expected;
        SkinRect rect;
        int x;
        int y;
    } kData[] = {
        { false, { { 0, 0 }, { 0, 0 } }, 0, 0 },
        { true, { { 0, 0 }, { 1, 1 } }, 0, 0 },

        { false, { { 10, 5 }, { 30, 70 } }, 9, 5 },
        { false, { { 10, 5 }, { 30, 70 } }, 10, 4 },
        { false, { { 10, 5 }, { 30, 70 } }, 40, 5 },
        { false, { { 10, 5 }, { 30, 70 } }, 10, 75 },
        { false, { { 10, 5 }, { 30, 70 } }, 40, 75 },

        { true, { { 10, 5 }, { 30, 70 } }, 10, 5 },
        { true, { { 10, 5 }, { 30, 70 } }, 39, 5 },
        { true, { { 10, 5 }, { 30, 70 } }, 10, 74 },
        { true, { { 10, 5 }, { 30, 70 } }, 39, 74 },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(kData[n].expected,
                  skin_rect_contains(&kData[n].rect,
                                     kData[n].x,
                                     kData[n].y)) << "#" << n;
    }
}

TEST(rect, skin_rect_contains_rect) {
    static const struct {
        SkinOverlap expected;
        SkinRect rect1;
        SkinRect rect2;
    } kData[] = {
        { SKIN_OUTSIDE, { { 0, 0 }, { 0, 0 } }, { { 0, 0 }, { 0, 0 } } },
        { SKIN_OUTSIDE, { { 0, 0 }, { 10, 20 } }, { { 10, 20 }, { 5, 5 } } },
        { SKIN_INSIDE, { { 0, 0 }, { 10, 20 } }, { { 5, 3 }, { 5, 5 } } },
        { SKIN_OVERLAP, { { 0, 0 }, { 10, 20 } }, { { 5, 3 }, { 15, 5 } } },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(kData[n].expected,
                  skin_rect_contains_rect(&kData[n].rect1,
                                          &kData[n].rect2)) << "#" << n;
    }
}

TEST(rect, skin_rect_intersect) {
    static const struct {
        bool expected;
        SkinRect result;
        SkinRect r1;
        SkinRect r2;
    } kData[] = {
        { false, {{ 0, 0},{ 0, 0}}, {{ 0, 0},{10,20}}, {{20, 0},{10,20}} },
        { true, {{20,20},{20,15}}, {{10,20},{30,30}}, {{20,10},{50,25}} },
        { true, {{20,40},{20,10}}, {{10,20},{30,30}}, {{20,40},{50,25}} },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        SkinRect result = { { 0, 0 }, { 0, 0 } };
        EXPECT_EQ(kData[n].expected,
                  skin_rect_intersect(&result, &kData[n].r1, &kData[n].r2))
                << "#" << n;
        if (kData[n].expected) {
            EXPECT_EQ(kData[n].result.pos.x, result.pos.x) << "#" << n;
            EXPECT_EQ(kData[n].result.pos.y, result.pos.y) << "#" << n;
            EXPECT_EQ(kData[n].result.size.w, result.size.w) << "#" << n;
            EXPECT_EQ(kData[n].result.size.h, result.size.h) << "#" << n;
        }
    }
}

TEST(rect, skin_rect_equals) {
    static const struct {
        bool expected;
        SkinRect r1;
        SkinRect r2;
    } kData[] = {
        { false, {{ 0, 0},{10,20}}, {{20, 0},{10,20}} },

        { true, {{10,20},{30,40}}, {{10,20},{30,40}} },

        { false, {{10,20},{30,40}}, {{10,-20},{30,40}} },
        { false, {{10,20},{30,40}}, {{-10,20},{30,40}} },
        { false, {{10,20},{30,40}}, {{-10,-20},{30,40}} },
        { false, {{10,20},{30,40}}, {{-10,-20},{30,40}} },

        { false, {{10,20},{30,40}}, {{10,20},{35,40}} },
        { false, {{10,20},{30,40}}, {{10,20},{30,45}} },
        { false, {{10,20},{30,40}}, {{10,20},{35,45}} },
    };
    const size_t kDataLen = ARRAYLEN(kData);
    for (size_t n = 0; n < kDataLen; ++n) {
        const SkinRect* r1 = &kData[n].r1;
        const SkinRect* r2 = &kData[n].r2;
        EXPECT_EQ(kData[n].expected, skin_rect_equals(r1, r2))
                << "#" << n
                << " r1.x=" << r1->pos.x
                << " r1.y=" << r1->pos.y
                << " r1.w=" << r1->size.w
                << " r1.h=" << r1->size.h
                << " r2.x=" << r2->pos.x
                << " r2.y=" << r2->pos.y
                << " r2.w=" << r2->size.w
                << " r2.h=" << r2->size.h;
    }
}

TEST(rect, skin_box_init) {
    SkinBox box = { 0, 0, 0, 0 };
    skin_box_init(&box, 10, 20, 30, 40);
    EXPECT_EQ(10, box.x1);
    EXPECT_EQ(20, box.y1);
    EXPECT_EQ(30, box.x2);
    EXPECT_EQ(40, box.y2);
}

TEST(rect, skin_box_from_rect) {
    SkinBox box = { 0, 0, 0, 0 };
    const SkinRect rect = { { 10, 20 }, { 30, 40 } };
    skin_box_from_rect(&box, &rect);
    EXPECT_EQ(10, box.x1);
    EXPECT_EQ(20, box.y1);
    EXPECT_EQ(40, box.x2);
    EXPECT_EQ(60, box.y2);
}

TEST(rect, skin_box_to_rect) {
    const SkinBox box = { 10, 20, 40, 60 };
    SkinRect rect;
    skin_box_to_rect(&box, &rect);
    EXPECT_EQ(10, rect.pos.x);
    EXPECT_EQ(20, rect.pos.y);
    EXPECT_EQ(30, rect.size.w);
    EXPECT_EQ(40, rect.size.h);
}

TEST(rect, skin_box_minmax_init) {
    SkinBox box = { 0, 0, 0, 0 };
    skin_box_minmax_init(&box);
}

TEST(rect, skin_box_minmax_to_rect) {
    const SkinRect r1 = {{10, 20},{30,40}};
    const SkinRect r2 = {{100,25},{50,50}};
    SkinBox box;
    skin_box_minmax_init(&box);

    skin_box_minmax_update(&box, &r1);
    skin_box_minmax_update(&box, &r2);

    SkinRect rect;
    skin_box_minmax_to_rect(&box, &rect);
    EXPECT_EQ(10,rect.pos.x);
    EXPECT_EQ(20,rect.pos.y);
    EXPECT_EQ(140,rect.size.w);
    EXPECT_EQ(55,rect.size.h);
}

}  // namespace android_skin
