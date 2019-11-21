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
#pragma once

#include "ui_location-page.h"
#include "android/gps/GpsFix.h"
#include "android/metrics/PeriodicReporter.h"
#include <QTimer>
#include <QThread>
#include <QWidget>
#include <memory>

struct QAndroidLocationAgent;
class GeoDataLoaderThread;
class LocationPage : public QWidget
{
    Q_OBJECT

public:
    explicit LocationPage(QWidget *parent = 0);
    ~LocationPage();

    void setLocationAgent(const QAndroidLocationAgent* agent);
    bool isLoadingGeoData() const { return mNowLoadingGeoData; }
    void requestStopLoadingGeoData() { mGpsNextPopulateIndex = mGpsFixesArray.size(); }

    static void getDeviceLocation(const QAndroidLocationAgent* locAgent,
                                  double* pOutLatitude,
                                  double* pOutLongitude,
                                  double* pOutAltitude);
    static void sendLocationToDevice(const QAndroidLocationAgent* locAgent,
                                     double latitude,
                                     double longitude,
                                     double altitude);

signals:
    void locationUpdateRequired(double latitude, double longitude, double altitude);
    void populateNextGeoDataChunk();

private slots:
    void on_loc_GpxKmlButton_clicked();
    void on_loc_pathTable_cellChanged(int row, int col);
    void on_loc_playStopButton_clicked();
    void on_loc_modeSwitch_currentIndexChanged(int index);
    void on_loc_sendPointButton_clicked();
    void on_loc_longitudeInput_valueChanged(double);
    void on_loc_latitudeInput_valueChanged(double);
    void on_loc_altitudeInput_editingFinished();
    void on_loc_playbackSpeed_currentIndexChanged(int index);

    // Called when the thread that loads and parses
    // geo data from file is initiated.
    void geoDataThreadStarted();

    // Called when the geo data loding thread is finished.
    // Calls finishGeoDataLoading.
    // Do not change QString to QString& - these arguments are passed
    // between threads.
    void geoDataThreadFinished(QString file_name, bool ok, QString error);

    // Takes the loaded geodata and populates UI table chunk-by-chunk, allowing
    // UI to process other events while loading.
    void populateTableByChunks();

    // Same as above, but only works for the initial invokation of the geo data
    // loader thread (the one that occurs when the widget is first constructed)
    void startupGeoDataThreadFinished(QString file_name, bool ok, QString error);

    void updateDisplayedLocation(double lat, double lon, double alt);

    void locationPlaybackStart();
    void locationPlaybackStop();
    void timeout();

private:
    void finishGeoDataLoading(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error);

    void updateControlsAfterLoading();

    static bool validateCell(QTableWidget* table,
                             int row,
                             int col,
                             QString* outErrorMessage);

    std::unique_ptr<Ui::LocationPage> mUi;
    const QAndroidLocationAgent* mLocationAgent;
    QDoubleValidator mAltitudeValidator;
    GpsFixArray          mGpsFixesArray;
    int                  mGpsNextPopulateIndex = 0;
    GeoDataLoaderThread* mGeoDataLoader;
    QTimer mTimer;
    bool mNowPlaying = false;
    bool mNowLoadingGeoData = false;
    bool mLocationUsed = false;
    int mRowToSend;
    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;
};

class GeoDataLoaderThread : public QThread {
Q_OBJECT
public:
    // Loads geo data from a gpx or kml file specified
    // by file_name into the GpsFixArray pointed to
    // by fixes
    void loadGeoDataFromFile(const QString& file_name, GpsFixArray* fixes);

    static GeoDataLoaderThread* newInstance(
            const QObject* handler,
            const char* started_slot,
            const char* finished_slot);

signals:
    void loadingFinished(QString file_name, bool ok, QString error);

protected:
    // Reimplemented to load the file into the given fixes array.
    void run() override;

private:
    GeoDataLoaderThread() = default;
    QString mFileName;
    GpsFixArray* mFixes = nullptr;
};
