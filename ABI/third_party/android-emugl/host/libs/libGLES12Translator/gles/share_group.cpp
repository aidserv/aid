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
#include "gles/share_group.h"
#include "gles/macros.h"

using android::base::AutoLock;

namespace {

ObjectGlobalName GenerateGlobalName(GlesContext* c, ObjectType type) {
  ObjectGlobalName name = 0;
  switch (type) {
    case BUFFER:
      PASS_THROUGH(c, GenBuffers, 1, &name);
      break;
    case TEXTURE:
      PASS_THROUGH(c, GenTextures, 1, &name);
      break;
    case RENDERBUFFER:
      PASS_THROUGH(c, GenRenderbuffers, 1, &name);
      break;
    case FRAMEBUFFER:
      PASS_THROUGH(c, GenFramebuffers, 1, &name);
      break;
    default:
      name = 0;
      break;
  }
#ifdef ENABLE_API_LOGGING
  ALOGI("Generated name %d of type %d", name, type);
#endif
  return name;
}

void DeleteGlobalName(GlesContext* c, ObjectType type, ObjectGlobalName name) {
  if (name == 0) {
    return;
  }
  switch (type) {
    case BUFFER:
      PASS_THROUGH(c, DeleteBuffers, 1, &name);
      break;
    case TEXTURE:
      PASS_THROUGH(c, DeleteTextures, 1, &name);
      break;
    case RENDERBUFFER:
      PASS_THROUGH(c, DeleteRenderbuffers, 1, &name);
      break;
    case FRAMEBUFFER:
      PASS_THROUGH(c, DeleteFramebuffers, 1, &name);
      break;
    default:
      // Do nothing.
      break;
  }
}

}  // namespace

// This class manages the association between "local" names (ie. names for
// objects used by client code) and "global" names (ie. names for objects
// managed by the underlying context.)
class NamespaceImpl {
 public:
  explicit NamespaceImpl(ObjectType type);
  ~NamespaceImpl();

  // Generates a unique local name that is currently not in use.
  ObjectLocalName GenLocalName();

  // Returns the global name associated with the specified local name, or 0 if
  // the name is unknown.
  ObjectGlobalName GetGlobalName(ObjectLocalName local_name);

  // Associates the specified local name with the global name.
  void SetGlobalName(ObjectLocalName local_name, ObjectGlobalName global_name);

  // Removes the name from the namespace as well as deleting the global name
  // from the underlying context.
  void DeleteName(GlesContext* c, ObjectLocalName local_name);

  // Deletes all names from the namespace.
  void DeleteAllNames(GlesContext* c);

  typedef std::map<ObjectLocalName, ObjectGlobalName> NameMap;

  const ObjectType type_;
  ObjectLocalName next_name_;
  NameMap names_;
};

NamespaceImpl::NamespaceImpl(ObjectType type)
  : type_(type),
    next_name_(0) {
}

NamespaceImpl::~NamespaceImpl() {
  LOG_ALWAYS_FATAL_IF(names_.size() > 0, "DeleteAllNames before destroying.");
}

ObjectLocalName NamespaceImpl::GenLocalName() {
  ObjectLocalName local_name = 0;
  do {
    local_name = ++next_name_;
  } while (local_name == 0 || names_.find(local_name) != names_.end());
  return local_name;
}

ObjectGlobalName NamespaceImpl::GetGlobalName(ObjectLocalName local_name) {
  NameMap::iterator it = names_.find(local_name);
  return it != names_.end() ? it->second : 0;
}

void NamespaceImpl::SetGlobalName(ObjectLocalName local_name,
                                  ObjectGlobalName global_name) {
  names_[local_name] = global_name;
}

void NamespaceImpl::DeleteName(GlesContext* c, ObjectLocalName local_name) {
  NameMap::iterator it = names_.find(local_name);
  if (it != names_.end()) {
    DeleteGlobalName(c, type_, it->second);
    names_.erase(it);
  }
}

void NamespaceImpl::DeleteAllNames(GlesContext* c) {
  for (NameMap::iterator it = names_.begin(); it != names_.end(); ++it) {
    DeleteGlobalName(c, type_, it->second);
  }
  names_.clear();
}

ShareGroup::ShareGroup(GlesContext* context)
  : context_(context) {
  for (int i = 0; i < NUM_OBJECT_TYPES; ++i) {
    namespace_[i] = new NamespaceImpl((ObjectType)i);
  }
}

