/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License")
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

#ifdef _WIN32
#undef  GL_APICALL
#define GL_API __declspec(dllexport)
#define GL_APICALL __declspec(dllexport)
#endif

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#include <OpenglCodecCommon/ErrorLog.h>
#include "GLcommon/SaveableTexture.h"
#include "GLcommon/TextureData.h"
#include "GLcommon/TranslatorIfaces.h"
#include "GLESv2Context.h"
#include "GLESv2Validate.h"
#include "SamplerData.h"
#include "ShaderParser.h"
#include "ProgramData.h"
#include "GLcommon/TextureUtils.h"
#include "GLcommon/FramebufferData.h"

#include "ANGLEShaderParser.h"

#include <stdio.h>

#include <numeric>
#include <unordered_map>

extern "C" {

//decleration
static void initGLESx();
static void initContext(GLEScontext* ctx,ShareGroupPtr grp);
static void deleteGLESContext(GLEScontext* ctx);
static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp);
static GLEScontext* createGLESContext(void);
static GLEScontext* createGLESxContext(int maj, int min, GlobalNameSpace* globalNameSpace, android::base::Stream* stream);
static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName);
static void saveTexture(SaveableTexture* texture, android::base::Stream* stream);
static SaveableTexture* loadTexture(android::base::Stream* stream, GlobalNameSpace* globalNameSpace);
}

/************************************** GLES EXTENSIONS *********************************************************/
typedef std::unordered_map<std::string, __translatorMustCastToProperFunctionPointerType> ProcTableMap;
ProcTableMap *s_glesExtensions = NULL;
/****************************************************************************************************************/

static EGLiface*  s_eglIface = NULL;
static GLESiface  s_glesIface = {
    .initGLESx                  = initGLESx,
    .createGLESContext          = createGLESxContext,
    .initContext                = initContext,
    .deleteGLESContext          = deleteGLESContext,
    .flush                      = (FUNCPTR_NO_ARGS_RET_VOID)glFlush,
    .finish                     = (FUNCPTR_NO_ARGS_RET_VOID)glFinish,
    .getError                   = (FUNCPTR_NO_ARGS_RET_INT)glGetError,
    .setShareGroup              = setShareGroup,
    .getProcAddress             = getProcAddress,
    .fenceSync                  = (FUNCPTR_FENCE_SYNC)glFenceSync,
    .clientWaitSync             = (FUNCPTR_CLIENT_WAIT_SYNC)glClientWaitSync,
    .deleteSync                 = (FUNCPTR_DELETE_SYNC)glDeleteSync,
    .saveTexture                = saveTexture,
    .loadTexture                = loadTexture,
    .deleteRbo                  = deleteRenderbufferGlobal,
};

#include <GLcommon/GLESmacros.h>

extern "C" {


static void initContext(GLEScontext* ctx,ShareGroupPtr grp) {
    if (!ctx->shareGroup()) {
        ctx->setShareGroup(grp);
    }
    if (!ctx->isInitialized()) {
        ctx->init(s_eglIface->eglGetGlLibrary());
        glBindTexture(GL_TEXTURE_2D,0);
        glBindTexture(GL_TEXTURE_CUBE_MAP,0);
    }
    if (ctx->needRestore()) {
        ctx->restore();
    }
}

static GLEScontext* createGLESContext() {
    return new GLESv2Context(2, 0, nullptr, nullptr, nullptr);
}
static GLEScontext* createGLESxContext(int maj, int min,
        GlobalNameSpace* globalNameSpace, android::base::Stream* stream) {
    return new GLESv2Context(maj, min, globalNameSpace, stream,
            s_eglIface->eglGetGlLibrary());
}

static bool shaderParserInitialized = false;

static void initGLESx() { }

static void deleteGLESContext(GLEScontext* ctx) {
    delete ctx;
}

static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp) {
    if(ctx) {
        ctx->setShareGroup(grp);
    }
}

GL_APICALL void  GL_APIENTRY glVertexAttribPointerWithDataSize(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr, GLsizei dataSize);

static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName) {
    GET_CTX_RET(NULL)
    ctx->getGlobalLock();
    static bool proc_table_initialized = false;
    if (!proc_table_initialized) {
        proc_table_initialized = true;
        if (!s_glesExtensions)
            s_glesExtensions = new ProcTableMap();
        else
            s_glesExtensions->clear();
        (*s_glesExtensions)["glEGLImageTargetTexture2DOES"] = (__translatorMustCastToProperFunctionPointerType)glEGLImageTargetTexture2DOES;
        (*s_glesExtensions)["glEGLImageTargetRenderbufferStorageOES"]=(__translatorMustCastToProperFunctionPointerType)glEGLImageTargetRenderbufferStorageOES;
        (*s_glesExtensions)["glVertexAttribPointerWithDataSize"] = (__translatorMustCastToProperFunctionPointerType)glVertexAttribPointerWithDataSize;
    }
    __translatorMustCastToProperFunctionPointerType ret=NULL;
    ProcTableMap::iterator val = s_glesExtensions->find(procName);
    if (val!=s_glesExtensions->end())
        ret = val->second;
    ctx->releaseGlobalLock();

    return ret;
}

static void saveTexture(SaveableTexture* texture, android::base::Stream* stream) {
    texture->onSave(stream);
}

static SaveableTexture* loadTexture(android::base::Stream* stream,
        GlobalNameSpace* globalNameSpace) {
    return new SaveableTexture(stream, globalNameSpace);
}

GL_APICALL GLESiface* GL_APIENTRY __translator_getIfaces(EGLiface* eglIface);

GLESiface* __translator_getIfaces(EGLiface* eglIface) {
    s_eglIface = eglIface;
    return & s_glesIface;
}

}  // extern "C"

static void s_attachShader(GLEScontext* ctx, GLuint program, GLuint shader,
                           ShaderParser* shaderParser) {
    if (ctx && program && shader && shaderParser) {
        shaderParser->attachProgram(program);
    }
}

static void s_detachShader(GLEScontext* ctx, GLuint program, GLuint shader) {
    if (ctx && shader && ctx->shareGroup().get()) {
        auto shaderData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        if (!shaderData) return;
        ShaderParser* shaderParser = (ShaderParser*)shaderData;
        shaderParser->detachProgram(program);
        if (shaderParser->getDeleteStatus()
                && !shaderParser->hasAttachedPrograms()) {
            ctx->shareGroup()->deleteName(NamedObjectType::SHADER_OR_PROGRAM, shader);
        }
    }
}

static TextureData* getTextureData(ObjectLocalName tex) {
    GET_CTX_RET(NULL);
    TextureData *texData = NULL;
    auto objData =
            ctx->shareGroup()->getObjectData(NamedObjectType::TEXTURE, tex);
    if(!objData){
        texData = new TextureData();
        ctx->shareGroup()->setObjectData(NamedObjectType::TEXTURE, tex,
                                         ObjectDataPtr(texData));
    } else {
        texData = (TextureData*)objData;
    }
    return texData;
}

static TextureData* getTextureTargetData(GLenum target){
    GET_CTX_RET(NULL);
    unsigned int tex = ctx->getBindedTexture(target);
    return getTextureData(ctx->getTextureLocalName(target,tex));
}

GL_APICALL void  GL_APIENTRY glActiveTexture(GLenum texture){
    GET_CTX_V2();
    SET_ERROR_IF (!GLESv2Validate::textureEnum(texture,ctx->getMaxCombinedTexUnits()),GL_INVALID_ENUM);
    ctx->setActiveTexture(texture);
    ctx->dispatcher().glActiveTexture(texture);
}

GL_APICALL void  GL_APIENTRY glAttachShader(GLuint program, GLuint shader){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName==0, GL_INVALID_VALUE);

        auto programData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        auto shaderData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!shaderData || !programData ,GL_INVALID_OPERATION);
        SET_ERROR_IF(!(shaderData->getDataType() ==SHADER_DATA) ||
                     !(programData->getDataType()==PROGRAM_DATA) ,GL_INVALID_OPERATION);

        GLenum shaderType = ((ShaderParser*)shaderData)->getType();
        ProgramData* pData = (ProgramData*)programData;
        SET_ERROR_IF((pData->getAttachedShader(shaderType)!=0), GL_INVALID_OPERATION);
        pData->attachShader(shader, (ShaderParser*)shaderData, shaderType);
        s_attachShader(ctx, program, shader, (ShaderParser*)shaderData);
        ctx->dispatcher().glAttachShader(globalProgramName,globalShaderName);
    }
}

GL_APICALL void  GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::attribName(name),GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLESv2Validate::attribIndex(index, ctx->getCaps()->maxVertexAttribs),GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);

        ctx->dispatcher().glBindAttribLocation(globalProgramName,index,name);

        ProgramData* pData = (ProgramData*)objData;
        pData->bindAttribLocation(name, index);
    }
}

GL_APICALL void  GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target), GL_INVALID_ENUM);
    //if buffer wasn't generated before,generate one
    if (buffer && ctx->shareGroup().get() &&
        !ctx->shareGroup()->isObject(NamedObjectType::VERTEXBUFFER, buffer)) {
        ctx->shareGroup()->genName(NamedObjectType::VERTEXBUFFER, buffer);
        ctx->shareGroup()->setObjectData(NamedObjectType::VERTEXBUFFER, buffer,
                                         ObjectDataPtr(new GLESbuffer()));
    }
    ctx->bindBuffer(target,buffer);
    if (buffer) {
        GLESbuffer* vbo =
                (GLESbuffer*)ctx->shareGroup()
                        ->getObjectData(NamedObjectType::VERTEXBUFFER, buffer);
        vbo->setBinded();
        const GLuint globalBufferName = ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer);
        ctx->dispatcher().glBindBuffer(target, globalBufferName);
    } else {
        ctx->dispatcher().glBindBuffer(target, 0);
    }
}

static bool sIsFboTextureTarget(GLenum target) {
    switch (target) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_2D_MULTISAMPLE:
        return true;
    default:
        return false;
    }
}

template <class T>
static bool sHasAttachmentWithFormat(const GLESv2Context* ctx,
                                     FramebufferData* fbData,
                                     const T& attachments,
                                     const std::initializer_list<GLenum> formats) {

    for (auto attachment : attachments) {
        GLenum target;
        GLuint name = fbData->getAttachment(attachment, &target, NULL);
        if (target == GL_RENDERBUFFER) {
            auto objData = ctx->shareGroup()->getObjectData(
                    NamedObjectType::RENDERBUFFER, name);
            if (auto rbData = (RenderbufferData*)objData) {
                GLenum rb_internalformat = rbData->internalformat;
                for (auto triggerFormat : formats) {
                    if (rb_internalformat == triggerFormat) {
                        return true;
                    }
                }
            }
        } else if (sIsFboTextureTarget(target)) {
            if (TextureData* tex = getTextureData(name)) {
                GLenum tex_internalformat = tex->internalFormat;
                for (auto triggerFormat : formats) {
                    if (tex_internalformat == triggerFormat) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


static void sSetDesktopGLEnable(const GLESv2Context* ctx, bool enable, GLenum cap) {
    if (enable)
        ctx->dispatcher().glEnable(cap);
    else
        ctx->dispatcher().glDisable(cap);
}

// Framebuffer format workarounds:
// Desktop OpenGL implicit framebuffer behavior is much more configurable
// than that of OpenGL ES. In OpenGL ES, some implicit operations can happen
// depending on the internal format and attachment combinations of the
// framebuffer object.
static void sUpdateFboEmulation(GLESv2Context* ctx) {
    if (ctx->getMajorVersion() < 3) return;

    std::vector<GLenum> colorAttachments(ctx->getCaps()->maxDrawBuffers);
    std::iota(colorAttachments.begin(), colorAttachments.end(), GL_COLOR_ATTACHMENT0);
    const auto depthAttachments =
        {GL_DEPTH_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT};

    GLuint read_fbo = ctx->getFramebufferBinding(GL_READ_FRAMEBUFFER);
    GLuint draw_fbo = ctx->getFramebufferBinding(GL_DRAW_FRAMEBUFFER);

    bool enableSRGB = false;
    bool enableDepth32fClamp = false;

    for (auto fbObj : {ctx->getFBOData(read_fbo),
                       ctx->getFBOData(draw_fbo)}) {

        if (fbObj == nullptr) { continue; }

        // Enable GL_FRAMEBUFFER_SRGB when any framebuffer has SRGB color attachment.
        if (sHasAttachmentWithFormat(ctx, fbObj,
                    colorAttachments, {GL_SRGB8_ALPHA8}))
            enableSRGB = true;

        // Enable GL_DEPTH_CLAMP when any fbo's
        // GL_DEPTH_ATTACHMENT or GL_DEPTH_STENCIL_ATTACHMENT is of internal format
        // GL_DEPTH_COMPONENT32F or GL_DEPTH32F_STENCIL8.
        if (sHasAttachmentWithFormat(ctx, fbObj,
                    depthAttachments, {GL_DEPTH_COMPONENT32F, GL_DEPTH32F_STENCIL8}))
            enableDepth32fClamp = true;
    }

    // TODO: GLES3: snapshot those enable value as well?
    sSetDesktopGLEnable(ctx, enableSRGB, GL_FRAMEBUFFER_SRGB);
    sSetDesktopGLEnable(ctx, enableDepth32fClamp, GL_DEPTH_CLAMP);
}

GL_APICALL void  GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::framebufferTarget(ctx, target),GL_INVALID_ENUM);

    GLuint globalFrameBufferName;
    bool isDefaultFBO = !framebuffer;
    if (isDefaultFBO) {
       globalFrameBufferName = ctx->getDefaultFBOGlobalName();
       ctx->dispatcher().glBindFramebufferEXT(target, globalFrameBufferName);
       ctx->setFramebufferBinding(target, 0);
    } else {
        globalFrameBufferName = framebuffer;
        if(framebuffer){
            globalFrameBufferName = ctx->getFBOGlobalName(framebuffer);
            //if framebuffer wasn't generated before,generate one
            if(!globalFrameBufferName){
                ctx->genFBOName(framebuffer);
                ctx->setFBOData(framebuffer,
                        ObjectDataPtr(new FramebufferData(framebuffer)));
                globalFrameBufferName = ctx->getFBOGlobalName(framebuffer);
            }
            // set that this framebuffer has been bound before
            auto fbObj = ctx->getFBOData(framebuffer);
            fbObj->setBoundAtLeastOnce();
        }
        ctx->dispatcher().glBindFramebufferEXT(target,globalFrameBufferName);
        ctx->setFramebufferBinding(target, framebuffer);
    }

    sUpdateFboEmulation(ctx);
}

GL_APICALL void  GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::renderbufferTarget(target),GL_INVALID_ENUM);

    GLuint globalRenderBufferName = renderbuffer;
    if(renderbuffer && ctx->shareGroup().get()){
        globalRenderBufferName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::RENDERBUFFER, renderbuffer);
        //if renderbuffer wasn't generated before,generate one
        if(!globalRenderBufferName){
            ctx->shareGroup()->genName(NamedObjectType::RENDERBUFFER,
                                       renderbuffer);
            ctx->shareGroup()->setObjectData(
                    NamedObjectType::RENDERBUFFER, renderbuffer,
                    ObjectDataPtr(new RenderbufferData()));
            globalRenderBufferName = ctx->shareGroup()->getGlobalName(
                    NamedObjectType::RENDERBUFFER, renderbuffer);
        }
    }
    ctx->dispatcher().glBindRenderbufferEXT(target,globalRenderBufferName);

    // update renderbuffer binding state
    ctx->setRenderbufferBinding(renderbuffer);
}

