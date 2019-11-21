/* Copyright (C) 2007-2015 The Android Open Source Project
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

#include "android/skin/rect.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A SkinSurface models a 32-bit ARGB pixel image that can be blitted to or from
 */
typedef struct SkinSurface  SkinSurface;

struct SkinScaler;

/* Decrement a surface's reference count. Takes the surface's address as parameter.
   It will be set to NULL on exit */
extern void          skin_surface_unrefp( SkinSurface*  *psurface );

extern int skin_surface_width(SkinSurface* s);
extern int skin_surface_height(SkinSurface* s);

/* Create a surface for the given dimensions. The "original" dimensions are the dimensions
 * of the original image, while the regular dimensions are those at which the image is scaled to
 * for display.
 */
extern SkinSurface*  skin_surface_create( int w, int h, int original_w, int original_h );

/* Create a surface from a given pixel buffer. */
extern SkinSurface*  skin_surface_create_argb32_from(
                            int                  w,
                            int                  h,
                            int                  pitch,
                            uint32_t*            pixels);

/* Check if a surface needs to be resized at all: only when the "original" dimensions change
 * does the surface need to be re-created. Otherwise, it can simply be updated with the appropriate
 * information to avoid extra computation. If given a NULL surface, create a new one.
 */
extern SkinSurface*  skin_surface_resize(
                            SkinSurface* surface,
                            int w,
                            int h,
                            int original_w,
                            int original_h );

extern void skin_surface_create_window(SkinSurface* surface,
                                       int x,
                                       int y,
                                       int w,
                                       int h);

extern void skin_surface_reverse_map(SkinSurface* surface,
                                     int* x,
                                     int* y);

extern void skin_surface_get_scaled_rect(SkinSurface* surface,
                                         const SkinRect* srect,
                                         SkinRect* drect);

/* Surface pixels information for surfaces */
typedef struct {
    int         w;
    int         h;
    int         pitch;
    uint32_t*   pixels;
} SkinSurfacePixels;

typedef struct {
    uint32_t r_shift;
    uint32_t r_mask;
    uint32_t g_shift;
    uint32_t g_mask;
    uint32_t b_shift;
    uint32_t b_mask;
    uint32_t a_shift;
    uint32_t a_mask;
} SkinSurfacePixelFormat;

extern void    skin_surface_update(SkinSurface* surface, SkinRect* rect);

extern void    skin_surface_upload(SkinSurface* surface,
                                   const SkinRect* rect,
                                   const void* pixels,
                                   int pitch);

/* List of composition operators for the blit routine */
typedef enum {
    SKIN_BLIT_COPY = 0,
    SKIN_BLIT_SRCOVER,
} SkinBlitOp;


/* Blit a surface into another one */
extern void    skin_surface_blit( SkinSurface*  dst,
                                  SkinPos*      dst_pos,
                                  SkinSurface*  src,
                                  SkinRect*     src_rect,
                                  SkinBlitOp    blitop );

/* Blit a colored rectangle into a destination surface */
extern void    skin_surface_fill(SkinSurface*  dst,
                                 SkinRect*     rect,
                                 uint32_t      argb_premul);

#ifdef __cplusplus
}
#endif
