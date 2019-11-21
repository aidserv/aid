/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _OBJECT_NAME_MANAGER_H
#define _OBJECT_NAME_MANAGER_H

#include "android/base/files/Stream.h"
#include "emugl/common/mutex.h"
#include "emugl/common/smart_ptr.h"
#include "GLcommon/NamedObject.h"
#include "GLcommon/ObjectData.h"

#include <atomic>
#include <GLES/gl.h>
#include <unordered_map>
#include <unordered_set>

enum LoadShaderOrProgram {
    LOAD_SHADER,
    LOAD_PROGRAM
};

class GlobalNameSpace;
class NameSpace;

//
// class ShareGroup -
//   That class manages objects of one "local" context share group, typically
//   there will be one inctance of ShareGroup for each user OpenGL context
//   unless the user context share with another user context. In that case they
//   both will share the same ShareGroup instance.
//   calls into that class gets serialized through a lock so it is thread safe.
//
class ShareGroup
{
    friend class ObjectNameManager;
public:
    ~ShareGroup();

    //
    // genName - generates new object name and returns its name value.
    //           if genLocal is false, p_localName will be used as the name.
    //           This function also generates a "global" name for the object
    //           which can be queried using the getGlobalName function.
    ObjectLocalName genName(GenNameInfo genNameInfo, ObjectLocalName p_localName = 0, bool genLocal= false);
    //           overload for generating non-shader object
    ObjectLocalName genName(NamedObjectType namedObjectType, ObjectLocalName p_localName = 0, bool genLocal= false);
    //           overload for generating shader / program object
    ObjectLocalName genName(ShaderProgramType shaderProgramType, ObjectLocalName p_localName = 0, bool genLocal= false, GLuint existingGlobal = 0);