GL_APICALL void  GL_APIENTRY glBindTexture(GLenum target, GLuint texture){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::textureTarget(ctx, target), GL_INVALID_ENUM);

    //for handling default texture (0)
    ObjectLocalName localTexName = ctx->getTextureLocalName(target,texture);
    GLuint globalTextureName = localTexName;
    if (ctx->shareGroup().get()) {
        globalTextureName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::TEXTURE, localTexName);
        //if texture wasn't generated before,generate one
        if(!globalTextureName){
            ctx->shareGroup()->genName(NamedObjectType::TEXTURE, localTexName);
            globalTextureName = ctx->shareGroup()->getGlobalName(
                    NamedObjectType::TEXTURE, localTexName);
        }

        TextureData* texData = getTextureData(localTexName);
        if (texData->target==0)
            texData->target = target;
        //if texture was already bound to another target

        if (ctx->GLTextureTargetToLocal(texData->target) != ctx->GLTextureTargetToLocal(target)) {
            fprintf(stderr, "%s: Set invalid operation!\n", __func__);
        }
        SET_ERROR_IF(ctx->GLTextureTargetToLocal(texData->target) != ctx->GLTextureTargetToLocal(target), GL_INVALID_OPERATION);
        texData->wasBound = true;
        texData->globalName = globalTextureName;
    }

    ctx->setBindedTexture(target,texture);
    ctx->dispatcher().glBindTexture(target,globalTextureName);

    if (ctx->getMajorVersion() < 3) return;

    // OpenGL ES assumes that rendered depth textures shade as (v, 0, 0, 1)
    // when coming out of the fragment shader.
    // Desktop OpenGL assumes (v, v, v, 1).
    // GL_DEPTH_TEXTURE_MODE can be set to GL_RED to follow the OpenGL ES behavior.
#define GL_DEPTH_TEXTURE_MODE 0x884B
    ctx->dispatcher().glTexParameteri(target,GL_DEPTH_TEXTURE_MODE,GL_RED);
}

GL_APICALL void  GL_APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){
    GET_CTX();
    ctx->dispatcher().glBlendColor(red,green,blue,alpha);
}

GL_APICALL void  GL_APIENTRY glBlendEquation( GLenum mode ){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::blendEquationMode(ctx, mode), GL_INVALID_ENUM);
    ctx->dispatcher().glBlendEquation(mode);
}

GL_APICALL void  GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::blendEquationMode(ctx, modeRGB) &&
                   GLESv2Validate::blendEquationMode(ctx, modeAlpha)), GL_INVALID_ENUM);
    ctx->dispatcher().glBlendEquationSeparate(modeRGB,modeAlpha);
}

GL_APICALL void  GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::blendSrc(sfactor) || !GLESv2Validate::blendDst(dfactor),GL_INVALID_ENUM)
    ctx->setBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
    ctx->dispatcher().glBlendFunc(sfactor,dfactor);
}

GL_APICALL void  GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha){
    GET_CTX();
    SET_ERROR_IF(
!(GLESv2Validate::blendSrc(srcRGB) && GLESv2Validate::blendDst(dstRGB) && GLESv2Validate::blendSrc(srcAlpha) && GLESv2Validate::blendDst(dstAlpha)),GL_INVALID_ENUM);
    ctx->setBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    ctx->dispatcher().glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GL_APICALL void  GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target), GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLESv2Validate::bufferUsage(ctx, usage), GL_INVALID_ENUM);
    ctx->setBufferData(target,size,data,usage);
    ctx->dispatcher().glBufferData(target, size, data, usage);
}

GL_APICALL void  GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target), GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    SET_ERROR_IF(!ctx->setBufferSubData(target,offset,size,data),GL_INVALID_VALUE);
    ctx->dispatcher().glBufferSubData(target, offset, size, data);
}


GL_APICALL GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target){
    GET_CTX_V2_RET(GL_FRAMEBUFFER_COMPLETE);
    RET_AND_SET_ERROR_IF(!GLESv2Validate::framebufferTarget(ctx, target), GL_INVALID_ENUM, GL_FRAMEBUFFER_COMPLETE);
    // We used to issue ctx->drawValidate() here, but it can corrupt the status of
    // separately bound draw/read framebuffer objects. So we just don't call it now.
    return ctx->dispatcher().glCheckFramebufferStatusEXT(target);
}

GL_APICALL void  GL_APIENTRY glClear(GLbitfield mask){
    GET_CTX();
    GLbitfield allowed_bits = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    GLbitfield has_disallowed_bits = (mask & ~allowed_bits);
    SET_ERROR_IF(has_disallowed_bits, GL_INVALID_VALUE);

    if (ctx->getMajorVersion() < 3)
        ctx->drawValidate();

    ctx->dispatcher().glClear(mask);
}
GL_APICALL void  GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){
    GET_CTX();
    ctx->setClearColor(red,green, blue, alpha);
    ctx->dispatcher().glClearColor(red,green,blue,alpha);
}
GL_APICALL void  GL_APIENTRY glClearDepthf(GLclampf depth){
    GET_CTX();
    ctx->setClearDepth(depth);
    ctx->dispatcher().glClearDepth(depth);
}
GL_APICALL void  GL_APIENTRY glClearStencil(GLint s){
    GET_CTX();
    ctx->setClearStencil(s);
    ctx->dispatcher().glClearStencil(s);
}
GL_APICALL void  GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){
    GET_CTX();
    ctx->setColorMask(red, green, blue, alpha);
    ctx->dispatcher().glColorMask(red,green,blue,alpha);
}

GL_APICALL void  GL_APIENTRY glCompileShader(GLuint shader){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(objData->getDataType()!= SHADER_DATA,GL_INVALID_OPERATION);
        ShaderParser* sp = (ShaderParser*)objData;
        SET_ERROR_IF(sp->getDeleteStatus(), GL_INVALID_VALUE);
        GLint compileStatus;
        if (sp->validShader()) {
            ctx->dispatcher().glCompileShader(globalShaderName);

            GLsizei infoLogLength=0;
            GLchar* infoLog;
            ctx->dispatcher().glGetShaderiv(globalShaderName,GL_INFO_LOG_LENGTH,&infoLogLength);
            infoLog = new GLchar[infoLogLength+1];
            ctx->dispatcher().glGetShaderInfoLog(globalShaderName,infoLogLength,NULL,infoLog);
            if (infoLogLength == 0) {
                infoLog[0] = 0;
            }
            sp->setInfoLog(infoLog);

            ctx->dispatcher().glGetShaderiv(globalShaderName,GL_COMPILE_STATUS,&compileStatus);
            sp->setCompileStatus(compileStatus == GL_FALSE ? false : true);
        } else {
            ctx->dispatcher().glCompileShader(globalShaderName);
            sp->setCompileStatus(false);
            ctx->dispatcher().glGetShaderiv(globalShaderName,GL_COMPILE_STATUS,&compileStatus);
            if (compileStatus != GL_FALSE) {
                fprintf(stderr, "%s: Warning: underlying GL compiled invalid shader!\n", __func__);
            }
        }
    }
}

GL_APICALL void  GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetEx(ctx, target),GL_INVALID_ENUM);
    SET_ERROR_IF(level < 0 || imageSize < 0, GL_INVALID_VALUE);

    doCompressedTexImage2D(ctx, target, level, internalformat,
                                width, height, border,
                                imageSize, data, glTexImage2D);

    TextureData* texData = getTextureTargetData(target);
    if (texData) {
        texData->compressed = true;
        texData->compressedFormat = internalformat;
    }
}

GL_APICALL void  GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetEx(ctx, target),GL_INVALID_ENUM);
    if (ctx->shareGroup().get()) {
        TextureData* texData = getTextureTargetData(target);
        if (texData) {
            if (isEtcFormat(texData->compressedFormat)) {
                int encodedDataSize =
                    etc_get_encoded_data_size(
                        getEtcFormat(texData->compressedFormat),
                        width, height);
                SET_ERROR_IF(imageSize != encodedDataSize, GL_INVALID_VALUE);
                GLsizei lvlWidth = texData->width >> level;
                GLsizei lvlHeight = texData->height >> level;
                if (texData->width && !lvlWidth) lvlWidth = 1;
                if (texData->height && !lvlHeight) lvlHeight = 1;
                SET_ERROR_IF((width % 4) && ((xoffset + width) != (GLsizei)lvlWidth), GL_INVALID_OPERATION);
                SET_ERROR_IF((height % 4) && ((yoffset + height) != (GLsizei)lvlHeight), GL_INVALID_OPERATION);
                SET_ERROR_IF(xoffset % 4, GL_INVALID_OPERATION);
                SET_ERROR_IF(yoffset % 4, GL_INVALID_OPERATION);
            }
            SET_ERROR_IF(format != texData->compressedFormat, GL_INVALID_OPERATION);
        }
        SET_ERROR_IF(ctx->getMajorVersion() < 3 && !data, GL_INVALID_OPERATION);
        doCompressedTexImage2D(ctx, target, level, format,
                width, height, 0, imageSize, data,
                [xoffset, yoffset](GLenum target, GLint level,
                GLint internalformat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type,
                const GLvoid* data) {
                    glTexSubImage2D(target, level, xoffset, yoffset,
                        width, height, format, type, data);
                });
    }
}

void s_glInitTexImage2D(GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLint border, GLenum* format,
        GLenum* type, GLint* internalformat_out) {
    GET_CTX();

    if (ctx->shareGroup().get()) {
        TextureData *texData = getTextureTargetData(target);

        if (texData) {
            texData->hasStorage = true;
        }

        if (texData && level == 0) {
            assert(texData->target == GL_TEXTURE_2D ||
                    texData->target == GL_TEXTURE_CUBE_MAP);
            if (GLESv2Validate::isCompressedFormat(internalformat)) {
                texData->compressed = true;
                texData->compressedFormat = internalformat;
                texData->internalFormat = decompressedInternalFormat(
                    internalformat);
            } else {
                texData->internalFormat = internalformat;
            }
            if (internalformat_out) {
                *internalformat_out = texData->internalFormat;
            }
            texData->width = width;
            texData->height = height;
            texData->border = border;
            if (format) texData->format = *format;
            if (type) texData->type = *type;

            if (texData->sourceEGLImage != 0) {
                //
                // This texture was a target of EGLImage,
                // but now it is re-defined so we need to
                // re-generate global texture name for it.
                //
                unsigned int tex = ctx->getBindedTexture(target);
                ctx->shareGroup()->genName(NamedObjectType::TEXTURE, tex,
                        false);
                unsigned int globalTextureName =
                    ctx->shareGroup()->getGlobalName(
                            NamedObjectType::TEXTURE, tex);
                ctx->dispatcher().glBindTexture(GL_TEXTURE_2D,
                        globalTextureName);
                texData->sourceEGLImage = 0;
                texData->globalName = globalTextureName;
            }
        }
    }

}

void s_glInitTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border){
    GET_CTX();

    if (ctx->shareGroup().get()){
        TextureData *texData = getTextureTargetData(target);

        if (texData) {
            texData->hasStorage = true;
        }

        if (texData && level == 0) {
            texData->width = width;
            texData->height = height;
            texData->depth = depth;
            texData->border = border;
            texData->internalFormat = internalformat;
            texData->target = target;
        }
    }
}

GL_APICALL void  GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::pixelFrmt(ctx,internalformat) &&
                   (GLESv2Validate::textureTarget(ctx, target) ||
                    GLESv2Validate::textureTargetEx(ctx, target))), GL_INVALID_ENUM);
    SET_ERROR_IF((GLESv2Validate::textureIsCubeMap(target) && width != height), GL_INVALID_VALUE);
    SET_ERROR_IF(border != 0,GL_INVALID_VALUE);
    // TODO: set up the correct format and type
    s_glInitTexImage2D(target, level, internalformat, width, height, border,
            nullptr, nullptr, nullptr);
    ctx->dispatcher().glCopyTexImage2D(target,level,internalformat,x,y,width,height,border);
}

