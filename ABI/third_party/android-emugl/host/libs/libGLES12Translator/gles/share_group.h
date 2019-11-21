/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GRAPHICS_TRANSLATION_GLES_SHARE_GROUP_H_
#define GRAPHICS_TRANSLATION_GLES_SHARE_GROUP_H_

#include "gles/buffer_data.h"
#include "gles/framebuffer_data.h"
#include "gles/program_data.h"
#include "gles/renderbuffer_data.h"
#include "gles/shader_data.h"
#include "gles/texture_data.h"

#include "android/base/synchronization/Lock.h"

#include <map>
#include <utility>
#include <utils/RefBase.h>

class GlesContext;
class NamespaceImpl;

// The ShareGroup manages the names and objects associated with a GLES context.
// Instances of this class can be shared between multiple contexts.
// (Specifically, when a context is created, a shared context can also be
// set, in which case both contexts will "share" this share group.)  All
// operations on this class are serialized through a lock so it is thread safe.
// Though most of the functions (ex. GenName) can operate on any object type,
// only a specific subset of the functionality is made public by explicitly
// providing functions for a given type (ex. GenBufferName).  This is to
// allow us to catch problematic usage at compile time rather than asserting
// at runtime.
class ShareGroup : public android::RefBase {
 public:
  explicit ShareGroup(GlesContext* context);

  // Generates a new object global and local name and returns its local name
  // value.
  void GenBuffers(int n, ObjectLocalName* names) {
    return GenNames(BUFFER, n, names);
  }
  void GenFramebuffers(int n, ObjectLocalName* names) {
    return GenNames(FRAMEBUFFER, n, names);
  }
  void GenRenderbuffers(int n, ObjectLocalName* names) {
    return GenNames(RENDERBUFFER, n, names);
  }
  void GenTextures(int n, ObjectLocalName* names) {
    return GenNames(TEXTURE, n, names);
  }
  void GenPrograms(int n, ObjectLocalName* names) {
    return GenNames(PROGRAM, n, names);
  }
  void GenVertexShaders(int n, ObjectLocalName* names) {
    return GenNames(VERTEX_SHADER, n, names);
  }
  void GenFragmentShaders(int n, ObjectLocalName* names) {
    return GenNames(FRAGMENT_SHADER, n, names);
  }

  // Create an object of the specified type with the given local name.
  BufferDataPtr CreateBufferData(ObjectLocalName name) {
    return GetObjectAs<BufferData>(BUFFER, name, true);
  }
  FramebufferDataPtr CreateFramebufferData(ObjectLocalName name) {
    return GetObjectAs<FramebufferData>(FRAMEBUFFER, name, true);
  }
  RenderbufferDataPtr CreateRenderbufferData(ObjectLocalName name) {
    return GetObjectAs<RenderbufferData>(RENDERBUFFER, name, true);
  }
  TextureDataPtr CreateTextureData(ObjectLocalName name) {
    return GetObjectAs<TextureData>(TEXTURE, name, true);
  }
  ProgramDataPtr CreateProgramData(ObjectLocalName name) {
    return GetObjectAs<ProgramData>(PROGRAM, name, true);
  }
  ShaderDataPtr CreateVertexShaderData(ObjectLocalName name) {
    return GetObjectAs<ShaderData>(VERTEX_SHADER, name, true);
  }
  ShaderDataPtr CreateFragmentShaderData(ObjectLocalName name) {
    return GetObjectAs<ShaderData>(FRAGMENT_SHADER, name, true);
  }

  // Retrieve the object of the specified type with the given local name.
  BufferDataPtr GetBufferData(ObjectLocalName name) {
    return GetObjectAs<BufferData>(BUFFER, name, false);
  }
  FramebufferDataPtr GetFramebufferData(ObjectLocalName name) {
    return GetObjectAs<FramebufferData>(FRAMEBUFFER, name, false);
  }
  RenderbufferDataPtr GetRenderbufferData(ObjectLocalName name) {
    return GetObjectAs<RenderbufferData>(RENDERBUFFER, name, false);
  }
  TextureDataPtr GetTextureData(ObjectLocalName name) {
    return GetObjectAs<TextureData>(TEXTURE, name, false);
  }
  ProgramDataPtr GetProgramData(ObjectLocalName name) {
    return GetObjectAs<ProgramData>(PROGRAM, name, false);
  }
  ShaderDataPtr GetShaderData(ObjectLocalName name) {
    return GetObjectAs<ShaderData>(SHADER, name, false);
  }