    //
    // getGlobalName - retrieves the "global" name of an object or 0 if the
    //                 object does not exist.
    //
    unsigned int getGlobalName(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // getLocalName - retrieves the "local" name of an object or 0 if the
    //                 object does not exist.
    //
    ObjectLocalName getLocalName(NamedObjectType p_type, unsigned int p_globalName);

    //
    // getNamedObject - returns the smart pointer of an object or null if the
    //                  object does not exist.
    NamedObjectPtr getNamedObject(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // deleteName - deletes and object from the namespace as well as its
    //              global name from the global name space.
    //
    void deleteName(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // replaceGlobalName - replaces an object to map to an existing global
    //        named object. (used when creating EGLImage siblings)
    //
    void replaceGlobalObject(NamedObjectType p_type, ObjectLocalName p_localName, NamedObjectPtr p_globalObject);

    //
    // isObject - returns true if the named object exist.
    //
    bool isObject(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // Assign object global data to a names object
    //
    void setObjectData(NamedObjectType p_type, ObjectLocalName p_localName,
            ObjectDataPtr data);

    //
    // Retrieve object global data
    //
    ObjectData* getObjectData(NamedObjectType p_type, ObjectLocalName p_localName);
    ObjectDataPtr getObjectDataPtr(NamedObjectType p_type, ObjectLocalName p_localName);
    uint64_t getId() const {return m_sharedGroupID;}
    void preSave(GlobalNameSpace *globalNameSpace);
    void onSave(android::base::Stream* stream);
    void postSave(android::base::Stream* stream);
    // postLoadRestore() restores resources on hardware GPU
    void postLoadRestore();
    bool needRestore();
private:
    explicit ShareGroup(GlobalNameSpace *globalNameSpace,
                        uint64_t sharedGroupID,
                        android::base::Stream* stream,
                        ObjectData::loadObject_t loadObject);

    void lockObjectData();
    void unlockObjectData();
    void setObjectDataLocked(NamedObjectType p_type,
            ObjectLocalName p_localName, ObjectDataPtr&& data);
    //
    // sets an object to map to an existing global object.
    //
    void setGlobalObject(NamedObjectType p_type, ObjectLocalName p_localName,
            NamedObjectPtr p_namedObject);

    // A RAII autolock class for the objectData spinlock.
    struct ObjectDataAutoLock;

private:
    const ObjectDataPtr& getObjectDataPtrNoLock(NamedObjectType p_type,
                                                ObjectLocalName p_localName);

    emugl::Mutex m_namespaceLock;
    emugl::Mutex m_restoreLock;
    NameSpace* m_nameSpace[static_cast<int>(NamedObjectType::NUM_OBJECT_TYPES)];

    // |m_objectsData| has no measured data races, so replace heavyweight mutex
    // with a simple spinlock - just in case if there's some missed
    // multi-threaded access path.
    // TODO(zyy@): Create a common spinlock class.
    std::atomic_flag m_objectsDataLock = ATOMIC_FLAG_INIT;
    // The ID of this shared group
    // It is unique within its ObjectNameManager
    uint64_t m_sharedGroupID;
    enum {PreSaved, Saved, Empty} m_saveStage = Empty;
    bool m_needLoadRestore = false;
};

typedef emugl::SmartPtr<ShareGroup> ShareGroupPtr;
// TODO: refactor ShareGroupsMap so that it is indexed by share group id
typedef std::unordered_map<void*, ShareGroupPtr> ShareGroupsMap;

//
// ObjectNameManager -
//   This class manages the set of all ShareGroups instances,
//   each ShareGroup instance can be accessed through one or more 'groupName'
//   values. the type of 'groupName' is void *, the intent is that the EGL
//   layer will use the user context handle as the name for its ShareGroup
//   object. Multiple names can be attached to a ShareGroup object to support
//   user context sharing.
//
class ObjectNameManager
{
public:
    explicit ObjectNameManager(GlobalNameSpace *globalNameSpace);

    //
    // createShareGroup - create a new ShareGroup object and attach it with
    //                    the "name" specified by p_groupName.
    //      sharedGroupID : the ID of the shared group. While loading from a
    //                      snapshot, please use the ID from the snapshot file.
    //                      In all other situations, use 0 to auto-generate a
    //                      new ID.

    ShareGroupPtr createShareGroup(void *p_groupName, uint64_t sharedGroupID,
        android::base::Stream* stream, ObjectData::loadObject_t loadObject);

    //
    // attachShareGroup - find the ShareGroup object attached to the ID
    //    specified in p_existingGroupID and attach p_groupName to the same
    //    ShareGroup instance.
    //
    ShareGroupPtr attachShareGroup(void *p_groupName, void *p_existingGroupName);
    ShareGroupPtr attachOrCreateShareGroup(void *p_groupName,
        uint64_t p_existingGroupID, android::base::Stream* stream,
        ObjectData::loadObject_t loadObject);

    //
    // getShareGroup - retreive a ShareGroup object based on its "name"
    //
    ShareGroupPtr getShareGroup(void *p_groupName);

    //
    // deleteShareGroup - deletes the attachment of the p_groupName to its
    //           attached ShareGroup. When the last name of ShareGroup is
    //           deleted the ShareGroup object is destroyed.
    //
    void deleteShareGroup(void *p_groupName);

    //
    //  getGlobalContext() - this function returns a name of an existing
    //                       ShareGroup. The intent is that the EGL layer will
    //                       use that function to get the GL context which each
    //                       new context needs to share with.
    //
    void *getGlobalContext();
    void preSave();
private:
    // TODO: refactor share group map so that it is indexed by share group ID
    ShareGroupsMap m_groups;
    emugl::Mutex m_lock;
    GlobalNameSpace *m_globalNameSpace = nullptr;
    // m_usedSharedGroupIDs is used to assign new IDs to new shared groups
    std::unordered_multiset<uint64_t> m_usedSharedGroupIDs;
    uint64_t m_nextSharedGroupID = 1;
};

#endif