GL_APICALL void  GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) ||
                   GLESv2Validate::textureTargetEx(ctx, target)), GL_INVALID_ENUM);
    ctx->dispatcher().glCopyTexSubImage2D(target,level,xoffset,yoffset,x,y,width,height);
}

GL_APICALL GLuint GL_APIENTRY glCreateProgram(void){
    GET_CTX_RET(0);
    if(ctx->shareGroup().get()) {
        ProgramData* programInfo = new ProgramData();
        const GLuint localProgramName =
                ctx->shareGroup()->genName(ShaderProgramType::PROGRAM, 0, true);
        ctx->shareGroup()->setObjectData(NamedObjectType::SHADER_OR_PROGRAM,
                                         localProgramName,
                                         ObjectDataPtr(programInfo));
        programInfo->addProgramName(ctx->shareGroup()->getGlobalName(
                         NamedObjectType::SHADER_OR_PROGRAM, localProgramName));
        return localProgramName;
    }
    return 0;
}

GL_APICALL GLuint GL_APIENTRY glCreateShader(GLenum type){
    GET_CTX_V2_RET(0);
    // Lazy init so we can catch the caps.
    if (!shaderParserInitialized) {
        shaderParserInitialized = true;

        GLint maxVertexAttribs; ctx->dispatcher().glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
        GLint maxVertexUniformVectors; ctx->dispatcher().glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
        GLint maxVaryingVectors; ctx->dispatcher().glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVectors);
        GLint maxVertexTextureImageUnits; ctx->dispatcher().glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureImageUnits);
        GLint maxCombinedTexImageUnits; ctx->dispatcher().glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTexImageUnits);
        GLint maxTextureImageUnits; ctx->dispatcher().glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);
        GLint maxFragmentUniformVectors; ctx->dispatcher().glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformVectors);
        GLint maxDrawBuffers; ctx->dispatcher().glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

        GLint fragmentPrecisionHigh = 1;

        GLint maxVertexOutputVectors; ctx->dispatcher().glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &maxVertexOutputVectors);
        GLint maxFragmentInputVectors; ctx->dispatcher().glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &maxFragmentInputVectors);
        GLint minProgramTexelOffset; ctx->dispatcher().glGetIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET, &minProgramTexelOffset);
        GLint maxProgramTexelOffset; ctx->dispatcher().glGetIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET, &maxProgramTexelOffset);

        // Clear GL errors if the underlying GL doesn't have those enums.
        ctx->dispatcher().glGetError();

        GLint maxDualSourceDrawBuffers = 1;

        ANGLEShaderParser::globalInitialize(
                maxVertexAttribs,
                maxVertexUniformVectors,
                maxVaryingVectors,
                maxVertexTextureImageUnits,
                maxCombinedTexImageUnits,
                maxTextureImageUnits,
                maxFragmentUniformVectors,
                maxDrawBuffers,
                fragmentPrecisionHigh,
                maxVertexOutputVectors,
                maxFragmentInputVectors,
                minProgramTexelOffset,
                maxProgramTexelOffset,
                maxDualSourceDrawBuffers);
    }
    RET_AND_SET_ERROR_IF(!GLESv2Validate::shaderType(ctx, type), GL_INVALID_ENUM, 0);
    if(ctx->shareGroup().get()) {
        ShaderProgramType shaderProgramType;
        switch (type) {
        case GL_VERTEX_SHADER:
            shaderProgramType = ShaderProgramType::VERTEX_SHADER;
            break;
        case GL_FRAGMENT_SHADER:
            shaderProgramType = ShaderProgramType::FRAGMENT_SHADER;
            break;
        case GL_COMPUTE_SHADER:
            shaderProgramType = ShaderProgramType::COMPUTE_SHADER;
            break;
        default:
            shaderProgramType = ShaderProgramType::VERTEX_SHADER;
            break;
        }
        const GLuint localShaderName = ctx->shareGroup()->genName(
                                                shaderProgramType, 0, true);
        ShaderParser* sp = new ShaderParser(type);
        ctx->shareGroup()->setObjectData(NamedObjectType::SHADER_OR_PROGRAM,
                                         localShaderName, ObjectDataPtr(sp));
        return localShaderName;
    }
    return 0;
}

GL_APICALL void  GL_APIENTRY glCullFace(GLenum mode){
    GET_CTX();
    ctx->setCullFace(mode);
    ctx->dispatcher().glCullFace(mode);
}

GL_APICALL void  GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i < n; i++){
            ctx->shareGroup()->deleteName(NamedObjectType::VERTEXBUFFER,
                                          buffers[i]);
            ctx->unbindBuffer(buffers[i]);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers){
    GET_CTX_V2();
    SET_ERROR_IF(n < 0, GL_INVALID_VALUE);
    for (int i = 0; i < n; i++) {
        if (ctx->getFramebufferBinding(GL_FRAMEBUFFER) == framebuffers[i]) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else if (ctx->getFramebufferBinding(GL_READ_FRAMEBUFFER) == framebuffers[i]) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
        ctx->deleteFBO(framebuffers[i]);
    }
}

static void s_detachFromFramebuffer(NamedObjectType bufferType,
                                    GLuint texture,
                                    GLenum target) {
    GET_CTX();
    GLuint fbName = ctx->getFramebufferBinding(target);
    if (!fbName) return;
    auto fbObj = ctx->getFBOData(fbName);
    if (fbObj == NULL) return;
    const GLenum kAttachments[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7,
        GL_COLOR_ATTACHMENT8,
        GL_COLOR_ATTACHMENT9,
        GL_COLOR_ATTACHMENT10,
        GL_COLOR_ATTACHMENT11,
        GL_COLOR_ATTACHMENT12,
        GL_COLOR_ATTACHMENT13,
        GL_COLOR_ATTACHMENT14,
        GL_COLOR_ATTACHMENT15,
        GL_DEPTH_ATTACHMENT,
        GL_STENCIL_ATTACHMENT,
        GL_DEPTH_STENCIL_ATTACHMENT };
    const size_t sizen = sizeof(kAttachments)/sizeof(GLenum);
    GLenum textarget;
    for (size_t i = 0; i < sizen; ++i ) {
        GLuint name = fbObj->getAttachment(kAttachments[i], &textarget, NULL);
        if (name != texture) continue;
        if (NamedObjectType::TEXTURE == bufferType &&
            GLESv2Validate::textureTargetEx(ctx, textarget)) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, kAttachments[i], textarget, 0, 0);
        } else if (NamedObjectType::RENDERBUFFER == bufferType &&
                   GLESv2Validate::renderbufferTarget(textarget)) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, kAttachments[i], textarget, 0);
        }
        // detach
        fbObj->setAttachment(kAttachments[i], (GLenum)0, 0, nullptr, false);
    }
}

GL_APICALL void  GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i < n; i++){
            ctx->shareGroup()->deleteName(NamedObjectType::RENDERBUFFER,
                                          renderbuffers[i]);
            s_detachFromFramebuffer(NamedObjectType::RENDERBUFFER,
                                    renderbuffers[i], GL_DRAW_FRAMEBUFFER);
            s_detachFromFramebuffer(NamedObjectType::RENDERBUFFER,
                                    renderbuffers[i], GL_READ_FRAMEBUFFER);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i < n; i++){
            if (textures[i]!=0) {
                if (ctx->getBindedTexture(GL_TEXTURE_2D) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_2D,0);
                if (ctx->getBindedTexture(GL_TEXTURE_CUBE_MAP) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_CUBE_MAP,0);
                if (ctx->getBindedTexture(GL_TEXTURE_2D_ARRAY) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_2D_ARRAY,0);
                if (ctx->getBindedTexture(GL_TEXTURE_3D) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_3D,0);
                if (ctx->getBindedTexture(GL_TEXTURE_2D_MULTISAMPLE) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_2D_MULTISAMPLE,0);
                s_detachFromFramebuffer(NamedObjectType::TEXTURE, textures[i], GL_DRAW_FRAMEBUFFER);
                s_detachFromFramebuffer(NamedObjectType::TEXTURE, textures[i], GL_READ_FRAMEBUFFER);
                ctx->shareGroup()->deleteName(NamedObjectType::TEXTURE,
                                              textures[i]);
            }
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteProgram(GLuint program){
    GET_CTX();
    if(program && ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(!globalProgramName, GL_INVALID_VALUE);

        auto programData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(!(programData->getDataType()==PROGRAM_DATA),
                GL_INVALID_OPERATION);
        ProgramData* pData = (ProgramData*)programData;
        if (pData && pData->isInUse()) {
            pData->setDeleteStatus(true);
            return;
        }
        s_detachShader(ctx, program, pData->getAttachedVertexShader());
        s_detachShader(ctx, program, pData->getAttachedFragmentShader());
        s_detachShader(ctx, program, pData->getAttachedComputeShader());

        ctx->shareGroup()->deleteName(NamedObjectType::SHADER_OR_PROGRAM, program);
    }
}

GL_APICALL void  GL_APIENTRY glDeleteShader(GLuint shader){
    GET_CTX();
    if(shader && ctx->shareGroup().get()) {
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!globalShaderName, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!objData ,GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType()!=SHADER_DATA,GL_INVALID_OPERATION);
        ShaderParser* sp = (ShaderParser*)objData;
        SET_ERROR_IF(sp->getDeleteStatus(), GL_INVALID_VALUE);
        if (sp->hasAttachedPrograms()) {
            sp->setDeleteStatus(true);
        } else {
            ctx->shareGroup()->deleteName(NamedObjectType::SHADER_OR_PROGRAM, shader);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDepthFunc(GLenum func){
    GET_CTX();
    ctx->setDepthFunc(func);
    ctx->dispatcher().glDepthFunc(func);
}
GL_APICALL void  GL_APIENTRY glDepthMask(GLboolean flag){
    GET_CTX();
    ctx->setDepthMask(flag);
    ctx->dispatcher().glDepthMask(flag);
}

GL_APICALL void  GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar){
    GET_CTX();
    ctx->setDepthRangef(zNear, zFar);
    ctx->dispatcher().glDepthRange(zNear,zFar);
}

GL_APICALL void  GL_APIENTRY glDetachShader(GLuint program, GLuint shader){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName==0, GL_INVALID_VALUE);

        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(!objData,GL_INVALID_OPERATION);
        SET_ERROR_IF(!(objData->getDataType()==PROGRAM_DATA) ,GL_INVALID_OPERATION);

        ProgramData* programData = (ProgramData*)objData;
        SET_ERROR_IF(!programData->isAttached(shader),GL_INVALID_OPERATION);
        programData->detachShader(shader);

        s_detachShader(ctx, program, shader);

        ctx->dispatcher().glDetachShader(globalProgramName,globalShaderName);
    }
}

GL_APICALL void  GL_APIENTRY glDisable(GLenum cap){
    GET_CTX();
    ctx->setEnable(cap, false);
    ctx->dispatcher().glDisable(cap);
}

GL_APICALL void  GL_APIENTRY glDisableVertexAttribArray(GLuint index){
    GET_CTX();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->enableArr(index,false);
    ctx->dispatcher().glDisableVertexAttribArray(index);
}


static void s_glDrawPre(GLESv2Context* ctx, GLenum mode) {
    if (ctx->getMajorVersion() < 3)
        ctx->drawValidate();

    if (mode == GL_POINTS) {
        //Enable texture generation for GL_POINTS and gl_PointSize shader variable
        //GLES2 assumes this is enabled by default, we need to set this state for GL
        if (mode==GL_POINTS) {
            ctx->dispatcher().glEnable(GL_POINT_SPRITE);
            ctx->dispatcher().glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        }
    }
}

static void s_glDrawPost(GLESv2Context* ctx, GLenum mode) {
    if (mode == GL_POINTS) {
        if (mode==GL_POINTS) {
            ctx->dispatcher().glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
            ctx->dispatcher().glDisable(GL_POINT_SPRITE);
        }
    }
}

static void s_glDrawEmulateClientArraysPre(GLESv2Context* ctx) {
    GLuint currarrbinding = 0;
    GLuint curreltbinding = 0;
    if (ctx->shareGroup()) {
        currarrbinding =
            ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, ctx->getBuffer(GL_ARRAY_BUFFER));
        curreltbinding =
            ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, ctx->getBuffer(GL_ELEMENT_ARRAY_BUFFER));
        if (currarrbinding)
            ctx->dispatcher().glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (curreltbinding)
            ctx->dispatcher().glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

static void s_glDrawEmulateClientArraysPost(GLESv2Context* ctx) {
    GLuint currarrbinding = 0;
    GLuint curreltbinding = 0;
    if (ctx->shareGroup()) {
        currarrbinding =
            ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, ctx->getBuffer(GL_ARRAY_BUFFER));
        curreltbinding =
            ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, ctx->getBuffer(GL_ELEMENT_ARRAY_BUFFER));
        if (currarrbinding)
            ctx->dispatcher().glBindBuffer(GL_ARRAY_BUFFER, currarrbinding);
        if (curreltbinding)
            ctx->dispatcher().glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curreltbinding);
    }
}

static void s_glDrawSetupArraysPre(GLESv2Context* ctx, GLESConversionArrays& tmpArrs, GLint first, GLsizei count, GLenum type, const GLvoid* indices, bool direct) {
    ctx->setupArraysPointers(tmpArrs,first,count,type,indices,direct);

    if (ctx->needAtt0PreDrawValidation()) {
        if (indices) {
            const unsigned int maxIndex = ctx->findMaxIndex(count, type, indices);
            ctx->validateAtt0PreDraw(maxIndex);
        } else {
            ctx->validateAtt0PreDraw(count);
        }
    }
}

static void s_glDrawSetupArraysPost(GLESv2Context* ctx) {
    ctx->validateAtt0PostDraw();
}

