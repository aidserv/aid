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
#include "android/gps.h"
#include "android/utils/debug.h"
#include "android/utils/stralloc.h"

#include <string.h>

CSerialLine* android_gps_serial_line;

#define  D(...)  VERBOSE_PRINT(gps,__VA_ARGS__)

void
android_gps_send_nmea( const char*  sentence )
{
    if (sentence == NULL)
        return;

    D("sending '%s'", sentence);

    if (android_gps_serial_line == NULL) {
        D("missing GPS channel, ignored");
        return;
    }

    android_serialline_write( android_gps_serial_line, (const void*)sentence, strlen(sentence) );
    android_serialline_write( android_gps_serial_line, (const void*)"\n", 1 );
}

////////////////////////////////////////////////////////////
//
// android_gps_send_location
//
// Send a GPS location to the AVD using an NMEA sentence
//
// Inputs: latitude:        Degrees
//         longitude:       Degrees
//         metersElevation: Meters above sea level
//         nSatellites:     Number of satellites used
//         time:            UTC, in the format provided
//                          by gettimeofday()

void
android_gps_send_location(double latitude, double longitude,
                          double metersElevation, int nSatellites,
                          const struct timeval *time)
{
    STRALLOC_DEFINE(msgStr);
    STRALLOC_DEFINE(elevationStr);
    char* elevStrPtr;

    int      deg, min;
    char     hemi;
    int      hh = 0, mm = 0, ss = 0;

    // Format overview:
    //    time of fix      123519     12:35:19 UTC
    //    latitude         4807.038   48 degrees, 07.038 minutes
    //    north/south      N or S
    //    longitude        01131.000  11 degrees, 31. minutes
    //    east/west        E or W
    //    fix quality      1          standard GPS fix
    //    satellites       1 to 12    number of satellites being tracked
    //    HDOP             <dontcare> horizontal dilution
    //    altitude         546.       altitude above sea-level
    //    altitude units   M          to indicate meters
    //    diff             <dontcare> height of sea-level above ellipsoid
    //    diff units       M          to indicate meters (should be <dontcare>)
    //    dgps age         <dontcare> time in seconds since last DGPS fix
    //    dgps sid         <dontcare> DGPS station id

    // time->tv_sec is elapsed seconds since epoch, UTC
    hh = (int) (time->tv_sec / (60 * 60)) % 24;
    mm = (int) (time->tv_sec /  60      ) % 60;
    ss = (int) (time->tv_sec            ) % 60;

    stralloc_add_format( msgStr, "$GPGGA,%02d%02d%02d", hh, mm, ss);

    // then the latitude
    hemi = 'N';
    if (latitude < 0) {
        hemi = 'S';
        latitude = -latitude;
    }
    deg = (int) latitude;
    latitude = 60*(latitude - deg);
    min = (int) latitude;
    latitude = 10000*(latitude - min);
    stralloc_add_format( msgStr, ",%02d%02d.%04d,%c", deg, min, (int)latitude, hemi );

    // The longitude
    hemi = 'E';
    if (longitude < 0) {
        hemi = 'W';
        longitude  = -longitude;
    }
    deg = (int) longitude;
    longitude = 60*(longitude - deg);
    min = (int) longitude;
    longitude = 10000*(longitude - min);
    stralloc_add_format( msgStr, ",%02d%02d.%04d,%c", deg, min, (int)longitude, hemi );

    // Bogus fix quality (1), satellite count, and bogus dilution
    stralloc_add_format( msgStr, ",1,6,");

    // Altitude (to 0.1 meter precision) + bogus diff
    // Make sure elevation is formatted with a decimal point instead of comma.
    // setlocale isn't used because of thread safety concerns.
    stralloc_add_format( elevationStr, "%.1f", metersElevation );
    for (elevStrPtr = stralloc_cstr(elevationStr); *elevStrPtr; ++elevStrPtr) {
        if (*elevStrPtr == ',') {
            *elevStrPtr = '.';
            break;
        }
    }
    stralloc_add_format( msgStr, ",%s,M,0.,M", stralloc_cstr(elevationStr) );
    stralloc_reset(elevationStr);

    // Bogus rest and checksum
    stralloc_add_str( msgStr, ",,,*47" );

    // Send it
    android_gps_send_nmea( stralloc_cstr(msgStr) );

    // Free it
    stralloc_reset(msgStr);
}

int
android_gps_get_location(double* outLatitude, double* outLongitude,
                         double* outMetersElevation, int* outNSatellites)
{
    // TODO: This should use 'adb shell dumpsys location' and parse the result.
    //       It must ignore parameters the caller does not want (null pointers).
    return 0;
}