ShareGroup::~ShareGroup() {
    AutoLock mutex(lock_);
  objects_.clear();
  for (int i = 0; i < NUM_OBJECT_TYPES; ++i) {
    namespace_[i]->DeleteAllNames(context_);
    delete namespace_[i];
  }
}

void ShareGroup::GenNames(ObjectType type, int n, ObjectLocalName* names) {
    AutoLock mutex(lock_);
  NamespaceImpl* ns = GetNamespace(type);
  for (int i = 0; i < n; ++i) {
    names[i] = ns->GenLocalName();
    // TODO(crbug.com/441939): Generate all global names in a batch.
    const ObjectGlobalName global_name = GenerateGlobalName(context_, type);
    ns->SetGlobalName(names[i], global_name);
  }
}

ObjectGlobalName ShareGroup::GetGlobalName(ObjectType type,
                                           ObjectLocalName local_name) {
    AutoLock mutex(lock_);
  return GetNamespace(type)->GetGlobalName(local_name);
}

void ShareGroup::SetGlobalName(ObjectType type,
                               ObjectLocalName local_name,
                               ObjectGlobalName global_name) {
    AutoLock mutex(lock_);
  GetNamespace(type)->SetGlobalName(local_name, global_name);
}

ObjectDataPtr ShareGroup::GetObject(ObjectType type, ObjectLocalName name,
                                    bool create_if_needed) {
  if (name == 0) {
    return ObjectDataPtr();
  }

  AutoLock mutex(lock_);

  const ObjectID id = GetObjectID(type, name);
  ObjectDataMap::iterator iter = objects_.find(id);
  if (iter != objects_.end()) {
    return iter->second;
  }

  if (!create_if_needed) {
    return ObjectDataPtr();
  }

  // We do not have an object with this name.  In this case, we are dealing
  // with client-managed object names.  We will generate a new global name and
  // associate it with the client-managed local name. Note that some objects
  // (e.g. programs and shaders) manage their own global names, and so will
  // not be allocated here.
  if (type != FRAGMENT_SHADER && type != VERTEX_SHADER && type != PROGRAM) {
    NamespaceImpl* ns = GetNamespace(type);
    ObjectGlobalName global_name = ns->GetGlobalName(name);
    if (global_name == 0) {
      global_name = GenerateGlobalName(context_, type);
      ns->SetGlobalName(name, global_name);
    }
  }

  // Create the object data for the newly generated name.
  ObjectDataPtr obj;
  switch (type) {
    case BUFFER:
      obj = ObjectDataPtr(new BufferData(name));
      break;
    case TEXTURE:
      obj = ObjectDataPtr(new TextureData(name));
      break;
    case RENDERBUFFER:
      obj = ObjectDataPtr(new RenderbufferData(name));
      break;
    case FRAMEBUFFER:
      obj = ObjectDataPtr(new FramebufferData(name));
      break;
    case VERTEX_SHADER:
      obj = ObjectDataPtr(new ShaderData(type, name));
      break;
    case FRAGMENT_SHADER:
      obj = ObjectDataPtr(new ShaderData(type, name));
      break;
    case PROGRAM:
      obj = ObjectDataPtr(new ProgramData(name));
      break;
    default:
      LOG_ALWAYS_FATAL("Unsupported object type %d", type);
      break;
  }

  objects_.insert(std::make_pair(id, obj));
  return obj;
}

void ShareGroup::DeleteObjects(ObjectType type, int n,
                               const ObjectLocalName* names) {
    AutoLock mutex(lock_);
  NamespaceImpl* ns = GetNamespace(type);
  for (int i = 0; i < n; ++i) {
    if (names[i] != 0) {
      // TODO(crbug.com/441939): Delete the underlying global names in a batch.
      ns->DeleteName(context_, names[i]);
      objects_.erase(GetObjectID(type, names[i]));
    }
  }
}

NamespaceImpl* ShareGroup::GetNamespace(ObjectType type) {
  return namespace_[ValidateType(type)];
}

ShareGroup::ObjectID ShareGroup::GetObjectID(ObjectType type,
                                             ObjectLocalName name) const {
  return ObjectID(ValidateType(type), name);
}

ObjectType ShareGroup::ValidateType(ObjectType type) const {
  LOG_ALWAYS_FATAL_IF(type < 0 || type >= NUM_OBJECT_TYPES);
  // To simplify lookup, both vertex shader and fragment shaders are given the
  // common SHADER type.
  if (type == FRAGMENT_SHADER || type == VERTEX_SHADER) {
    return SHADER;
  } else {
    return type;
  }
}