GL_APICALL void  GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count){
    GET_CTX_V2();
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!GLESv2Validate::drawMode(mode),GL_INVALID_ENUM);

    s_glDrawPre(ctx, mode);
    if (ctx->vertexAttributesBufferBacked()) {
        ctx->dispatcher().glDrawArrays(mode,first,count);
    } else {
        GLESConversionArrays tmpArrs;
        s_glDrawSetupArraysPre(ctx, tmpArrs, first, count, 0, NULL, true);
        ctx->dispatcher().glDrawArrays(mode,first,count);
        s_glDrawSetupArraysPost(ctx);
    }
    s_glDrawPost(ctx, mode);
}

GL_APICALL void  GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    GET_CTX_V2();
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode) && GLESv2Validate::drawType(type)),GL_INVALID_ENUM);

    s_glDrawPre(ctx, mode);

    if (ctx->isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER)) {
        ctx->dispatcher().glDrawElements(mode,count,type,indices);
    } else {
        s_glDrawEmulateClientArraysPre(ctx);
        GLESConversionArrays tmpArrs;
        s_glDrawSetupArraysPre(ctx,tmpArrs,0,count,type,indices,false);
        ctx->dispatcher().glDrawElements(mode,count,type,indices);
        s_glDrawSetupArraysPost(ctx);
        s_glDrawEmulateClientArraysPost(ctx);
    }

    s_glDrawPost(ctx, mode);
}

GL_APICALL void  GL_APIENTRY glEnable(GLenum cap){
    GET_CTX();
    ctx->setEnable(cap, true);
    ctx->dispatcher().glEnable(cap);
}

GL_APICALL void  GL_APIENTRY glEnableVertexAttribArray(GLuint index){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->enableArr(index,true);
    ctx->dispatcher().glEnableVertexAttribArray(index);
}

GL_APICALL void  GL_APIENTRY glFinish(void){
    GET_CTX();
    ctx->dispatcher().glFinish();
}
GL_APICALL void  GL_APIENTRY glFlush(void){
    GET_CTX();
    ctx->dispatcher().glFlush();
}


GL_APICALL void  GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(ctx, target) &&
                   GLESv2Validate::renderbufferTarget(renderbuffertarget) &&
                   GLESv2Validate::framebufferAttachment(ctx, attachment)), GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->shareGroup().get(), GL_INVALID_OPERATION);
    SET_ERROR_IF(ctx->isDefaultFBOBound(target), GL_INVALID_OPERATION);

    GLuint globalRenderbufferName = 0;
    ObjectDataPtr obj;

    // generate the renderbuffer object if not yet exist
    if(renderbuffer) {
        if (!ctx->shareGroup()->isObject(NamedObjectType::RENDERBUFFER,
                                         renderbuffer)) {
            ctx->shareGroup()->genName(NamedObjectType::RENDERBUFFER,
                                       renderbuffer);
            obj = ObjectDataPtr(new RenderbufferData());
            ctx->shareGroup()->setObjectData(NamedObjectType::RENDERBUFFER,
                                             renderbuffer, obj);
        }
        else {
            obj = ctx->shareGroup()->getObjectDataPtr(
                    NamedObjectType::RENDERBUFFER, renderbuffer);
        }

        globalRenderbufferName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::RENDERBUFFER, renderbuffer);
    }

    // Update the the current framebuffer object attachment state
    GLuint fbName = ctx->getFramebufferBinding(target);
    auto fbObj = ctx->getFBOData(fbName);
    if (fbObj != NULL) {
        fbObj->setAttachment(attachment, renderbuffertarget, renderbuffer, obj);
    }

    if (renderbuffer && obj.get() != NULL) {
        RenderbufferData *rbData = (RenderbufferData *)obj.get();
        if (rbData->eglImageGlobalTexObject) {
            //
            // This renderbuffer object is an eglImage target
            // attach the eglimage's texture instead the renderbuffer.
            //
            ctx->dispatcher().glFramebufferTexture2DEXT(target,
                                    attachment,
                                    GL_TEXTURE_2D,
                                    rbData->eglImageGlobalTexObject->getGlobalName(),
                                    0);
            return;
        }
    }

    ctx->dispatcher().glFramebufferRenderbufferEXT(target,attachment,renderbuffertarget,globalRenderbufferName);

    sUpdateFboEmulation(ctx);
}

GL_APICALL void  GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(ctx, target) &&
                   GLESv2Validate::textureTargetEx(ctx, textarget)  &&
                   GLESv2Validate::framebufferAttachment(ctx, attachment)), GL_INVALID_ENUM);
    SET_ERROR_IF(ctx->getMajorVersion() < 3 && level != 0, GL_INVALID_VALUE);
    SET_ERROR_IF(!ctx->shareGroup().get(), GL_INVALID_OPERATION);
    SET_ERROR_IF(ctx->isDefaultFBOBound(target), GL_INVALID_OPERATION);

    GLuint globalTextureName = 0;

    if(texture) {
        if (!ctx->shareGroup()->isObject(NamedObjectType::TEXTURE, texture)) {
            ctx->shareGroup()->genName(NamedObjectType::TEXTURE, texture);
        }
        ObjectLocalName texname = ctx->getTextureLocalName(textarget,texture);
        globalTextureName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::TEXTURE, texname);
    }

    ctx->dispatcher().glFramebufferTexture2DEXT(target,attachment,textarget,globalTextureName,level);

    // Update the the current framebuffer object attachment state
    GLuint fbName = ctx->getFramebufferBinding(target);
    auto fbObj = ctx->getFBOData(fbName);
    if (fbObj) {
        fbObj->setAttachment(attachment, textarget,
                              texture, ObjectDataPtr());
    }

    sUpdateFboEmulation(ctx);
}


GL_APICALL void  GL_APIENTRY glFrontFace(GLenum mode){
    GET_CTX();
    ctx->setFrontFace(mode);
    ctx->dispatcher().glFrontFace(mode);
}

GL_APICALL void  GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            buffers[i] = ctx->shareGroup()->genName(
                    NamedObjectType::VERTEXBUFFER, 0, true);
            //generating vbo object related to this buffer name
            ctx->shareGroup()->setObjectData(NamedObjectType::VERTEXBUFFER,
                                             buffers[i],
                                             ObjectDataPtr(new GLESbuffer()));
        }
    }
}

GL_APICALL void  GL_APIENTRY glGenerateMipmap(GLenum target){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::textureTarget(ctx, target), GL_INVALID_ENUM);
    // Assuming we advertised GL_OES_texture_npot
    ctx->dispatcher().glGenerateMipmapEXT(target);
}

GL_APICALL void  GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            framebuffers[i] = ctx->genFBOName(0, true);
            ctx->setFBOData(framebuffers[i],
                    ObjectDataPtr(new FramebufferData(framebuffers[i])));
        }
    }
}

GL_APICALL void  GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            renderbuffers[i] = ctx->shareGroup()->genName(
                    NamedObjectType::RENDERBUFFER, 0, true);
            ctx->shareGroup()->setObjectData(
                    NamedObjectType::RENDERBUFFER, renderbuffers[i],
                    ObjectDataPtr(new RenderbufferData()));
        }
    }
}

GL_APICALL void  GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            textures[i] = ctx->shareGroup()->genName(NamedObjectType::TEXTURE,
                                                     0, true);
        }
    }
}

GL_APICALL void  GL_APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ctx->dispatcher().glGetActiveAttrib(globalProgramName,index,bufsize,length,size,type,name);
    }
}

GL_APICALL void  GL_APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ctx->dispatcher().glGetActiveUniform(globalProgramName,index,bufsize,length,size,type,name);
    }
}

GL_APICALL void  GL_APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        ctx->dispatcher().glGetAttachedShaders(globalProgramName,maxcount,count,shaders);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        GLint numShaders=0;
        ctx->dispatcher().glGetProgramiv(globalProgramName,GL_ATTACHED_SHADERS,&numShaders);
        for(int i=0 ; i < maxcount && i<numShaders ;i++){
            shaders[i] = ctx->shareGroup()->getLocalName(
                    NamedObjectType::SHADER_OR_PROGRAM, shaders[i]);
        }
    }
}

GL_APICALL int GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar* name){
     GET_CTX_RET(-1);
     if(ctx->shareGroup().get()) {
         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                 NamedObjectType::SHADER_OR_PROGRAM, program);
         RET_AND_SET_ERROR_IF(globalProgramName == 0, GL_INVALID_VALUE, -1);
         auto objData = ctx->shareGroup()->getObjectData(
                 NamedObjectType::SHADER_OR_PROGRAM, program);
         RET_AND_SET_ERROR_IF(objData->getDataType() != PROGRAM_DATA,
                              GL_INVALID_OPERATION, -1);
         ProgramData* pData = (ProgramData*)objData;
         RET_AND_SET_ERROR_IF(pData->getLinkStatus() != GL_TRUE,
                              GL_INVALID_OPERATION, -1);
         int ret = ctx->dispatcher().glGetAttribLocation(globalProgramName, name);
         if (ret != -1) {
             pData->linkedAttribLocation(name, ret);
         }
         return ret;
     }
     return -1;
}

template <typename T>
using GLStateQueryFunc = void (*)(GLenum pname, T* params);

template <typename T>
using GLStateQueryFuncIndexed = void (*)(GLenum pname, GLuint index, T* params);

static void s_glGetBooleanv_wrapper(GLenum pname, GLboolean* params) {
    GET_CTX();
    ctx->dispatcher().glGetBooleanv(pname, params);
}

static void s_glGetIntegerv_wrapper(GLenum pname, GLint* params) {
    GET_CTX();
    ctx->dispatcher().glGetIntegerv(pname, params);
}

static void s_glGetInteger64v_wrapper(GLenum pname, GLint64* params) {
    GET_CTX();
    ctx->dispatcher().glGetInteger64v(pname, params);
}

static void s_glGetFloatv_wrapper(GLenum pname, GLfloat* params) {
    GET_CTX();
    ctx->dispatcher().glGetFloatv(pname, params);
}

static void s_glGetIntegeri_v_wrapper(GLenum pname, GLuint index, GLint* data) {
    GET_CTX_V2();
    ctx->dispatcher().glGetIntegeri_v(pname, index, data);
}

static void s_glGetInteger64i_v_wrapper(GLenum pname, GLuint index, GLint64* data) {
    GET_CTX_V2();
    ctx->dispatcher().glGetInteger64i_v(pname, index, data);
}

template <typename T>
static void s_glStateQueryTv(bool es2, GLenum pname, T* params, GLStateQueryFunc<T> getter) {
    T i;
    GET_CTX_V2();
    switch (pname) {
    case GL_CURRENT_PROGRAM:
        if (ctx->shareGroup().get()) {
            getter(pname,&i);
            *params = ctx->shareGroup()->getLocalName(NamedObjectType::SHADER_OR_PROGRAM,
                                                      i);
        }
        break;
    case GL_FRAMEBUFFER_BINDING:
    case GL_READ_FRAMEBUFFER_BINDING:
        getter(pname,&i);
        *params = ctx->getFBOLocalName(i);
        break;
    case GL_RENDERBUFFER_BINDING:
        if (ctx->shareGroup().get()) {
            getter(pname,&i);
            *params = ctx->shareGroup()->getLocalName(
                    NamedObjectType::RENDERBUFFER, i);
        }
        break;
    case GL_READ_BUFFER:
    case GL_DRAW_BUFFER0:
        if (ctx->shareGroup().get()) {
            getter(pname, &i);
            GLenum target = pname == GL_READ_BUFFER ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
            if (ctx->isDefaultFBOBound(target) && (GLint)i == GL_COLOR_ATTACHMENT0) {
                i = (T)GL_BACK;
            }
            *params = i;
        }
        break;
    case GL_VERTEX_ARRAY_BINDING:
        getter(pname,&i);
        *params = ctx->getVAOLocalName(i);
        break;
    case GL_ARRAY_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_ARRAY_BUFFER);
        break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_ELEMENT_ARRAY_BUFFER);
        break;
    case GL_COPY_READ_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_COPY_READ_BUFFER);
        break;
    case GL_COPY_WRITE_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_COPY_WRITE_BUFFER);
        break;
    case GL_PIXEL_PACK_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_PIXEL_PACK_BUFFER);
        break;
    case GL_PIXEL_UNPACK_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_PIXEL_UNPACK_BUFFER);
        break;
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
        break;
    case GL_UNIFORM_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_UNIFORM_BUFFER);
        break;
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_ATOMIC_COUNTER_BUFFER);
        break;
    case GL_DISPATCH_INDIRECT_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_DISPATCH_INDIRECT_BUFFER);
        break;
    case GL_DRAW_INDIRECT_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_DRAW_INDIRECT_BUFFER);
        break;
    case GL_SHADER_STORAGE_BUFFER_BINDING:
        *params = ctx->getBuffer(GL_SHADER_STORAGE_BUFFER);
        break;
    case GL_TEXTURE_BINDING_2D:
        *params = ctx->getBindedTexture(GL_TEXTURE_2D);
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP:
        *params = ctx->getBindedTexture(GL_TEXTURE_CUBE_MAP);
        break;
    case GL_TEXTURE_BINDING_2D_ARRAY:
        *params = ctx->getBindedTexture(GL_TEXTURE_2D_ARRAY);
        break;
    case GL_TEXTURE_BINDING_3D:
        *params = ctx->getBindedTexture(GL_TEXTURE_3D);
        break;
    case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
        *params = ctx->getBindedTexture(GL_TEXTURE_2D_MULTISAMPLE);
        break;
    case GL_SAMPLER_BINDING:
        if (ctx->shareGroup().get()) {
            getter(pname,&i);
            *params = ctx->shareGroup()->getLocalName(
                    NamedObjectType::SAMPLER, i);
        }
        break;

    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        *params = (T)getCompressedFormats(NULL);
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        {
            int nparams = getCompressedFormats(NULL);
            if (nparams > 0) {
                int* iparams = new int[nparams];
                getCompressedFormats(iparams);
                for (int i = 0; i < nparams; i++) {
                    params[i] = (T)iparams[i];
                }
                delete [] iparams;
            }
        }
        break;
    case GL_SHADER_COMPILER:
        if(es2)
            getter(pname, params);
        else
            *params = 1;
        break;

    case GL_SHADER_BINARY_FORMATS:
        if(es2)
            getter(pname,params);
        break;

    case GL_NUM_SHADER_BINARY_FORMATS:
        if(es2)
            getter(pname,params);
        else
            *params = 0;
        break;

    case GL_MAX_VERTEX_UNIFORM_VECTORS:
        if(es2)
            getter(pname,params);
        else
            *params = 128;
        break;

    case GL_MAX_VARYING_VECTORS:
        if(es2)
            getter(pname,params);
        else
            *params = 8;
        break;

    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
        if(es2)
            getter(pname,params);
        else
            *params = 16;
        break;

    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        getter(pname,params);
        break;
    case GL_STENCIL_BACK_VALUE_MASK:
    case GL_STENCIL_BACK_WRITEMASK:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK:
        {
            T myT = 0;
            getter(pname, &myT);
            *params = myT;
        }
        break;
    default:
        getter(pname,params);
        break;
    }
}

