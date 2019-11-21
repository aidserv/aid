/*
* Copyright (C) 2011 The Android Open Source Project
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
#include "GLClientState.h"
#include "ErrorLog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glUtils.h"
#include <cutils/log.h>

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

GLClientState::GLClientState(int nLocations)
{
    if (nLocations < LAST_LOCATION) {
        nLocations = LAST_LOCATION;
    }
    m_nLocations = nLocations;
    m_states = new VertexAttribState[m_nLocations];
    for (int i = 0; i < m_nLocations; i++) {
        m_states[i].enabled = 0;
        m_states[i].enableDirty = false;
        m_states[i].data = 0;
    }
    m_currentArrayVbo = 0;
    m_currentIndexVbo = 0;
    // init gl constans;
    m_states[VERTEX_LOCATION].glConst = GL_VERTEX_ARRAY;
    m_states[NORMAL_LOCATION].glConst = GL_NORMAL_ARRAY;
    m_states[COLOR_LOCATION].glConst = GL_COLOR_ARRAY;
    m_states[POINTSIZE_LOCATION].glConst = GL_POINT_SIZE_ARRAY_OES;
    m_states[TEXCOORD0_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD1_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD2_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD3_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD4_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD5_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD6_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[TEXCOORD7_LOCATION].glConst = GL_TEXTURE_COORD_ARRAY;
    m_states[MATRIXINDEX_LOCATION].glConst = GL_MATRIX_INDEX_ARRAY_OES;
    m_states[WEIGHT_LOCATION].glConst = GL_WEIGHT_ARRAY_OES;
    m_activeTexture = 0;
    m_currentProgram = 0;

    m_pixelStore.unpack_alignment = 4;
    m_pixelStore.pack_alignment = 4;

    memset(m_tex.unit, 0, sizeof(m_tex.unit));
    m_tex.activeUnit = &m_tex.unit[0];
    m_tex.textures = NULL;
    m_tex.numTextures = 0;
    m_tex.allocTextures = 0;

    mRboState.boundRenderbuffer = 0;
    mRboState.boundRenderbufferIndex = 0;
    addFreshRenderbuffer(0);

    mFboState.boundFramebuffer = 0;
    mFboState.boundFramebufferIndex = 0;
    mFboState.fboCheckStatus = GL_NONE;
    addFreshFramebuffer(0);

    m_maxVertexAttribsDirty = true;
}

GLClientState::~GLClientState()
{
    delete m_states;
}

void GLClientState::enable(int location, int state)
{
    if (!validLocation(location)) {
        return;
    }

    m_states[location].enableDirty |= (state != m_states[location].enabled);
    m_states[location].enabled = state;
}

void GLClientState::setState(int location, int size, GLenum type, GLboolean normalized, GLsizei stride, const void *data)
{
    if (!validLocation(location)) {
        return;
    }
    m_states[location].size = size;
    m_states[location].type = type;
    m_states[location].stride = stride;
    m_states[location].data = (void*)data;
    m_states[location].bufferObject = m_currentArrayVbo;
    m_states[location].elementSize = size ? (glSizeof(type) * size) : 0;
    m_states[location].normalized = normalized;
}

void GLClientState::setBufferObject(int location, GLuint id)
{
    if (!validLocation(location)) {
        return;
    }

    m_states[location].bufferObject = id;
}

const GLClientState::VertexAttribState * GLClientState::getState(int location)
{
    if (!validLocation(location)) {
        return NULL;
    }
    return & m_states[location];
}

const GLClientState::VertexAttribState * GLClientState::getStateAndEnableDirty(int location, bool *enableChanged)
{
    if (!validLocation(location)) {
        return NULL;
    }

    if (enableChanged) {
        *enableChanged = m_states[location].enableDirty;
    }

    m_states[location].enableDirty = false;
    return & m_states[location];
}

int GLClientState::getLocation(GLenum loc)
{
    int retval;

    switch(loc) {
    case GL_VERTEX_ARRAY:
        retval = int(VERTEX_LOCATION);
        break;
    case GL_NORMAL_ARRAY:
        retval = int(NORMAL_LOCATION);
        break;
    case GL_COLOR_ARRAY:
        retval = int(COLOR_LOCATION);
        break;
    case GL_POINT_SIZE_ARRAY_OES:
        retval = int(POINTSIZE_LOCATION);
        break;
    case GL_TEXTURE_COORD_ARRAY:
        retval = int (TEXCOORD0_LOCATION + m_activeTexture);
        break;
    case GL_MATRIX_INDEX_ARRAY_OES:
        retval = int (MATRIXINDEX_LOCATION);
        break;
    case GL_WEIGHT_ARRAY_OES:
        retval = int (WEIGHT_LOCATION);
        break;
    default:
        retval = loc;
    }
    return retval;
}

void GLClientState::getClientStatePointer(GLenum pname, GLvoid** params)
{
    const GLClientState::VertexAttribState *state = NULL;
    switch (pname) {
    case GL_VERTEX_ARRAY_POINTER: {
        state = getState(GLClientState::VERTEX_LOCATION);
        break;
        }
    case GL_NORMAL_ARRAY_POINTER: {
        state = getState(GLClientState::NORMAL_LOCATION);
        break;
        }
    case GL_COLOR_ARRAY_POINTER: {
        state = getState(GLClientState::COLOR_LOCATION);
        break;
        }
    case GL_TEXTURE_COORD_ARRAY_POINTER: {
        state = getState(getActiveTexture() + GLClientState::TEXCOORD0_LOCATION);
        break;
        }
    case GL_POINT_SIZE_ARRAY_POINTER_OES: {
        state = getState(GLClientState::POINTSIZE_LOCATION);
        break;
        }
    case GL_MATRIX_INDEX_ARRAY_POINTER_OES: {
        state = getState(GLClientState::MATRIXINDEX_LOCATION);
        break;
        }
    case GL_WEIGHT_ARRAY_POINTER_OES: {
        state = getState(GLClientState::WEIGHT_LOCATION);
        break;
        }
    }
    if (state && params)
        *params = state->data;
}

int GLClientState::setPixelStore(GLenum param, GLint value)
{
    int retval = 0;
    switch(param) {
    case GL_UNPACK_ALIGNMENT:
        if (value == 1 || value == 2 || value == 4 || value == 8) {
            m_pixelStore.unpack_alignment = value;
        } else {
            retval =  GL_INVALID_VALUE;
        }
        break;
    case GL_PACK_ALIGNMENT:
        if (value == 1 || value == 2 || value == 4 || value == 8) {
            m_pixelStore.pack_alignment = value;
        } else {
            retval =  GL_INVALID_VALUE;
        }
        break;
        default:
            retval = GL_INVALID_ENUM;
    }
    return retval;
}




size_t GLClientState::pixelDataSize(GLsizei width, GLsizei height, GLenum format, GLenum type, int pack) const
{
    if (width <= 0 || height <= 0) return 0;

    int pixelsize = glUtilsPixelBitSize(format, type) >> 3;

    int alignment = pack ? m_pixelStore.pack_alignment : m_pixelStore.unpack_alignment;

    if (pixelsize == 0 ) {
        ERR("unknown pixel size: width: %d height: %d format: %d type: %d pack: %d align: %d\n",
             width, height, format, type, pack, alignment);
    }
    size_t linesize = pixelsize * width;
    size_t aligned_linesize = int(linesize / alignment) * alignment;
    if (aligned_linesize < linesize) {
        aligned_linesize += alignment;
    }
    return aligned_linesize * height;
}

GLenum GLClientState::setActiveTextureUnit(GLenum texture)
{
    GLuint unit = texture - GL_TEXTURE0;
    if (unit >= MAX_TEXTURE_UNITS) {
        return GL_INVALID_ENUM;
    }
    m_tex.activeUnit = &m_tex.unit[unit];
    return GL_NO_ERROR;
}

GLenum GLClientState::getActiveTextureUnit() const
{
    return GL_TEXTURE0 + (m_tex.activeUnit - &m_tex.unit[0]);
}

void GLClientState::enableTextureTarget(GLenum target)
{
    switch (target) {
    case GL_TEXTURE_2D:
        m_tex.activeUnit->enables |= (1u << TEXTURE_2D);
        break;
    case GL_TEXTURE_EXTERNAL_OES:
        m_tex.activeUnit->enables |= (1u << TEXTURE_EXTERNAL);
        break;
    }
}

void GLClientState::disableTextureTarget(GLenum target)
{
    switch (target) {
    case GL_TEXTURE_2D:
        m_tex.activeUnit->enables &= ~(1u << TEXTURE_2D);
        break;
    case GL_TEXTURE_EXTERNAL_OES:
        m_tex.activeUnit->enables &= ~(1u << TEXTURE_EXTERNAL);
        break;
    }
}

GLenum GLClientState::getPriorityEnabledTarget(GLenum allDisabled) const
{
    unsigned int enables = m_tex.activeUnit->enables;
    if (enables & (1u << TEXTURE_EXTERNAL)) {
        return GL_TEXTURE_EXTERNAL_OES;
    } else if (enables & (1u << TEXTURE_2D)) {
        return GL_TEXTURE_2D;
    } else {
        return allDisabled;
    }
}

int GLClientState::compareTexId(const void* pid, const void* prec)
{
    const GLuint* id = (const GLuint*)pid;
    const TextureRec* rec = (const TextureRec*)prec;
    return (GLint)(*id) - (GLint)rec->id;
}

GLenum GLClientState::bindTexture(GLenum target, GLuint texture,
        GLboolean* firstUse)
{
    GLboolean first = GL_FALSE;
    TextureRec* texrec = NULL;
    if (texture != 0) {
        if (m_tex.textures) {
            texrec = (TextureRec*)bsearch(&texture, m_tex.textures,
                    m_tex.numTextures, sizeof(TextureRec), compareTexId);
        }
        if (!texrec) {
            if (!(texrec = addTextureRec(texture, target))) {
                return GL_OUT_OF_MEMORY;
            }
            first = GL_TRUE;
        }
        if (target != texrec->target) {
            return GL_INVALID_OPERATION;
        }
    }

    switch (target) {
    case GL_TEXTURE_2D:
        m_tex.activeUnit->texture[TEXTURE_2D] = texture;
        break;
    case GL_TEXTURE_EXTERNAL_OES:
        m_tex.activeUnit->texture[TEXTURE_EXTERNAL] = texture;
        break;
    }

    if (firstUse) {
        *firstUse = first;
    }

    return GL_NO_ERROR;
}

GLClientState::TextureRec* GLClientState::addTextureRec(GLuint id,
        GLenum target)
{
    if (m_tex.numTextures == m_tex.allocTextures) {
        const GLuint MAX_TEXTURES = 0xFFFFFFFFu;

        GLuint newAlloc;
        if (MAX_TEXTURES - m_tex.allocTextures >= m_tex.allocTextures) {
            newAlloc = MAX(4, 2 * m_tex.allocTextures);
        } else {
            if (m_tex.allocTextures == MAX_TEXTURES) {
                return NULL;
            }
            newAlloc = MAX_TEXTURES;
        }

        TextureRec* newTextures = (TextureRec*)realloc(m_tex.textures,
                newAlloc * sizeof(TextureRec));
        if (!newTextures) {
            return NULL;
        }

        m_tex.textures = newTextures;
        m_tex.allocTextures = newAlloc;
    }

    TextureRec* tex = m_tex.textures + m_tex.numTextures;
    TextureRec* prev = tex - 1;
    while (tex != m_tex.textures && id < prev->id) {
        *tex-- = *prev--;
    }
    tex->id = id;
    tex->target = target;
    tex->format = -1;
    m_tex.numTextures++;

    return tex;
}

void GLClientState::setBoundTextureInternalFormat(GLenum target, GLint internalformat) {
    GLuint texture = getBoundTexture(target);
    TextureRec* texrec = NULL;
    texrec = (TextureRec*)bsearch(&texture, m_tex.textures,
                                  m_tex.numTextures,
                                  sizeof(TextureRec),
                                  compareTexId);
    if (!texrec) return;
    texrec->internalformat = internalformat;
}

void GLClientState::setBoundTextureFormat(GLenum target, GLenum format) {
    GLuint texture = getBoundTexture(target);
    TextureRec* texrec = NULL;
    texrec = (TextureRec*)bsearch(&texture, m_tex.textures,
                                  m_tex.numTextures,
                                  sizeof(TextureRec),
                                  compareTexId);
    if (!texrec) return;
    texrec->format = format;
}

void GLClientState::setBoundTextureType(GLenum target, GLenum type) {
    GLuint texture = getBoundTexture(target);
    TextureRec* texrec = NULL;
    texrec = (TextureRec*)bsearch(&texture, m_tex.textures,
                                  m_tex.numTextures,
                                  sizeof(TextureRec),
                                  compareTexId);
    if (!texrec) return;
    texrec->type = type;
}

GLuint GLClientState::getBoundTexture(GLenum target) const
{
    switch (target) {
    case GL_TEXTURE_2D:
        return m_tex.activeUnit->texture[TEXTURE_2D];
    case GL_TEXTURE_EXTERNAL_OES:
        return m_tex.activeUnit->texture[TEXTURE_EXTERNAL];
    default:
        return 0;
    }
}

// BEGIN driver workarounds-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-
// (>' ')><(' '<)(>' ')><(' '<)(>' ')><(' '<)(>' ')><(' '<)(>' ')><(' '<)(>' ')>

static bool unreliableInternalFormat(GLenum internalformat) {
    switch (internalformat) {
    case GL_LUMINANCE:
        return true;
    default:
        return false;
    }
}

void GLClientState::writeCopyTexImageState
    (GLenum target, GLint level, GLenum internalformat) {
    if (unreliableInternalFormat(internalformat)) {
        CubeMapDef entry;
        entry.id = getBoundTexture(GL_TEXTURE_2D);
        entry.target = target;
        entry.level = level;
        entry.internalformat = internalformat;
        m_cubeMapDefs.insert(entry);
    }
}

static GLenum identifyPositiveCubeMapComponent(GLenum target) {
    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    default:
        return 0;
    }
}

GLenum GLClientState::copyTexImageNeededTarget
    (GLenum target, GLint level, GLenum internalformat) {
    if (unreliableInternalFormat(internalformat)) {
        GLenum positiveComponent =
            identifyPositiveCubeMapComponent(target);
        if (positiveComponent) {
            CubeMapDef query;
            query.id = getBoundTexture(GL_TEXTURE_2D);
            query.target = positiveComponent;
            query.level = level;
            query.internalformat = internalformat;
            if (m_cubeMapDefs.find(query) ==
                m_cubeMapDefs.end()) {
                return positiveComponent;
            }
        }
    }
    return 0;
}

GLenum GLClientState::copyTexImageLuminanceCubeMapAMDWorkaround
    (GLenum target, GLint level, GLenum internalformat) {
    writeCopyTexImageState(target, level, internalformat);
    return copyTexImageNeededTarget(target, level, internalformat);
}

// (>' ')><(' '<)(>' ')><(' '<)(>' ')><(' '<)(>' ')><(' '<)(>' ')><(' '<)(>' ')>
// END driver workarounds-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-

void GLClientState::deleteTextures(GLsizei n, const GLuint* textures)
{
    // Updating the textures array could be made more efficient when deleting
    // several textures:
    // - compacting the array could be done in a single pass once the deleted
    //   textures are marked, or
    // - could swap deleted textures to the end and re-sort.
    TextureRec* texrec;
    for (const GLuint* texture = textures; texture != textures + n; texture++) {
        texrec = (TextureRec*)bsearch(texture, m_tex.textures,
                m_tex.numTextures, sizeof(TextureRec), compareTexId);
        if (texrec) {
            const TextureRec* end = m_tex.textures + m_tex.numTextures;
            memmove(texrec, texrec + 1,
                    (end - texrec - 1) * sizeof(TextureRec));
            m_tex.numTextures--;

            for (TextureUnit* unit = m_tex.unit;
                 unit != m_tex.unit + MAX_TEXTURE_UNITS;
                 unit++)
            {
                if (unit->texture[TEXTURE_2D] == *texture) {
                    unit->texture[TEXTURE_2D] = 0;
                } else if (unit->texture[TEXTURE_EXTERNAL] == *texture) {
                    unit->texture[TEXTURE_EXTERNAL] = 0;
                }
            }
        }
    }
}

// RBO//////////////////////////////////////////////////////////////////////////

void GLClientState::addFreshRenderbuffer(GLuint name) {
    mRboState.rboData.push_back(RboProps());
    RboProps& props = mRboState.rboData.back();
    props.target = GL_RENDERBUFFER;
    props.name = name;
    props.format = GL_NONE;
    props.previouslyBound = false;
}

void GLClientState::addRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    for (size_t i = 0; i < n; i++) {
        addFreshRenderbuffer(renderbuffers[i]);
    }
}

size_t GLClientState::getRboIndex(GLuint name) const {
    for (size_t i = 0; i < mRboState.rboData.size(); i++) {
        if (mRboState.rboData[i].name == name) {
            return i;
        }
    }
    return -1;
}

void GLClientState::removeRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    size_t bound_rbo_idx = getRboIndex(boundRboProps_const().name);

    std::vector<GLuint> to_remove;
    for (size_t i = 0; i < n; i++) {
        if (renderbuffers[i] != 0) { // Never remove the zero rb.
            to_remove.push_back(getRboIndex(renderbuffers[i]));
        }
    }

    for (size_t i = 0; i < to_remove.size(); i++) {
        mRboState.rboData[to_remove[i]] = mRboState.rboData.back();
        mRboState.rboData.pop_back();
    }

    // If we just deleted the currently bound rb,
    // bind the zero rb
    if (getRboIndex(boundRboProps_const().name) != bound_rbo_idx) {
        bindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}

bool GLClientState::usedRenderbufferName(GLuint name) const {
    for (size_t i = 0; i < mRboState.rboData.size(); i++) {
        if (mRboState.rboData[i].name == name) {
            return true;
        }
    }
    return false;
}

void GLClientState::setBoundRenderbufferIndex() {
    for (size_t i = 0; i < mRboState.rboData.size(); i++) {
        if (mRboState.rboData[i].name == mRboState.boundRenderbuffer) {
            mRboState.boundRenderbufferIndex = i;
            break;
        }
    }
}

RboProps& GLClientState::boundRboProps() {
    return mRboState.rboData[mRboState.boundRenderbufferIndex];
}

const RboProps& GLClientState::boundRboProps_const() const {
    return mRboState.rboData[mRboState.boundRenderbufferIndex];
}

void GLClientState::bindRenderbuffer(GLenum target, GLuint name) {
    // If unused, add it.
    if (!usedRenderbufferName(name)) {
        addFreshRenderbuffer(name);
    }
    mRboState.boundRenderbuffer = name;
    setBoundRenderbufferIndex();
    boundRboProps().target = target;
    boundRboProps().previouslyBound = true;
}

GLuint GLClientState::boundRenderbuffer() const {
    return boundRboProps_const().name;
}

void GLClientState::setBoundRenderbufferFormat(GLenum format) {
    boundRboProps().format = format;
}

// FBO//////////////////////////////////////////////////////////////////////////

// Format querying

GLenum GLClientState::queryRboFormat(GLuint rbo_name) const {
    return mRboState.rboData[getRboIndex(rbo_name)].format;
}

GLint GLClientState::queryTexInternalFormat(GLuint tex_name) const {
    TextureRec* texrec = NULL;
    texrec = (TextureRec*)bsearch(&tex_name, m_tex.textures,
                                  m_tex.numTextures, sizeof(TextureRec), compareTexId);
    if (!texrec) return -1;
    return texrec->internalformat;
}

GLenum GLClientState::queryTexFormat(GLuint tex_name) const {
    TextureRec* texrec = NULL;
    texrec = (TextureRec*)bsearch(&tex_name, m_tex.textures,
                                  m_tex.numTextures, sizeof(TextureRec), compareTexId);
    if (!texrec) return -1;
    return texrec->format;
}

GLenum GLClientState::queryTexType(GLuint tex_name) const {
    TextureRec* texrec = NULL;
    texrec = (TextureRec*)bsearch(&tex_name, m_tex.textures,
                                  m_tex.numTextures, sizeof(TextureRec), compareTexId);
    if (!texrec) return -1;
    return texrec->type;
}

void GLClientState::getBoundFramebufferFormat(
        GLenum attachment, FboFormatInfo* res_info) const {
    const FboProps& props = boundFboProps_const();

    res_info->type = FBO_ATTACHMENT_NONE;
    res_info->rb_format = GL_NONE;
    res_info->tex_internalformat = -1;
    res_info->tex_format = GL_NONE;
    res_info->tex_type = GL_NONE;

    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        if (props.colorAttachment0_hasRbo) {
            res_info->type = FBO_ATTACHMENT_RENDERBUFFER;
            res_info->rb_format = queryRboFormat(props.colorAttachment0_rbo);
        } else if (props.colorAttachment0_hasTexObj) {
            res_info->type = FBO_ATTACHMENT_TEXTURE;
            res_info->tex_internalformat = queryTexInternalFormat(props.colorAttachment0_texture);
            res_info->tex_format = queryTexFormat(props.colorAttachment0_texture);
            res_info->tex_type = queryTexType(props.colorAttachment0_texture);
        } else {
            res_info->type = FBO_ATTACHMENT_NONE;
        }
        break;
    case GL_DEPTH_ATTACHMENT:
        if (props.depthAttachment_hasRbo) {
            res_info->type = FBO_ATTACHMENT_RENDERBUFFER;
            res_info->rb_format = queryRboFormat(props.depthAttachment_rbo);
        } else if (props.depthAttachment_hasTexObj) {
            res_info->type = FBO_ATTACHMENT_TEXTURE;
            res_info->tex_internalformat = queryTexInternalFormat(props.depthAttachment_texture);
            res_info->tex_format = queryTexFormat(props.depthAttachment_texture);
            res_info->tex_type = queryTexType(props.depthAttachment_texture);
        } else {
            res_info->type = FBO_ATTACHMENT_NONE;
        }
        break;
    case GL_STENCIL_ATTACHMENT:
        if (props.stencilAttachment_hasRbo) {
            res_info->type = FBO_ATTACHMENT_RENDERBUFFER;
            res_info->rb_format = queryRboFormat(props.stencilAttachment_rbo);
        } else if (props.stencilAttachment_hasTexObj) {
            res_info->type = FBO_ATTACHMENT_TEXTURE;
            res_info->tex_internalformat = queryTexInternalFormat(props.stencilAttachment_texture);
            res_info->tex_format = queryTexFormat(props.stencilAttachment_texture);
            res_info->tex_type = queryTexType(props.stencilAttachment_texture);
        } else {
            res_info->type = FBO_ATTACHMENT_NONE;
        }
        break;
    default:
        res_info->type = FBO_ATTACHMENT_NONE;
        break;
    }
}

void GLClientState::addFreshFramebuffer(GLuint name) {
    mFboState.fboData.push_back(FboProps());
    FboProps& props = mFboState.fboData.back();
    props.target = GL_FRAMEBUFFER;
    props.name = name;
    props.previouslyBound = false;

    props.colorAttachment0_texture = 0;
    props.depthAttachment_texture = 0;
    props.stencilAttachment_texture = 0;

    props.colorAttachment0_hasTexObj = false;
    props.depthAttachment_hasTexObj = false;
    props.stencilAttachment_hasTexObj = false;

    props.colorAttachment0_rbo = 0;
    props.depthAttachment_rbo = 0;
    props.stencilAttachment_rbo = 0;

    props.colorAttachment0_hasRbo = false;
    props.depthAttachment_hasRbo = false;
    props.stencilAttachment_hasRbo = false;
}

void GLClientState::addFramebuffers(GLsizei n, GLuint* framebuffers) {
    for (size_t i = 0; i < n; i++) {
        addFreshFramebuffer(framebuffers[i]);
    }
}

size_t GLClientState::getFboIndex(GLuint name) const {
    for (size_t i = 0; i < mFboState.fboData.size(); i++) {
        if (mFboState.fboData[i].name == name) {
            return i;
        }
    }
    return -1;
}


void GLClientState::removeFramebuffers(GLsizei n, const GLuint* framebuffers) {
    size_t bound_fbo_idx = getFboIndex(boundFboProps_const().name);

    std::vector<GLuint> to_remove;
    for (size_t i = 0; i < n; i++) {
        if (framebuffers[i] != 0) { // Never remove the zero fb.
            to_remove.push_back(getFboIndex(framebuffers[i]));
        }
    }

    for (size_t i = 0; i < to_remove.size(); i++) {
        mFboState.fboData[to_remove[i]] = mFboState.fboData.back();
        mFboState.fboData.pop_back();
    }

    // If we just deleted the currently bound fb<
    // bind the zero fb
    if (getFboIndex(boundFboProps_const().name) != bound_fbo_idx) {
        bindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

bool GLClientState::usedFramebufferName(GLuint name) const {
    for (size_t i = 0; i < mFboState.fboData.size(); i++) {
        if (mFboState.fboData[i].name == name) {
            return true;
        }
    }
    return false;
}

void GLClientState::setBoundFramebufferIndex() {
    for (size_t i = 0; i < mFboState.fboData.size(); i++) {
        if (mFboState.fboData[i].name == mFboState.boundFramebuffer) {
            mFboState.boundFramebufferIndex = i;
            break;
        }
    }
}

FboProps& GLClientState::boundFboProps() {
    return mFboState.fboData[mFboState.boundFramebufferIndex];
}

const FboProps& GLClientState::boundFboProps_const() const {
    return mFboState.fboData[mFboState.boundFramebufferIndex];
}

void GLClientState::bindFramebuffer(GLenum target, GLuint name) {
    // If unused, add it.
    if (!usedFramebufferName(name)) {
        addFreshFramebuffer(name);
    }
    mFboState.boundFramebuffer = name;
    setBoundFramebufferIndex();
    boundFboProps().target = target;
    boundFboProps().previouslyBound = true;
}

void GLClientState::setCheckFramebufferStatus(GLenum status) {
    mFboState.fboCheckStatus = status;
}

GLenum GLClientState::getCheckFramebufferStatus() const {
    return mFboState.fboCheckStatus;
}

GLuint GLClientState::boundFramebuffer() const {
    return boundFboProps_const().name;
}

// Texture objects for FBOs/////////////////////////////////////////////////////

void GLClientState::attachTextureObject(GLenum attachment, GLuint texture) {
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        boundFboProps().colorAttachment0_texture = texture;
        boundFboProps().colorAttachment0_hasTexObj = true;
        break;
    case GL_DEPTH_ATTACHMENT:
        boundFboProps().depthAttachment_texture = texture;
        boundFboProps().depthAttachment_hasTexObj = true;
        break;
    case GL_STENCIL_ATTACHMENT:
        boundFboProps().stencilAttachment_texture = texture;
        boundFboProps().stencilAttachment_hasTexObj = true;
        break;
    default:
        break;
    }
}

GLuint GLClientState::getFboAttachmentTextureId(GLenum attachment) const {
    GLuint res;
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        res = boundFboProps_const().colorAttachment0_texture;
        break;
    case GL_DEPTH_ATTACHMENT:
        res = boundFboProps_const().depthAttachment_texture;
        break;
    case GL_STENCIL_ATTACHMENT:
        res = boundFboProps_const().stencilAttachment_texture;
        break;
    default:
        res = 0; // conservative validation for now
    }
    return res;
}

// RBOs for FBOs////////////////////////////////////////////////////////////////

void GLClientState::attachRbo(GLenum attachment, GLuint renderbuffer) {
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        boundFboProps().colorAttachment0_rbo = renderbuffer;
        boundFboProps().colorAttachment0_hasRbo = true;
        break;
    case GL_DEPTH_ATTACHMENT:
        boundFboProps().depthAttachment_rbo = renderbuffer;
        boundFboProps().depthAttachment_hasRbo = true;
        break;
    case GL_STENCIL_ATTACHMENT:
        boundFboProps().stencilAttachment_rbo = renderbuffer;
        boundFboProps().stencilAttachment_hasRbo = true;
        break;
    default:
        break;
    }
}

GLuint GLClientState::getFboAttachmentRboId(GLenum attachment) const {
    GLuint res;
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        res = boundFboProps_const().colorAttachment0_rbo;
        break;
    case GL_DEPTH_ATTACHMENT:
        res = boundFboProps_const().depthAttachment_rbo;
        break;
    case GL_STENCIL_ATTACHMENT:
        res = boundFboProps_const().stencilAttachment_rbo;
        break;
    default:
        res = 0; // conservative validation for now
    }
    return res;
}

bool GLClientState::attachmentHasObject(GLenum attachment) const {
    bool res;
    switch (attachment) {
    case GL_COLOR_ATTACHMENT0:
        res = (boundFboProps_const().colorAttachment0_hasTexObj) ||
              (boundFboProps_const().colorAttachment0_hasRbo);
        break;
    case GL_DEPTH_ATTACHMENT:
        res = (boundFboProps_const().depthAttachment_hasTexObj) ||
              (boundFboProps_const().depthAttachment_hasRbo);
        break;
    case GL_STENCIL_ATTACHMENT:
        res = (boundFboProps_const().stencilAttachment_hasTexObj) ||
              (boundFboProps_const().stencilAttachment_hasRbo);
        break;
    default:
        res = true; // liberal validation for now
    }
    return res;
}

void GLClientState::fromMakeCurrent() {
    FboProps& default_fb_props = mFboState.fboData[getFboIndex(0)];
    default_fb_props.colorAttachment0_hasRbo = true;
    default_fb_props.depthAttachment_hasRbo = true;
    default_fb_props.stencilAttachment_hasRbo = true;
}