  // Deletes the object of the specified type as well as unregistering its
  // names from the ShareGroup.
  void DeleteBuffers(int n, const ObjectLocalName* names) {
    DeleteObjects(BUFFER, n, names);
  }
  void DeleteFramebuffers(int n, const ObjectLocalName* names) {
    DeleteObjects(FRAMEBUFFER, n, names);
  }
  void DeleteRenderbuffers(int n, const ObjectLocalName* names) {
    DeleteObjects(RENDERBUFFER, n, names);
  }
  void DeleteTextures(int n, const ObjectLocalName* names) {
    DeleteObjects(TEXTURE, n, names);
  }
  void DeletePrograms(int n, const ObjectLocalName* names) {
    // TODO(crbug.com/424353): Keep program name active until the program
    // is actually unused, even if it was marked as deleted.
    DeleteObjects(PROGRAM, n, names);
  }
  void DeleteShaders(int n, const ObjectLocalName* names) {
    DeleteObjects(SHADER, n, names);
  }

  // Retrieves the "global" name of an object or 0 if the object does not exist.
  ObjectGlobalName GetBufferGlobalName(ObjectLocalName local_name) {
    return GetGlobalName(BUFFER, local_name);
  }
  ObjectGlobalName GetFramebufferGlobalName(ObjectLocalName local_name) {
    return GetGlobalName(FRAMEBUFFER, local_name);
  }
  ObjectGlobalName GetRenderbufferGlobalName(ObjectLocalName local_name) {
    return GetGlobalName(RENDERBUFFER, local_name);
  }
  ObjectGlobalName GetTextureGlobalName(ObjectLocalName local_name) {
    return GetGlobalName(TEXTURE, local_name);
  }

  // Maps an object to the specified global named object.  (Note: useful when
  // creating EGLImage siblings).
  void SetTextureGlobalName(ObjectLocalName local_name,
                            ObjectGlobalName global_name) {
    SetGlobalName(TEXTURE, local_name, global_name);
  }

 protected:
  virtual ~ShareGroup();

 private:
  typedef std::pair<ObjectType, ObjectLocalName> ObjectID;
  typedef std::map<ObjectID, ObjectDataPtr> ObjectDataMap;

  ObjectDataPtr GetObject(ObjectType type, ObjectLocalName name,
                          bool create_if_needed);
  template <typename T>
  android::sp<T> GetObjectAs(ObjectType type, ObjectLocalName name,
                             bool create_if_needed) {
    return static_cast<T*>(GetObject(type, name, create_if_needed).get());
  }
  void DeleteObjects(ObjectType type, int n, const ObjectLocalName* names);

  void GenNames(ObjectType type, int n, ObjectLocalName* names);
  ObjectGlobalName GetGlobalName(ObjectType type, ObjectLocalName local_name);
  void SetGlobalName(ObjectType type, ObjectLocalName local_name,
                     ObjectGlobalName global_name);

  ObjectType ValidateType(ObjectType type) const;
  ObjectID GetObjectID(ObjectType type, ObjectLocalName name) const;
  NamespaceImpl* GetNamespace(ObjectType type);

  android::base::Lock lock_;
  NamespaceImpl* namespace_[NUM_OBJECT_TYPES];
  ObjectDataMap objects_;
  GlesContext* context_;

  ShareGroup(const ShareGroup&);
  ShareGroup& operator=(const ShareGroup&);
};

typedef android::sp<ShareGroup> ShareGroupPtr;

#endif  // GRAPHICS_TRANSLATION_GLES_SHARE_GROUP_H_