template <typename T>
static void s_glStateQueryTi_v(GLenum pname, GLuint index, T* params, GLStateQueryFuncIndexed<T> getter) {
    GET_CTX_V2();
    switch (pname) {
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
        *params = ctx->getIndexedBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, index);
        break;
    case GL_UNIFORM_BUFFER_BINDING:
        *params = ctx->getIndexedBuffer(GL_UNIFORM_BUFFER, index);
        break;
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:
        *params = ctx->getIndexedBuffer(GL_ATOMIC_COUNTER_BUFFER, index);
        break;
    case GL_SHADER_STORAGE_BUFFER_BINDING:
        *params = ctx->getIndexedBuffer(GL_SHADER_STORAGE_BUFFER, index);
        break;
    case GL_IMAGE_BINDING_NAME:
        // Need the local name here.
        getter(pname, index, params);
        *params = ctx->shareGroup()->getLocalName(NamedObjectType::TEXTURE, *params);
        break;
    default:
        getter(pname, index, params);
        break;
    }
}

GL_APICALL void  GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params){
    GET_CTX_V2();
#define TO_GLBOOL(params, x) \
    *params = x ? GL_TRUE : GL_FALSE; \

    GLint i;
    switch (pname) {
    case GL_CURRENT_PROGRAM:
        if (ctx->shareGroup().get()) {
            s_glGetIntegerv_wrapper(pname,&i);
            TO_GLBOOL(params,ctx->shareGroup()->getLocalName(NamedObjectType::SHADER_OR_PROGRAM, i));
        }
        break;
    case GL_FRAMEBUFFER_BINDING:
    case GL_READ_FRAMEBUFFER_BINDING:
        s_glGetIntegerv_wrapper(pname,&i);
        TO_GLBOOL(params,ctx->getFBOLocalName(i));
        break;
    case GL_RENDERBUFFER_BINDING:
        if (ctx->shareGroup().get()) {
            s_glGetIntegerv_wrapper(pname,&i);
            TO_GLBOOL(params,ctx->shareGroup()->getLocalName(NamedObjectType::RENDERBUFFER, i));
        }
        break;
    case GL_ARRAY_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_ARRAY_BUFFER));
        break;
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_ELEMENT_ARRAY_BUFFER));
        break;
    case GL_COPY_READ_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_COPY_READ_BUFFER));
        break;
    case GL_COPY_WRITE_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_COPY_WRITE_BUFFER));
        break;
    case GL_PIXEL_PACK_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_PIXEL_PACK_BUFFER));
        break;
    case GL_PIXEL_UNPACK_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_PIXEL_UNPACK_BUFFER));
        break;
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_TRANSFORM_FEEDBACK_BUFFER));
        break;
    case GL_UNIFORM_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_UNIFORM_BUFFER));
        break;
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_ATOMIC_COUNTER_BUFFER));
        break;
    case GL_DISPATCH_INDIRECT_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_DISPATCH_INDIRECT_BUFFER));
        break;
    case GL_DRAW_INDIRECT_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_DRAW_INDIRECT_BUFFER));
        break;
    case GL_SHADER_STORAGE_BUFFER_BINDING:
        TO_GLBOOL(params, ctx->getBuffer(GL_SHADER_STORAGE_BUFFER));
        break;
    case GL_TEXTURE_BINDING_2D:
        TO_GLBOOL(params, ctx->getBindedTexture(GL_TEXTURE_2D));
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP:
        TO_GLBOOL(params, ctx->getBindedTexture(GL_TEXTURE_CUBE_MAP));
        break;
    case GL_TEXTURE_BINDING_2D_ARRAY:
        TO_GLBOOL(params, ctx->getBindedTexture(GL_TEXTURE_2D_ARRAY));
        break;
    case GL_TEXTURE_BINDING_3D:
        TO_GLBOOL(params, ctx->getBindedTexture(GL_TEXTURE_3D));
        break;
    case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
        TO_GLBOOL(params, ctx->getBindedTexture(GL_TEXTURE_2D_MULTISAMPLE));
        break;
    case GL_SAMPLER_BINDING:
        if (ctx->shareGroup().get()) {
            s_glGetIntegerv_wrapper(pname,&i);
            TO_GLBOOL(params,ctx->shareGroup()->getLocalName(NamedObjectType::SAMPLER, i));
        }
        break;
    default:
        s_glGetBooleanv_wrapper(pname, params);
        break;
    }
}

GL_APICALL void  GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target), GL_INVALID_ENUM);
    SET_ERROR_IF(!GLESv2Validate::bufferParam(ctx, pname), GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    switch(pname) {
    case GL_BUFFER_SIZE:
        ctx->getBufferSize(target,params);
        break;
    case GL_BUFFER_USAGE:
        ctx->getBufferUsage(target,params);
        break;
    }
}


GL_APICALL GLenum GL_APIENTRY glGetError(void){
    GET_CTX_RET(GL_NO_ERROR)
    GLenum err = ctx->getGLerror();
    if(err != GL_NO_ERROR) {
        ctx->setGLerror(GL_NO_ERROR);
        return err;
    }
    return ctx->dispatcher().glGetError();
}

GL_APICALL void  GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params){
    GET_CTX();
    s_glStateQueryTv<GLfloat>(true, pname, params, s_glGetFloatv_wrapper);
}

GL_APICALL void  GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params){
    int destroyCtx = 0;
    GET_CTX();

    if (!ctx) {
        ctx = createGLESContext();
        if (ctx)
            destroyCtx = 1;
    }
    if (ctx->glGetIntegerv(pname,params))
    {
        if (destroyCtx)
            deleteGLESContext(ctx);
            return;
    }

    // For non-int64 glGetIntegerv, the following params have precision issues,
    // so just use glGetFloatv straight away:
    GLfloat floatVals[4];
    switch (pname) {
    case GL_DEPTH_RANGE:
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
    case GL_DEPTH_CLEAR_VALUE:
        ctx->dispatcher().glGetFloatv(pname, floatVals);
    default:
        break;
    }

    int converted_float_params = 0;

    switch (pname) {
    case GL_DEPTH_RANGE:
        converted_float_params = 2;
        break;
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
        converted_float_params = 4;
        break;
    case GL_DEPTH_CLEAR_VALUE:
        converted_float_params = 1;
        break;
    default:
        break;
    }

    if (converted_float_params) {
        for (int i = 0; i < converted_float_params; i++) {
            params[i] = (GLint)((GLint64)(floatVals[i] * 2147483647.0));
        }
        return;
    }

    bool es2 = ctx->getCaps()->GL_ARB_ES2_COMPATIBILITY;
    s_glStateQueryTv<GLint>(es2, pname, params, s_glGetIntegerv_wrapper);

    if (destroyCtx)
        deleteGLESContext(ctx);
}

GL_APICALL void  GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params){
    GET_CTX_V2();

    //
    // Take the attachment attribute from our state - if available
    //
    GLuint fbName = ctx->getFramebufferBinding(target);
    if (fbName) {
        auto fbObj = ctx->getFBOData(fbName);
        if (fbObj != NULL) {
            GLenum target;
            GLuint name = fbObj->getAttachment(attachment, &target, NULL);
            if (!name) {
                SET_ERROR_IF(pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE &&
                        pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, GL_INVALID_ENUM);
            }
            if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) {
                if (target == GL_TEXTURE_2D) {
                    *params = GL_TEXTURE;
                    return;
                }
                else if (target == GL_RENDERBUFFER) {
                    *params = GL_RENDERBUFFER;
                    return;
                } else {
                    *params = GL_NONE;
                }
            }
            else if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME) {
                *params = name;
                return;
            }
        }
    }

    if (ctx->isDefaultFBOBound(target)) {
        SET_ERROR_IF(
            attachment == GL_DEPTH_ATTACHMENT ||
            attachment == GL_STENCIL_ATTACHMENT ||
            attachment == GL_DEPTH_STENCIL_ATTACHMENT ||
            (attachment >= GL_COLOR_ATTACHMENT0 &&
             attachment <= GL_COLOR_ATTACHMENT15), GL_INVALID_OPERATION);
        SET_ERROR_IF(pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, GL_INVALID_ENUM);

        if (attachment == GL_BACK)
            attachment = GL_COLOR_ATTACHMENT0;
        if (attachment == GL_DEPTH)
            attachment = GL_DEPTH_ATTACHMENT;
        if (attachment == GL_STENCIL)
            attachment = GL_STENCIL_ATTACHMENT;
    }

    ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(target,attachment,pname,params);

    if (ctx->isDefaultFBOBound(target) && *params == GL_RENDERBUFFER) {
        *params = GL_FRAMEBUFFER_DEFAULT;
    }
}

GL_APICALL void  GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::renderbufferTarget(target) &&
                   GLESv2Validate::renderbufferParam(ctx, pname)), GL_INVALID_ENUM);

    //
    // If this is a renderbuffer which is eglimage's target, we
    // should query the underlying eglimage's texture object instead.
    //
    GLuint rb = ctx->getRenderbufferBinding();
    if (rb) {
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::RENDERBUFFER, rb);
        RenderbufferData *rbData = (RenderbufferData *)objData;
        if (rbData && rbData->eglImageGlobalTexObject) {
            GLenum texPname;
            switch(pname) {
                case GL_RENDERBUFFER_WIDTH:
                    texPname = GL_TEXTURE_WIDTH;
                    break;
                case GL_RENDERBUFFER_HEIGHT:
                    texPname = GL_TEXTURE_HEIGHT;
                    break;
                case GL_RENDERBUFFER_INTERNAL_FORMAT:
                    texPname = GL_TEXTURE_INTERNAL_FORMAT;
                    break;
                case GL_RENDERBUFFER_RED_SIZE:
                    texPname = GL_TEXTURE_RED_SIZE;
                    break;
                case GL_RENDERBUFFER_GREEN_SIZE:
                    texPname = GL_TEXTURE_GREEN_SIZE;
                    break;
                case GL_RENDERBUFFER_BLUE_SIZE:
                    texPname = GL_TEXTURE_BLUE_SIZE;
                    break;
                case GL_RENDERBUFFER_ALPHA_SIZE:
                    texPname = GL_TEXTURE_ALPHA_SIZE;
                    break;
                case GL_RENDERBUFFER_DEPTH_SIZE:
                    texPname = GL_TEXTURE_DEPTH_SIZE;
                    break;
                case GL_RENDERBUFFER_STENCIL_SIZE:
                default:
                    *params = 0; //XXX
                    return;
                    break;
            }

            GLint prevTex;
            ctx->dispatcher().glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D,
                                            rbData->eglImageGlobalTexObject->getGlobalName());
            ctx->dispatcher().glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                                                       texPname,
                                                       params);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, prevTex);
            return;
        }
    }

    ctx->dispatcher().glGetRenderbufferParameterivEXT(target,pname,params);
    if (pname == GL_RENDERBUFFER_INTERNAL_FORMAT && *params == GL_RGBA) {
        *params = GL_RGBA4;
    }
}


GL_APICALL void  GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::programParam(ctx, pname), GL_INVALID_ENUM);
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        switch(pname) {
        case GL_DELETE_STATUS:
            {
            auto objData = ctx->shareGroup()->getObjectData(
                    NamedObjectType::SHADER_OR_PROGRAM, program);
            SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
            SET_ERROR_IF(objData->getDataType() != PROGRAM_DATA,
                         GL_INVALID_OPERATION);
            ProgramData* programData = (ProgramData*)objData;
            params[0] = programData->getDeleteStatus() ? GL_TRUE : GL_FALSE;
            }
            break;
        case GL_LINK_STATUS:
            {
            auto objData = ctx->shareGroup()->getObjectData(
                    NamedObjectType::SHADER_OR_PROGRAM, program);
            SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
            SET_ERROR_IF(objData->getDataType() != PROGRAM_DATA,
                         GL_INVALID_OPERATION);
            ProgramData* programData = (ProgramData*)objData;
            params[0] = programData->getLinkStatus();
            }
            break;
        //validate status should not return GL_TRUE if link failed
        case GL_VALIDATE_STATUS:
            {
            auto objData = ctx->shareGroup()->getObjectData(
                    NamedObjectType::SHADER_OR_PROGRAM, program);
            SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
            SET_ERROR_IF(objData->getDataType() != PROGRAM_DATA,
                         GL_INVALID_OPERATION);
            ProgramData* programData = (ProgramData*)objData;
            if (programData->getLinkStatus() == GL_TRUE)
                ctx->dispatcher().glGetProgramiv(globalProgramName, pname,
                                                 params);
            else
                params[0] = GL_FALSE;
            }
            break;
        case GL_INFO_LOG_LENGTH:
            {
            auto objData = ctx->shareGroup()->getObjectData(
                    NamedObjectType::SHADER_OR_PROGRAM, program);
            SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
            SET_ERROR_IF(objData->getDataType() != PROGRAM_DATA,
                         GL_INVALID_OPERATION);
            ProgramData* programData = (ProgramData*)objData;
            GLint logLength = strlen(programData->getInfoLog());
            params[0] = (logLength > 0) ? logLength + 1 : 0;
            }
            break;
        default:
            ctx->dispatcher().glGetProgramiv(globalProgramName,pname,params);
        }
    }
}

