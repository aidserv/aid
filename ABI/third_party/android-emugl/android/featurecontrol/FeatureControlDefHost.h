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

// This file maintain a list of advanced features that can be switched on/off
// with feature control.
//
// The features in this file should be independent from system image builds.
// To add system image dependent features, please add them to
// FeatureControlDefGuest.h
//
// To add a new item, please add a new line in the following format:
// FEATURE_CONTROL_ITEM(YOUR_FEATURE_NAME)
// You will also need to edit android/data/advancedFeatures.ini to set its
// default value.

// This file is supposed to be included multiple times. It should not have
// #pragma once here.

FEATURE_CONTROL_ITEM(GLPipeChecksum)
FEATURE_CONTROL_ITEM(ForceANGLE)
FEATURE_CONTROL_ITEM(ForceSwiftshader)
FEATURE_CONTROL_ITEM(HYPERV)
FEATURE_CONTROL_ITEM(HVF)
FEATURE_CONTROL_ITEM(KVM)
FEATURE_CONTROL_ITEM(HAXM)
