/* Copyright (C) 2007-2008 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

/**  Rectangles
 **/

// List of values used to identify a clockwise 90-degree rotation.
typedef enum {
    SKIN_ROTATION_0,
    SKIN_ROTATION_90,
    SKIN_ROTATION_180,
    SKIN_ROTATION_270
} SkinRotation;

// A structure used to model a position in pixels on the screen.
// |x| is the horizontal coordinate.
// |y| is the vertical coordinate.
// By convention, (0,0) is the top-left pixel.
typedef struct {
    int  x, y;
} SkinPos;

// Rotates the given rotation by the given amount, i.e.
// skin_rotation_rotate(SKIN_ROTATION_90, SKIN_ROTATION_180)
// will return SKIN_ROTATION_270
extern SkinRotation skin_rotation_rotate(SkinRotation rotation, SkinRotation by);

// Rotate input position |src| by rotation |rot|, writing the result into
// position |*dst|. One can use the same address for |src| and |dst|.
extern void skin_pos_rotate(SkinPos* dst,
                            const SkinPos* src,
                            SkinRotation rot);

// A structure used to model dimesions in pixels.
typedef struct {
    int  w, h;
} SkinSize;

extern void skin_size_rotate(SkinSize* dst,
                             const SkinSize* src,
                             SkinRotation rot);

extern bool skin_size_contains(const SkinSize* size, int x, int y);

// A structure used to model a rectangle in pixels.
// |pos| is the location of the rectangle's origin (top-left corner).
// |size| is the dimension in integer pixels of the rectangle.
typedef struct {
    SkinPos   pos;
    SkinSize  size;
} SkinRect;

// Initialize a given SkinRect structure with direct values.
// |x| and |y| are the top-left corner coordinates, and |w| and |h| are
// the width and height in pixels, respectively.
extern void  skin_rect_init(SkinRect* r, int x, int y, int w, int h);

// Translate a given SkinRect. This modifies the rectangle's |pos| field
// by adding |dx| and |dy| to its coordinates.
extern void  skin_rect_translate(SkinRect* r, int dx, int dy);

// Rotate a given SkinRect. This applies the clockwise |rotation| to the
// input |src| rectangle, and writes the result into |dst|.
// Note that one can use the same address for |src| and |dst|.
extern void  skin_rect_rotate(SkinRect* dst,
                              const SkinRect* src,
                              SkinRotation  rotation);

// Return true iff a given rectangle |r| contains the pixel at coordinates
// |x| and |y|.
extern bool skin_rect_contains(const SkinRect* r, int x, int y);


// Compute the intersection of two rectangles |r1| and |r2|, and store
// the result into |result|.
extern bool  skin_rect_intersect(SkinRect* result,
                                const SkinRect* r1,
                                const SkinRect* r2);

// Returns true iff two rectangles |r1| and |r2| are equal.
extern bool skin_rect_equals(const SkinRect* r1, const SkinRect* r2);

// Return a value that indicates if/how two rectangle |r1| and |r2| intersect.
// SKIN_OUTSIDE means the two rectangles do not intersect at all.
// SKIN_INSIDE indicates that |r1| is completely contained inside |r2|.
// SKIN_OVERLAP indicates that the two rectangles interesect, but also
// have non-common pixels.
typedef enum {
    SKIN_OUTSIDE = 0,
    SKIN_INSIDE  = 1,
    SKIN_OVERLAP = 2
} SkinOverlap;

extern SkinOverlap  skin_rect_contains_rect(const SkinRect* r1,
                                            const SkinRect* r2);

// Alternative model for a rectangle, that uses the coordinates of
// opposite corners instead. This is more convenient to compute
// accumulated min/max bounds.
// |x1| and |y1| are the coordinates of the top-left corner.
// |x2| and |y2| are the coordinates of the bottom-right corner.
typedef struct {
    int  x1, y1;
    int  x2, y2;
} SkinBox;

// Initialize a SkinBox structure |box| with coordinates |x1|, |y1|,
// |x2| and |y2|.
extern void skin_box_init(SkinBox* box, int x1, int y1, int x2, int y2) ;

// Initialize a SkinBox structure to perform the computation of accumulated
// rectangle bounds. This function is typically used in code that looks like:
//
//    SkinBox box;
//    skin_box_minmax_init(&box);
//    ..foreach rectangle r do
//       skin_box_minmax_update(&box, &r);
//
//    SkinRect bounds;
//    skin_box_minmax_to_rect(&box, &bounds);
//
// Which computes in |bounds| the enclosing bounds of all rectangles
// from the loop.
//
extern void  skin_box_minmax_init( SkinBox*  box );

// Add a new rectangle to the bounds minmax computation.
// |box| is the SkinBox used to perform the computation, and |rect|
// a rectangle.
extern void  skin_box_minmax_update(SkinBox*  box, const SkinRect* rect);

// Extract the bounds of a previous minmax computation as a rectangle.
// |box| is the SkinBox used to perform the computation, and |rect| will
// receive the final bounds.
extern int   skin_box_minmax_to_rect(const SkinBox* box, SkinRect* rect);

// Convert a rectangle |rect| into the equivalent SkinBox |box|.
extern void  skin_box_from_rect(SkinBox* box, const SkinRect* rect);

// Convert a SkinBox |box| into the equivalent SkinRect |rect|.
extern void  skin_box_to_rect(const SkinBox* box, SkinRect* rect);

ANDROID_END_HEADER