GL_APICALL void  GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(!objData ,GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ProgramData* programData = (ProgramData*)objData;

        if (bufsize==0) {
            if (length) {
                *length = 0;
            }
            return;
        }

        GLsizei logLength;
        logLength = strlen(programData->getInfoLog());

        GLsizei returnLength=0;
        if (infolog) {
            returnLength = bufsize-1 < logLength ? bufsize-1 : logLength;
            strncpy(infolog,programData->getInfoLog(),returnLength+1);
            infolog[returnLength] = '\0';
        }
        if (length) {
            *length = returnLength;
        }
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType() != SHADER_DATA,
                     GL_INVALID_OPERATION);
        ShaderParser* sp = (ShaderParser*)objData;
        switch(pname) {
        case GL_DELETE_STATUS:
            {
            params[0]  = (sp->getDeleteStatus()) ? GL_TRUE : GL_FALSE;
            }
            break;
        case GL_INFO_LOG_LENGTH:
            {
            GLint logLength = strlen(sp->getInfoLog());
            params[0] = (logLength > 0) ? logLength + 1 : 0;
            }
            break;
        case GL_SHADER_SOURCE_LENGTH:
            {
            GLint srcLength = sp->getOriginalSrc().length();
            params[0] = (srcLength > 0) ? srcLength + 1 : 0;
            }
            break;
        default:
            ctx->dispatcher().glGetShaderiv(globalShaderName,pname,params);
        }
    }
}


GL_APICALL void  GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!objData ,GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType()!=SHADER_DATA,GL_INVALID_OPERATION);
        ShaderParser* sp = (ShaderParser*)objData;

        if (bufsize==0) {
            if (length) {
                *length = 0;
            }
            return;
        }

        GLsizei logLength;
        logLength = strlen(sp->getInfoLog());

        GLsizei returnLength=0;
        if (infolog) {
            returnLength = bufsize-1 <logLength ? bufsize-1 : logLength;
            strncpy(infolog,sp->getInfoLog(),returnLength+1);
            infolog[returnLength] = '\0';
        }
        if (length) {
            *length = returnLength;
        }
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::shaderType(ctx, shadertype) &&
                   GLESv2Validate::precisionType(precisiontype)),GL_INVALID_ENUM);

    switch (precisiontype) {
    case GL_LOW_INT:
    case GL_MEDIUM_INT:
        // range[0] = range[1] = 16;
        // *precision = 0;
        // break;
    case GL_HIGH_INT:
        range[0] = 31;
        range[1] = 30;
        *precision = 0;
        break;

    case GL_LOW_FLOAT:
    case GL_MEDIUM_FLOAT:
    case GL_HIGH_FLOAT:
        if(ctx->dispatcher().glGetShaderPrecisionFormat != NULL) {
            ctx->dispatcher().glGetShaderPrecisionFormat(shadertype,precisiontype,range,precision);
        } else {
            range[0] = range[1] = 127;
            *precision = 24;
        }
        break;
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName == 0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType() != SHADER_DATA,
                     GL_INVALID_OPERATION);
        const std::string& src =
                ((ShaderParser*)objData)->getOriginalSrc();
        int srcLength = static_cast<int>(src.size());

        int returnLength = bufsize < srcLength ? bufsize - 1 : srcLength;
        if (returnLength) {
            strncpy(source, src.c_str(), returnLength);
            source[returnLength] = '\0';
       }

       if (length)
          *length = returnLength;
    }
}


GL_APICALL const GLubyte* GL_APIENTRY glGetString(GLenum name){
    GET_CTX_V2_RET(NULL)
    static const GLubyte SHADING[] = "OpenGL ES GLSL ES 1.0.17";
    static const GLubyte SHADING30[] = "OpenGL ES GLSL ES 3.00";
    static const GLubyte SHADING31[] = "OpenGL ES GLSL ES 3.10";
    static const GLubyte SHADING32[] = "OpenGL ES GLSL ES 3.20";
    switch(name) {
        case GL_VENDOR:
            return (const GLubyte*)ctx->getVendorString();
        case GL_RENDERER:
            return (const GLubyte*)ctx->getRendererString();
        case GL_VERSION:
            return (const GLubyte*)ctx->getVersionString();
        case GL_SHADING_LANGUAGE_VERSION:
            switch (ctx->getMajorVersion()) {
            case 3:
                switch (ctx->getMinorVersion()) {
                case 0:
                    return SHADING30;
                case 1:
                    return SHADING31;
                case 2:
                    return SHADING32;
                default:
                    return SHADING31;
                }
            default:
                return SHADING;
             }
        case GL_EXTENSIONS:
            return (const GLubyte*)ctx->getExtensionString();
        default:
            RET_AND_SET_ERROR_IF(true,GL_INVALID_ENUM,NULL);
    }
}

GL_APICALL void  GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) &&
                   GLESv2Validate::textureParams(ctx, pname)),
                 GL_INVALID_ENUM);
    ctx->dispatcher().glGetTexParameterfv(target,pname,params);

}
GL_APICALL void  GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) &&
                   GLESv2Validate::textureParams(ctx, pname)),
                 GL_INVALID_ENUM);
    ctx->dispatcher().glGetTexParameteriv(target,pname,params);
}

GL_APICALL void  GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params){
    GET_CTX();
    SET_ERROR_IF(location < 0,GL_INVALID_OPERATION);
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ProgramData* pData = (ProgramData *)objData;
        SET_ERROR_IF(pData->getLinkStatus() != GL_TRUE,GL_INVALID_OPERATION);
        ctx->dispatcher().glGetUniformfv(globalProgramName,location,params);
    }
}

GL_APICALL void  GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params){
    GET_CTX();
    SET_ERROR_IF(location < 0,GL_INVALID_OPERATION);
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ProgramData* pData = (ProgramData *)objData;
        SET_ERROR_IF(pData->getLinkStatus() != GL_TRUE,GL_INVALID_OPERATION);
        ctx->dispatcher().glGetUniformiv(globalProgramName,location,params);
    }
}

GL_APICALL int GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar* name){
    GET_CTX_RET(-1);
    printf("glGetUniformLocation=%s\n", name);
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        RET_AND_SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE,-1);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        RET_AND_SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION,-1);
        ProgramData* pData = (ProgramData *)objData;
        RET_AND_SET_ERROR_IF(pData->getLinkStatus() != GL_TRUE,GL_INVALID_OPERATION,-1);
        return ctx->dispatcher().glGetUniformLocation(globalProgramName,name);
    }
    return -1;
}

static bool s_invalidVertexAttribIndex(GLuint index) {
    GLint param=0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &param);
    return (param < 0 || index >= (GLuint)param);
}

GL_APICALL void  GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params){
    GET_CTX_V2();
    SET_ERROR_IF(s_invalidVertexAttribIndex(index), GL_INVALID_VALUE);
    const GLESpointer* p = ctx->getPointer(index);
    if(p) {
        switch(pname){
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = 0;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = p->isEnable();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = p->getSize();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = p->getStride();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = p->getType();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = p->isNormalize();
            break;
        case GL_CURRENT_VERTEX_ATTRIB:
            if(index == 0)
            {
                const float* att0 = ctx->getAtt0();
                for(int i=0; i<4; i++)
                    params[i] = att0[i];
            }
            else
                ctx->dispatcher().glGetVertexAttribfv(index,pname,params);
            break;
        default:
            ctx->setGLerror(GL_INVALID_ENUM);
        }
    } else {
        ctx->setGLerror(GL_INVALID_VALUE);
    }
}

GL_APICALL void  GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params){
    GET_CTX_V2();
    SET_ERROR_IF(s_invalidVertexAttribIndex(index), GL_INVALID_VALUE);
    const GLESpointer* p = ctx->getPointer(index);
    if(p) {
        switch(pname){
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = 0;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = p->isEnable();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = p->getSize();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = p->getStride();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = p->getType();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = p->isNormalize();
            break;
        case GL_CURRENT_VERTEX_ATTRIB:
            if(index == 0)
            {
                const float* att0 = ctx->getAtt0();
                for(int i=0; i<4; i++)
                    params[i] = (GLint)att0[i];
            }
            else
                ctx->dispatcher().glGetVertexAttribiv(index,pname,params);
            break;
        default:
            ctx->setGLerror(GL_INVALID_ENUM);
        }
    } else {
        ctx->setGLerror(GL_INVALID_VALUE);
    }
}

GL_APICALL void  GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer){
    GET_CTX();
    SET_ERROR_IF(pname != GL_VERTEX_ATTRIB_ARRAY_POINTER,GL_INVALID_ENUM);
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);

    const GLESpointer* p = ctx->getPointer(index);
    if(p) {
        *pointer = const_cast<void *>( p->getBufferData());
    } else {
        ctx->setGLerror(GL_INVALID_VALUE);
    }
}

GL_APICALL void  GL_APIENTRY glHint(GLenum target, GLenum mode){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::hintTargetMode(target,mode),GL_INVALID_ENUM);
    ctx->dispatcher().glHint(target,mode);
}

GL_APICALL GLboolean    GL_APIENTRY glIsEnabled(GLenum cap){
    GET_CTX_RET(GL_FALSE);
    return ctx->dispatcher().glIsEnabled(cap);
}

GL_APICALL GLboolean    GL_APIENTRY glIsBuffer(GLuint buffer){
    GET_CTX_RET(GL_FALSE)
    if(buffer && ctx->shareGroup().get()) {
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::VERTEXBUFFER, buffer);
        return objData ? ((GLESbuffer*)objData)->wasBinded()
                             : GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsFramebuffer(GLuint framebuffer){
    GET_CTX_RET(GL_FALSE)
    if(framebuffer){
        if (!ctx->isFBO(framebuffer))
            return GL_FALSE;
        auto fbObj = ctx->getFBOData(framebuffer);
        if (!fbObj) return GL_FALSE;
        return fbObj->hasBeenBoundAtLeastOnce() ? GL_TRUE : GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer){
    GET_CTX_RET(GL_FALSE)
    if(renderbuffer && ctx->shareGroup().get()){
        return (ctx->shareGroup()->isObject(NamedObjectType::RENDERBUFFER,
                                            renderbuffer) &&
                ctx->getRenderbufferBinding() == renderbuffer)
                       ? GL_TRUE
                       : GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsTexture(GLuint texture){
    GET_CTX_RET(GL_FALSE)
    if (texture==0)
        return GL_FALSE;
    TextureData* tex = getTextureData(texture);
    return tex ? tex->wasBound : GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsProgram(GLuint program){
    GET_CTX_RET(GL_FALSE)
    if (program && ctx->shareGroup().get() &&
        ctx->shareGroup()->isObject(NamedObjectType::SHADER_OR_PROGRAM, program)) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        return ctx->dispatcher().glIsProgram(globalProgramName);
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsShader(GLuint shader){
    GET_CTX_RET(GL_FALSE)
    if (shader && ctx->shareGroup().get() &&
        ctx->shareGroup()->isObject(NamedObjectType::SHADER_OR_PROGRAM, shader)) {
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        return ctx->dispatcher().glIsShader(globalShaderName);
    }
    return GL_FALSE;
}

GL_APICALL void  GL_APIENTRY glLineWidth(GLfloat width){
    GET_CTX();
    ctx->setLineWidth(width);
    ctx->dispatcher().glLineWidth(width);
}

GL_APICALL void  GL_APIENTRY glLinkProgram(GLuint program){
    GET_CTX_V2();
    GLint linkStatus = GL_FALSE;
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);

        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA, GL_INVALID_OPERATION);

        ProgramData* programData = (ProgramData*)objData;
        GLint fragmentShader   = programData->getAttachedFragmentShader();
        GLint vertexShader =  programData->getAttachedVertexShader();

        if (ctx->getMajorVersion() >= 3 && ctx->getMinorVersion() >= 1) {
            ctx->dispatcher().glLinkProgram(globalProgramName);
            ctx->dispatcher().glGetProgramiv(globalProgramName,GL_LINK_STATUS,&linkStatus);
        } else {
            if (vertexShader != 0 && fragmentShader!=0) {
                auto fragObjData = ctx->shareGroup()->getObjectData(
                        NamedObjectType::SHADER_OR_PROGRAM, fragmentShader);
                auto vertObjData = ctx->shareGroup()->getObjectData(
                        NamedObjectType::SHADER_OR_PROGRAM, vertexShader);
                ShaderParser* fragSp = (ShaderParser*)fragObjData;
                ShaderParser* vertSp = (ShaderParser*)vertObjData;

                if(fragSp->getCompileStatus() && vertSp->getCompileStatus()) {
                    if (programData->validateLink(fragSp, vertSp)) {
                        ctx->dispatcher().glLinkProgram(globalProgramName);
                        ctx->dispatcher().glGetProgramiv(globalProgramName,GL_LINK_STATUS,&linkStatus);
                    } else {
                        programData->setLinkStatus(GL_FALSE);
                        programData->setErrInfoLog();
                        return;
                    }
                }
            }
        }

        programData->setLinkStatus(linkStatus);

        GLsizei infoLogLength=0;
        GLchar* infoLog;
        ctx->dispatcher().glGetProgramiv(globalProgramName,GL_INFO_LOG_LENGTH,&infoLogLength);
        infoLog = new GLchar[infoLogLength+1];
        ctx->dispatcher().glGetProgramInfoLog(globalProgramName,infoLogLength,NULL,infoLog);
        programData->setInfoLog(infoLog);
    }
}

GL_APICALL void  GL_APIENTRY glPixelStorei(GLenum pname, GLint param){
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::pixelStoreParam(ctx, pname), GL_INVALID_ENUM);
    switch (pname) {
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
        SET_ERROR_IF(!((param==1)||(param==2)||(param==4)||(param==8)), GL_INVALID_VALUE);
        break;
    default:
        SET_ERROR_IF(param < 0, GL_INVALID_VALUE);
        break;
    }
    ctx->setPixelStorei(pname, param);
    ctx->dispatcher().glPixelStorei(pname,param);
}

GL_APICALL void  GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units){
    GET_CTX();
    ctx->setPolygonOffset(factor, units);
    ctx->dispatcher().glPolygonOffset(factor,units);
}

GL_APICALL void  GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::pixelOp(format,type)),GL_INVALID_OPERATION);
    SET_ERROR_IF(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE, GL_INVALID_FRAMEBUFFER_OPERATION);

    if (ctx->isDefaultFBOBound(GL_READ_FRAMEBUFFER) &&
        ctx->getDefaultFBOMultisamples()) {

        GLint prev_bound_rbo;
        GLint prev_bound_draw_fbo;

        glGetIntegerv(GL_RENDERBUFFER_BINDING, &prev_bound_rbo);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_bound_draw_fbo);

        GLuint resolve_fbo;
        GLuint resolve_rbo;
        glGenFramebuffers(1, &resolve_fbo);
        glGenRenderbuffers(1, &resolve_rbo);

        int fboFormat = ctx->getDefaultFBOColorFormat();
        int fboWidth = ctx->getDefaultFBOWidth();
        int fboHeight = ctx->getDefaultFBOHeight();

        glBindRenderbuffer(GL_RENDERBUFFER, resolve_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, fboFormat, fboWidth, fboHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resolve_rbo);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo);

        bool scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

        if (scissorEnabled) glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0, 0, fboWidth, fboHeight, 0, 0, fboWidth, fboHeight,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        if (scissorEnabled) glEnable(GL_SCISSOR_TEST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, resolve_fbo);

        ctx->dispatcher().glReadPixels(x,y,width,height,format,type,pixels);

        glDeleteRenderbuffers(1, &resolve_rbo);
        glDeleteFramebuffers(1, &resolve_fbo);

        glBindRenderbuffer(GL_RENDERBUFFER, prev_bound_rbo);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_bound_draw_fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    } else {
        ctx->dispatcher().glReadPixels(x,y,width,height,format,type,pixels);
    }
}


