/* Copyright (C) 2016 The Android Open Source Project
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

#include <initializer_list>
#include <string>
#include <string.h>

// This file contains a set of Tag Length Values (TLV) that are stored on a UICC
// (commonly called a SIM card). These classes are named after the specification
// and their constructors are written such that they only allow sub TLVs that
// are allowed by the spec. The spec can be found at https://globalplatform.org/
// and it's called the Secure Element Access Control specification.

namespace android {

// Base class for all data objects that consist of a tag, length and a value.
// This class deals with encoding and storing those three fields into a single
// string of data.
class TagLengthValue {
public:
    const char* c_str() const {
        return mData.c_str();
    }
    size_t size() const {
        return mData.size();
    }
protected:
    // Disallow construction and destruction of base class.
    TagLengthValue() = default;
    ~TagLengthValue() = default;

    void populateData(const char* tag,
                      std::initializer_list<const TagLengthValue*> data);

    template<class T>
    void populateData(const char* tag, std::initializer_list<const T*> data);

    template<class T>
    void populateData(const char* tag, T begin, T end);

private:
    static std::string createSizeString(size_t size);

    std::string mData;
};

// AID-REF-DO, contains an app ID identifying a particular application
// Currently not implemented as this is not used, just here for completeness
class AidRefDo : public TagLengthValue {
    static const char kTag[];
};

// PKG-REF-DO, contains a package name string identifying a particular
// application. If a package name is present the access rule only applies to the
// package matching this name
class PkgRefDo : public TagLengthValue {
    static const char kTag[];
public:
    explicit PkgRefDo(const std::string& packageName);
};

// DeviceAppID-REF-DO, contains a unique app ID or hash of a certificate used
// to sign an app.
class DeviceAppIdRefDo : public TagLengthValue {
    static const char kTag[];
public:
    // Create a DeviceAppID-REF-DO from data represented as a hexadecimal string
    explicit DeviceAppIdRefDo(const std::string& stringData);
};

// REF-DO, Reference Data Object, contains a DeviceAppID reference and
// optionally either an AidRefDo or a PkgRefDo. This uniquely identifies an
// application.
class RefDo : public TagLengthValue {
    static const char kTag[];
public:
    explicit RefDo(const DeviceAppIdRefDo& deviceAppIdRefDo);
    RefDo(const AidRefDo& aidRefDo,
          const DeviceAppIdRefDo& deviceAppIdRefDo);
    RefDo(const DeviceAppIdRefDo& deviceAppIdRefDo,
          const PkgRefDo& pkgRefDo);
};

// APDU-AR-DO, used to set access rules regarding APDU commands, can be used to
// allow or deny all APDU commands in one go or to set fine-grainer permissions
// for different APDU command types.
class ApduArDo : public TagLengthValue {
    static const char kTag[];
public:
    enum class Allow {
        Never = 0,
        Always = 1,
    };
    // Create an ApduArDo with a general access rule, allowing all or no APDUs
    explicit ApduArDo(Allow rule);

    // Create an ApduArDo with a set of specific access rules for each APDU,
    // See the Secure Elements Access Control specification from
    // globalplatform.org for details on the format.
    explicit ApduArDo(std::initializer_list<std::string> rules);
};

// NFC-AR-DO, indicates if NFC events are allowed for the device application
class NfcArDo : public TagLengthValue {
    static const char kTag[];
public:
    enum class Allow {
        Never = 0,
        Always = 1,
    };
    explicit NfcArDo(Allow rule);
};

// PERM-AR-DO, an 8 byte bitmask containing 64 seprate permissions, see
// https://source.android.com/devices/tech/config/uicc.html
class PermArDo : public TagLengthValue {
    static const char kTag[];
public:
    explicit PermArDo(const std::string& stringData);
};

// AR-DO, contains a permission, APDU and/or NFC access rules
class ArDo : public TagLengthValue {
    static const char kTag[];
public:
    explicit ArDo(const ApduArDo& apduArDo);
    explicit ArDo(const NfcArDo& nfcArDo);
    explicit ArDo(const PermArDo& permArDo);
    ArDo(const ApduArDo& apduArDo, const NfcArDo& nfcArDo);
};

// REF-AR-DO, a data object containing a reference to an access rule and the
// corresponding rule.
class RefArDo : public TagLengthValue {
    static const char kTag[];
public:
    RefArDo(const RefDo& refDo, const ArDo& arDo);
};

// ALL-REF-AR-DO, a list of all RefArDo's
class AllRefArDo : public TagLengthValue {
    static const char kTag[];
public:
    explicit AllRefArDo(std::initializer_list<RefArDo> refArDos);
};

}  // namespace android