GL_APICALL void  GL_APIENTRY glReleaseShaderCompiler(void){
// this function doesn't work on Mac OS with MacOSX10.9sdk
#ifndef __APPLE__

    /* Use this function with mesa will cause potential bug. Specifically,
     * calling this function between glCompileShader() and glLinkProgram() will
     * release resources that would be potentially used by glLinkProgram,
     * resulting in a segmentation fault.
     */
    const char* env = ::getenv("ANDROID_GL_LIB");
    if (env && !strcmp(env, "mesa")) {
        return;
    }

    GET_CTX();

    if(ctx->dispatcher().glReleaseShaderCompiler != NULL)
    {
        ctx->dispatcher().glReleaseShaderCompiler();
    }
#endif // !__APPLE__
}

static GLenum sPrepareRenderbufferStorage(GLenum internalformat, GLsizei width,
        GLsizei height, GLint* err) {
    GET_CTX_V2_RET(GL_NONE);
    GLenum internal = internalformat;
    if (ctx->getMajorVersion() < 3) {
        switch (internalformat) {
            case GL_RGB565:
                internal = GL_RGB;
                break;
            case GL_RGB5_A1:
                internal = GL_RGBA;
                break;
            default:
                break;
        }
    }


    // Get current bounded renderbuffer
    // raise INVALID_OPERATIOn if no renderbuffer is bounded
    GLuint rb = ctx->getRenderbufferBinding();
    if (!rb) { *err = GL_INVALID_OPERATION; return GL_NONE; }
    auto objData = ctx->shareGroup()->getObjectData(NamedObjectType::RENDERBUFFER, rb);
    RenderbufferData *rbData = (RenderbufferData *)objData;
    if (!rbData) { *err = GL_INVALID_OPERATION; return GL_NONE; }

    rbData->internalformat = internalformat;
    rbData->hostInternalFormat = internal;
    rbData->width = width;
    rbData->height = height;

    //
    // if the renderbuffer was an eglImage target, release
    // its underlying texture.
    //
    rbData->eglImageGlobalTexObject.reset();

    *err = GL_NO_ERROR;

    return internal;
}

GL_APICALL void  GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height){
    GET_CTX();
    GLint err = GL_NO_ERROR;
    internalformat = sPrepareRenderbufferStorage(internalformat, width, height,
            &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    ctx->dispatcher().glRenderbufferStorageEXT(target,internalformat,width,height);
}

GL_APICALL void  GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert){
    GET_CTX();
    ctx->setSampleCoverage(value, invert);
    ctx->dispatcher().glSampleCoverage(value,invert);
}

GL_APICALL void  GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height){
    GET_CTX();
    ctx->setScissor(x, y, width, height);
    ctx->dispatcher().glScissor(x,y,width,height);
}

GL_APICALL void  GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length){
    GET_CTX();

    SET_ERROR_IF( (ctx->dispatcher().glShaderBinary == NULL), GL_INVALID_OPERATION);

    if(ctx->shareGroup().get()){
        for(int i=0; i < n ; i++){
            const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                    NamedObjectType::SHADER_OR_PROGRAM, shaders[i]);
            SET_ERROR_IF(globalShaderName == 0,GL_INVALID_VALUE);
            ctx->dispatcher().glShaderBinary(1,&globalShaderName,binaryformat,binary,length);
        }
    }
}

static int sDetectShaderESSLVersion(GLESv2Context* ctx, const GLchar* const* strings) {
    // Just look at the first line of the first string for now
    const char* pos = ((const char* const*)strings)[0];
    const char* linePos = strstr(pos, "\n");
    const char* versionPos = strstr(pos, "#version");
    if (!linePos || !versionPos) {
        // default to ESSL 100
        return 100;
    }

    const char* version_end = versionPos + strlen("#version");
    int wantedESSLVersion;
    sscanf(version_end, " %d", &wantedESSLVersion);
    return wantedESSLVersion;
}

GL_APICALL void  GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length){
    GET_CTX_V2();
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()){
        const GLuint globalShaderName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(globalShaderName == 0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, shader);
        SET_ERROR_IF(!objData, GL_INVALID_OPERATION);
        SET_ERROR_IF(objData->getDataType() != SHADER_DATA,
                     GL_INVALID_OPERATION);
        ShaderParser* sp = (ShaderParser*)objData;
        int esslVersion = sDetectShaderESSLVersion(ctx, string);
        sp->setSrc(esslVersion, count, string, length);
        ctx->dispatcher().glShaderSource(globalShaderName, 1, sp->parsedLines(),
                                         NULL);
    }
}

GL_APICALL void  GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask){
    GET_CTX();
    ctx->setStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
    ctx->dispatcher().glStencilFunc(func,ref,mask);
}
GL_APICALL void  GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask){
    GET_CTX();
    ctx->setStencilFuncSeparate(face, func, ref, mask);
    ctx->dispatcher().glStencilFuncSeparate(face,func,ref,mask);
}
GL_APICALL void  GL_APIENTRY glStencilMask(GLuint mask){
    GET_CTX();
    ctx->setStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
    ctx->dispatcher().glStencilMask(mask);
}

GL_APICALL void  GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask){
    GET_CTX();
    ctx->setStencilMaskSeparate(face, mask);
    ctx->dispatcher().glStencilMaskSeparate(face,mask);
}

GL_APICALL void  GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass){
    GET_CTX();
    ctx->setStencilOpSeparate(GL_FRONT_AND_BACK, fail, zfail, zpass);
    ctx->dispatcher().glStencilOp(fail,zfail,zpass);
}

GL_APICALL void  GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass){
    GET_CTX();
    switch (face) {
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
            break;
        default:
            SET_ERROR_IF(1, GL_INVALID_ENUM);
    }
    ctx->setStencilOpSeparate(face, fail, zfail, zpass);
    ctx->dispatcher().glStencilOpSeparate(face, fail,zfail,zpass);
}

#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815

static void sPrepareTexImage2D(GLenum target, GLsizei level, GLint internalformat,
                               GLsizei width, GLsizei height, GLint border,
                               GLenum format, GLenum type, const GLvoid* pixels,
                               GLenum* type_out,
                               GLint* internalformat_out,
                               GLint* err_out) {
    GET_CTX_V2();

#define VALIDATE(cond, err) do { if (cond) { *err_out = err; fprintf(stderr, "%s:%d failed validation\n", __FUNCTION__, __LINE__); return; } } while(0) \

    bool isCompressedFormat =
        GLESv2Validate::isCompressedFormat(internalformat);

    if (!isCompressedFormat) {
        VALIDATE(!(GLESv2Validate::textureTarget(ctx, target) ||
                   GLESv2Validate::textureTargetEx(ctx, target)), GL_INVALID_ENUM);
        VALIDATE(!GLESv2Validate::pixelFrmt(ctx, format), GL_INVALID_ENUM);
        VALIDATE(!GLESv2Validate::pixelType(ctx, type), GL_INVALID_ENUM);

        VALIDATE(!GLESv2Validate::pixelItnlFrmt(ctx,internalformat), GL_INVALID_VALUE);
        VALIDATE((GLESv2Validate::textureIsCubeMap(target) && width != height), GL_INVALID_VALUE);
        VALIDATE(ctx->getMajorVersion() < 3 &&
                (format == GL_DEPTH_COMPONENT || internalformat == GL_DEPTH_COMPONENT) &&
                (type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_OPERATION);

        VALIDATE(ctx->getMajorVersion() < 3 &&
                (type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT) &&
                (format != GL_DEPTH_COMPONENT || internalformat != GL_DEPTH_COMPONENT), GL_INVALID_OPERATION);

        VALIDATE(!GLESv2Validate::pixelOp(format,type) && internalformat == ((GLint)format),GL_INVALID_OPERATION);
        VALIDATE(!GLESv2Validate::pixelSizedFrmt(ctx, internalformat, format, type), GL_INVALID_OPERATION);
    }

    VALIDATE(border != 0,GL_INVALID_VALUE);

    s_glInitTexImage2D(target, level, internalformat, width, height, border,
            &format, &type, &internalformat);

    if (!isCompressedFormat && ctx->getMajorVersion() < 3) {
        if (type==GL_HALF_FLOAT_OES)
            type = GL_HALF_FLOAT_NV;
        if (pixels==NULL && type==GL_UNSIGNED_SHORT_5_5_5_1)
            type = GL_UNSIGNED_SHORT;
        if (type == GL_FLOAT)
            internalformat = (format == GL_RGBA) ? GL_RGBA32F : GL_RGB32F;
    }

    *type_out = type;
    *internalformat_out = internalformat;
    *err_out = GL_NO_ERROR;
}

GL_APICALL void  GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels){
    GET_CTX_V2();
    // clear previous error
    GLint err = ctx->dispatcher().glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "%s: got err pre :( 0x%x internal 0x%x format 0x%x type 0x%x\n", __func__, err, internalformat, format, type);
    }
    sPrepareTexImage2D(target, level, internalformat, width, height, border, format, type, pixels, &type, &internalformat, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    ctx->dispatcher().glTexImage2D(target,level,internalformat,width,height,border,format,type,pixels);
    err = ctx->dispatcher().glGetError();
    if (err != GL_NO_ERROR) {
    	// printf("target=%x,level=%x,internalformat=%x,width=%x,height=%x,border=%x,format=%x,type=%x\n", target,level,internalformat,width,height,border,format,type);
        fprintf(stderr, "%s: got err :( 0x%x internal 0x%x format 0x%x type 0x%x\n", __func__, err, internalformat, format, type);
    }
}

GL_APICALL void  GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) &&
                   GLESv2Validate::textureParams(ctx, pname)),
                 GL_INVALID_ENUM);
    ctx->dispatcher().glTexParameterf(target,pname,param);
}
GL_APICALL void  GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) &&
                   GLESv2Validate::textureParams(ctx, pname)),
                 GL_INVALID_ENUM);
    ctx->dispatcher().glTexParameterfv(target,pname,params);
}
GL_APICALL void  GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) &&
                   GLESv2Validate::textureParams(ctx, pname)),
                 GL_INVALID_ENUM);
    TextureData *texData = getTextureTargetData(target);
    if (texData) {
        texData->setTexParam(pname, param);
    }
    ctx->dispatcher().glTexParameteri(target,pname,param);
}
GL_APICALL void  GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) &&
                   GLESv2Validate::textureParams(ctx, pname)),
                 GL_INVALID_ENUM);
    TextureData *texData = getTextureTargetData(target);
    if (texData) {
        texData->setTexParam(pname, params[0]);
    }
    ctx->dispatcher().glTexParameteriv(target,pname,params);
}

GL_APICALL void  GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(ctx, target) ||
                   GLESv2Validate::textureTargetEx(ctx, target)), GL_INVALID_ENUM);
    SET_ERROR_IF(!GLESv2Validate::pixelFrmt(ctx,format), GL_INVALID_ENUM);
    SET_ERROR_IF(!GLESv2Validate::pixelType(ctx,type),GL_INVALID_ENUM);
    // set an error if level < 0 or level > log 2 max
    SET_ERROR_IF(level < 0 || 1<<level > ctx->getMaxTexSize(), GL_INVALID_VALUE);
    SET_ERROR_IF(xoffset < 0 || yoffset < 0 || width < 0 || height < 0, GL_INVALID_VALUE);
    if (ctx->shareGroup().get()) {
        TextureData *texData = getTextureTargetData(target);
        if (texData) {
            SET_ERROR_IF(xoffset + width > (GLint)texData->width ||
                     yoffset + height > (GLint)texData->height,
                     GL_INVALID_VALUE);
        }
    }
    SET_ERROR_IF(!(GLESv2Validate::pixelFrmt(ctx,format) &&
                   GLESv2Validate::pixelType(ctx,type)),GL_INVALID_ENUM);
    SET_ERROR_IF(!GLESv2Validate::pixelOp(format,type),GL_INVALID_OPERATION);
    SET_ERROR_IF(!pixels && !ctx->isBindedBuffer(GL_PIXEL_UNPACK_BUFFER),GL_INVALID_OPERATION);
    if (type==GL_HALF_FLOAT_OES)
        type = GL_HALF_FLOAT_NV;

    ctx->dispatcher().glTexSubImage2D(target,level,xoffset,yoffset,width,height,format,type,pixels);
}

GL_APICALL void  GL_APIENTRY glUniform1f(GLint location, GLfloat x){
    GET_CTX();
    ctx->dispatcher().glUniform1f(location,x);
}

GL_APICALL void  GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform1fv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform1i(GLint location, GLint x){
    GET_CTX();
    ctx->dispatcher().glUniform1i(location,x);
}

GL_APICALL void  GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform1iv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y){
    GET_CTX();
    ctx->dispatcher().glUniform2f(location,x,y);
}

GL_APICALL void  GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform2fv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform2i(GLint location, GLint x, GLint y){
    GET_CTX();
    ctx->dispatcher().glUniform2i(location,x,y);
}

GL_APICALL void  GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform2iv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z){
    GET_CTX();
    ctx->dispatcher().glUniform3f(location,x,y,z);
}

GL_APICALL void  GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform3fv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z){
    GET_CTX();
    ctx->dispatcher().glUniform3i(location,x,y,z);
}

GL_APICALL void  GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform3iv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w){
    GET_CTX();
    ctx->dispatcher().glUniform4f(location,x,y,z,w);
}

GL_APICALL void  GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform4fv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w){
    GET_CTX();
    ctx->dispatcher().glUniform4i(location,x,y,z,w);
}

GL_APICALL void  GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform4iv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){
    GET_CTX_V2();
    SET_ERROR_IF(ctx->getMajorVersion() < 3 &&
                 transpose != GL_FALSE,GL_INVALID_VALUE);
    ctx->dispatcher().glUniformMatrix2fv(location,count,transpose,value);
}

GL_APICALL void  GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){
    GET_CTX_V2();
    SET_ERROR_IF(ctx->getMajorVersion() < 3 &&
                 transpose != GL_FALSE,GL_INVALID_VALUE);
    ctx->dispatcher().glUniformMatrix3fv(location,count,transpose,value);
}

GL_APICALL void  GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){
    GET_CTX_V2();
    SET_ERROR_IF(ctx->getMajorVersion() < 3 &&
                 transpose != GL_FALSE,GL_INVALID_VALUE);
    ctx->dispatcher().glUniformMatrix4fv(location,count,transpose,value);
}

static void s_unUseCurrentProgram() {
    GET_CTX();
    GLint localCurrentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &localCurrentProgram);
    if (!localCurrentProgram) return;

    auto objData = ctx->shareGroup()->getObjectData(
            NamedObjectType::SHADER_OR_PROGRAM, localCurrentProgram);
    SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
    ProgramData* programData = (ProgramData*)objData;
    programData->setInUse(false);
    if (programData->getDeleteStatus()) {
        s_detachShader(ctx, localCurrentProgram,
                programData->getAttachedVertexShader());
        s_detachShader(ctx, localCurrentProgram,
                programData->getAttachedFragmentShader());
        s_detachShader(ctx, localCurrentProgram,
                programData->getAttachedComputeShader());
        ctx->shareGroup()->deleteName(NamedObjectType::SHADER_OR_PROGRAM,
                                      localCurrentProgram);
    }
}

GL_APICALL void  GL_APIENTRY glUseProgram(GLuint program){
    GET_CTX_V2();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(program!=0 && globalProgramName==0,GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectDataPtr(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData && (objData->getDataType()!=PROGRAM_DATA),GL_INVALID_OPERATION);

        s_unUseCurrentProgram();

        ProgramData* programData = (ProgramData*)objData.get();
        if (programData) programData->setInUse(true);

        ctx->setUseProgram(program, objData);
        ctx->dispatcher().glUseProgram(globalProgramName);
    }
}

GL_APICALL void  GL_APIENTRY glValidateProgram(GLuint program){
    GET_CTX();
    if(ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ProgramData* programData = (ProgramData*)objData;
        ctx->dispatcher().glValidateProgram(globalProgramName);

        GLsizei infoLogLength=0;
        GLchar* infoLog;
        ctx->dispatcher().glGetProgramiv(globalProgramName,GL_INFO_LOG_LENGTH,&infoLogLength);
        infoLog = new GLchar[infoLogLength+1];
        ctx->dispatcher().glGetProgramInfoLog(globalProgramName,infoLogLength,NULL,infoLog);
        programData->setInfoLog(infoLog);
    }
}

GL_APICALL void  GL_APIENTRY glVertexAttrib1f(GLuint index, GLfloat x){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib1f(index,x);
    ctx->setAttribValue(index, 1, &x);
    if(index == 0)
        ctx->setAttribute0value(x, 0.0, 0.0, 1.0);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib1fv(GLuint index, const GLfloat* values){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib1fv(index,values);
    ctx->setAttribValue(index, 1, values);
    if(index == 0)
        ctx->setAttribute0value(values[0], 0.0, 0.0, 1.0);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib2f(index,x,y);
    GLfloat values[] = {x, y};
    ctx->setAttribValue(index, 2, values);
    if(index == 0)
        ctx->setAttribute0value(x, y, 0.0, 1.0);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib2fv(GLuint index, const GLfloat* values){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib2fv(index,values);
    ctx->setAttribValue(index, 2, values);
    if(index == 0)
        ctx->setAttribute0value(values[0], values[1], 0.0, 1.0);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib3f(index,x,y,z);
    GLfloat values[3] = {x, y, z};
    ctx->setAttribValue(index, 3, values);
    if(index == 0)
        ctx->setAttribute0value(x, y, z, 1.0);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib3fv(GLuint index, const GLfloat* values){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib3fv(index,values);
    ctx->setAttribValue(index, 3, values);
    if(index == 0)
        ctx->setAttribute0value(values[0], values[1], values[2], 1.0);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib4f(index,x,y,z,w);
    GLfloat values[4] = {x, y, z, z};
    ctx->setAttribValue(index, 4, values);
    if(index == 0)
        ctx->setAttribute0value(x, y, z, w);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib4fv(GLuint index, const GLfloat* values){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->dispatcher().glVertexAttrib4fv(index,values);
    ctx->setAttribValue(index, 4, values);
    if(index == 0)
        ctx->setAttribute0value(values[0], values[1], values[2], values[3]);
}

static void s_glPrepareVertexAttribPointer(GLESv2Context* ctx, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr, GLsizei dataSize, bool isInt) {
    ctx->setVertexAttribBindingIndex(index, index);
    GLsizei effectiveStride = stride;
    if (stride == 0) {
        effectiveStride = GLESv2Validate::sizeOfType(type) * size; 
        switch (type) {
            case GL_INT_2_10_10_10_REV:
            case GL_UNSIGNED_INT_2_10_10_10_REV:
                effectiveStride /= 4;
                break;
            default:
                break;
        }
    }
    ctx->bindIndexedBuffer(0, index, ctx->getBuffer(GL_ARRAY_BUFFER), (GLintptr)ptr, 0, effectiveStride);
    ctx->setVertexAttribFormat(index, size, type, normalized, 0, isInt);
    // Still needed to deal with client arrays
    ctx->setPointer(index, size, type, stride, ptr, dataSize, normalized, isInt);
}

GL_APICALL void  GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr){
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    if (type == GL_HALF_FLOAT_OES) type = GL_HALF_FLOAT;

    s_glPrepareVertexAttribPointer(ctx, index, size, type, normalized, stride, ptr, 0, false);
    if (ctx->isBindedBuffer(GL_ARRAY_BUFFER)) {
        ctx->dispatcher().glVertexAttribPointer(index, size, type, normalized, stride, ptr);
    }
}

GL_APICALL void  GL_APIENTRY glVertexAttribPointerWithDataSize(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr, GLsizei dataSize) {
    (void)dataSize;
    GET_CTX_V2();
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    if (type == GL_HALF_FLOAT_OES) type = GL_HALF_FLOAT;

    s_glPrepareVertexAttribPointer(ctx, index, size, type, normalized, stride, ptr, dataSize, false);
    if (ctx->isBindedBuffer(GL_ARRAY_BUFFER)) {
        ctx->dispatcher().glVertexAttribPointer(index, size, type, normalized, stride, ptr);
    }
}

GL_APICALL void  GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height){
    GET_CTX();
    ctx->setViewport(x, y, width, height);
    ctx->dispatcher().glViewport(x,y,width,height);
}

GL_APICALL void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::textureTargetLimited(target),GL_INVALID_ENUM);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = s_eglIface->getEGLImage(imagehndl);
    if (img) {
        // Create the texture object in the underlying EGL implementation,
        // flag to the OpenGL layer to skip the image creation and map the
        // current binded texture object to the existing global object.
        if (ctx->shareGroup().get()) {
            ObjectLocalName tex = ctx->getTextureLocalName(target,ctx->getBindedTexture(target));
            // replace mapping and bind the new global object
            ctx->shareGroup()->replaceGlobalObject(NamedObjectType::TEXTURE, tex,
                                                   img->globalTexObj);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, img->globalTexObj->getGlobalName());
            TextureData *texData = getTextureTargetData(target);
            SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
            texData->width = img->width;
            texData->height = img->height;
            texData->border = img->border;
            texData->internalFormat = img->internalFormat;
            texData->format = img->format;
            texData->type = img->type;
            texData->sourceEGLImage = imagehndl;
            texData->globalName = img->globalTexObj->getGlobalName();
            // TODO: set up texData->type
            if (!imagehndl) {
                fprintf(stderr, "glEGLImageTargetTexture2DOES with empty handle\n");
            }
        }
    }
}

GL_APICALL void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
    GET_CTX();
    SET_ERROR_IF(target != GL_RENDERBUFFER_OES,GL_INVALID_ENUM);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = s_eglIface->getEGLImage(imagehndl);
    SET_ERROR_IF(!img,GL_INVALID_VALUE);
    SET_ERROR_IF(!ctx->shareGroup().get(),GL_INVALID_OPERATION);

    // Get current bounded renderbuffer
    // raise INVALID_OPERATIOn if no renderbuffer is bounded
    GLuint rb = ctx->getRenderbufferBinding();
    SET_ERROR_IF(rb == 0,GL_INVALID_OPERATION);
    auto objData =
            ctx->shareGroup()->getObjectData(NamedObjectType::RENDERBUFFER, rb);
    RenderbufferData *rbData = (RenderbufferData *)objData;
    SET_ERROR_IF(!rbData,GL_INVALID_OPERATION);

    //
    // acquire the texture in the renderbufferData that it is an eglImage target
    //
    rbData->eglImageGlobalTexObject = img->globalTexObj;

    //
    // if the renderbuffer is attached to a framebuffer
    // change the framebuffer attachment in the undelying OpenGL
    // to point to the eglImage texture object.
    //
    if (rbData->attachedFB) {
        // update the framebuffer attachment point to the
        // underlying texture of the img
        GLuint prevFB = ctx->getFramebufferBinding(GL_FRAMEBUFFER_EXT);
        if (prevFB != rbData->attachedFB) {
            ctx->dispatcher().glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                                                   rbData->attachedFB);
        }
        ctx->dispatcher().glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                                    rbData->attachedPoint,
                                                    GL_TEXTURE_2D,
                                                    img->globalTexObj->getGlobalName(),
                                                    0);
        if (prevFB != rbData->attachedFB) {
            ctx->dispatcher().glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                                                   prevFB);
        }
    }
}

// Extension: Vertex array objects
GL_APICALL void GL_APIENTRY glGenVertexArraysOES(GLsizei n, GLuint* arrays) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);
    for (GLsizei i = 0; i < n; i++) {
        arrays[i] = ctx->genVAOName(0, true);
    }
    ctx->addVertexArrayObjects(n, arrays);
}

GL_APICALL void GL_APIENTRY glBindVertexArrayOES(GLuint array) {
    GET_CTX_V2();
    if (ctx->setVertexArrayObject(array)) {
        // TODO: This could be useful for a glIsVertexArray
        // that doesn't use the host GPU, but currently, it doesn't
        // really work. VAOs need to be bound first if glIsVertexArray
        // is to return true, and for now let's just ask the GPU
        // directly.
        ctx->setVAOEverBound();
    }
    ctx->dispatcher().glBindVertexArray(ctx->getVAOGlobalName(array));
}

GL_APICALL void GL_APIENTRY glDeleteVertexArraysOES(GLsizei n, const GLuint * arrays) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);
    ctx->removeVertexArrayObjects(n, arrays);
    for (GLsizei i = 0; i < n; i++) {
        ctx->deleteVAO(arrays[i]);
    }
}

GL_APICALL GLboolean GL_APIENTRY glIsVertexArrayOES(GLuint array) {
    GET_CTX_V2_RET(0);
    // TODO: Figure out how to answer this completely in software.
    // Currently, state gets weird so we need to ask the GPU directly.
    return ctx->dispatcher().glIsVertexArray(ctx->getVAOGlobalName(array));
}

#include "GLESv30Imp.cpp"
#include "GLESv31Imp.cpp"
