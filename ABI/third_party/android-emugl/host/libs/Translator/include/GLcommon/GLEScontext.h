/*
* Copyright 2011 The Android Open Source Project
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

#ifndef GLES_CONTEXT_H
#define GLES_CONTEXT_H

#include "android/base/files/Stream.h"
#include "emugl/common/mutex.h"
#include "GLDispatch.h"
#include "GLESpointer.h"
#include "ObjectNameSpace.h"
#include "ShareGroup.h"

#include <string>
#include <unordered_map>
#include <vector>

typedef std::unordered_map<GLenum,GLESpointer*>  ArraysMap;

enum TextureTarget {
TEXTURE_2D,
TEXTURE_CUBE_MAP,
TEXTURE_2D_ARRAY,
TEXTURE_3D,
TEXTURE_2D_MULTISAMPLE,
NUM_TEXTURE_TARGETS
};

typedef struct _textureTargetState {
    GLuint texture;
    GLboolean enabled;
} textureTargetState;

typedef textureTargetState textureUnitState[NUM_TEXTURE_TARGETS];

class Version{
public:
    explicit Version(int major = 0,int minor = 0,int release = 0);
    Version(const char* versionString);
    Version(const Version& ver);
    bool operator<(const Version& ver) const;
    Version& operator=(const Version& ver);
private:
    int m_major;
    int m_minor;
    int m_release;
};

struct GLSupport {
    int  maxLights = 0;
    int  maxVertexAttribs = 0;
    int  maxClipPlane = 0;
    int  maxTexUnits = 0;
    int  maxTexImageUnits = 0;
    int  maxTexSize = 0;
    int  maxCombinedTexImageUnits = 0;

    int  maxTransformFeedbackSeparateAttribs = 0;
    int  maxUniformBufferBindings = 0;
    int  maxAtomicCounterBufferBindings = 0;
    int  maxShaderStorageBufferBindings = 0;
    int  maxVertexAttribBindings = 0;

    int maxDrawBuffers = 1;

    Version glslVersion;
    bool GL_EXT_TEXTURE_FORMAT_BGRA8888 = false;
    bool GL_EXT_FRAMEBUFFER_OBJECT = false;
    bool GL_ARB_VERTEX_BLEND = false;
    bool GL_ARB_MATRIX_PALETTE = false;
    bool GL_EXT_PACKED_DEPTH_STENCIL = false;
    bool GL_OES_READ_FORMAT = false;
    bool GL_ARB_HALF_FLOAT_PIXEL = false;
    bool GL_NV_HALF_FLOAT = false;
    bool GL_ARB_HALF_FLOAT_VERTEX = false;
    bool GL_SGIS_GENERATE_MIPMAP = false;
    bool GL_ARB_ES2_COMPATIBILITY = false;
    bool GL_OES_STANDARD_DERIVATIVES = false;
    bool GL_OES_TEXTURE_NPOT = false;
    bool GL_OES_RGB8_RGBA8 = false;

    bool GL_EXT_color_buffer_float = false;
};

struct ArrayData {
    void*        data = nullptr;
    GLenum       type = 0;
    unsigned int stride = 0;
    bool         allocated = false;
};

struct BufferBinding {
    GLuint buffer = 0;
    GLintptr offset = 0;
    GLsizeiptr size = 0;
    GLintptr stride = 0;
    GLuint divisor = 0;
    void onLoad(android::base::Stream* stream);
    void onSave(android::base::Stream* stream) const;
};

typedef std::vector<BufferBinding> VertexAttribBindingVector;

struct VAOState {
    VAOState() : VAOState(0, NULL, 0) { }
    VAOState(GLuint ibo, ArraysMap* arr, int numVertexAttribBindings) :
        element_array_buffer_binding(ibo),
        arraysMap(arr),
        bindingState(numVertexAttribBindings),
        everBound(false) { }
    VAOState(android::base::Stream* stream);
    GLuint element_array_buffer_binding;
    ArraysMap* arraysMap;
    VertexAttribBindingVector bindingState;
    bool bufferBacked;
    bool everBound;
    void onSave(android::base::Stream* stream) const;
};

typedef std::unordered_map<GLuint, VAOState> VAOStateMap;

struct VAOStateRef {
    VAOStateRef() { }
    VAOStateRef(VAOStateMap::iterator iter) : it(iter) { }
    GLuint vaoId() const { return it->first; }
    GLuint& iboId() { return it->second.element_array_buffer_binding; }

    ArraysMap::iterator begin() {
        return it->second.arraysMap->begin();
    }
    ArraysMap::iterator end() {
        return it->second.arraysMap->end();
    }
    ArraysMap::iterator find(GLenum arrType) {
        return it->second.arraysMap->find(arrType);
    }
    GLESpointer*& operator[](size_t k) {
        ArraysMap* map = it->second.arraysMap;
        return (*map)[k];
    }
    VertexAttribBindingVector& bufferBindings() {
        return it->second.bindingState;
    }
    void setEverBound() {
        it->second.everBound = true;
    }
    bool isEverBound() {
        return it->second.everBound;
    }
    VAOStateMap::iterator it;
};

class FramebufferData;

class GLESConversionArrays
{
public:
    void setArr(void* data,unsigned int stride,GLenum type);
    void allocArr(unsigned int size,GLenum type);
    ArrayData& operator[](int i);
    void* getCurrentData();
    ArrayData& getCurrentArray();
    unsigned int getCurrentIndex();
    void operator++();

    ~GLESConversionArrays();
private:
    std::unordered_map<GLenum,ArrayData> m_arrays;
    unsigned int m_current = 0;
};


class GLEScontext{
public:
    GLEScontext();
    GLEScontext(GlobalNameSpace* globalNameSpace, android::base::Stream* stream,
            GlLibrary* glLib);
    virtual void init(GlLibrary* glLib);
    GLenum getGLerror();
    void setGLerror(GLenum err);
    void setShareGroup(ShareGroupPtr grp){m_shareGroup = grp;};
    const ShareGroupPtr& shareGroup() const { return m_shareGroup; }
    virtual void setActiveTexture(GLenum tex);
    unsigned int getBindedTexture(GLenum target);
    unsigned int getBindedTexture(GLenum unit,GLenum target);
    void setBindedTexture(GLenum target,unsigned int tex);
    bool isTextureUnitEnabled(GLenum unit);
    void setTextureEnabled(GLenum target, GLenum enable);
    ObjectLocalName getDefaultTextureName(GLenum target);
    ObjectLocalName getTextureLocalName(GLenum target, unsigned int tex);
    bool isInitialized() { return m_initialized; };
    bool needRestore();
    GLint getUnpackAlignment();

    bool  isArrEnabled(GLenum);
    void  enableArr(GLenum arr,bool enable);

    void addVertexArrayObjects(GLsizei n, GLuint* arrays);
    void removeVertexArrayObjects(GLsizei n, const GLuint* arrays);
    bool setVertexArrayObject(GLuint array);
    void setVAOEverBound();
    GLuint getVertexArrayObject() const;
    bool vertexAttributesBufferBacked();
    const GLvoid* setPointer(GLenum arrType,GLint size,GLenum type,GLsizei stride,const GLvoid* data, GLsizei dataSize, bool normalize = false, bool isInt = false);
    virtual const GLESpointer* getPointer(GLenum arrType);
    virtual void setupArraysPointers(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct) = 0;

    void bindBuffer(GLenum target,GLuint buffer);
    void bindIndexedBuffer(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size, GLintptr stride = 0);
    void bindIndexedBuffer(GLenum target, GLuint index, GLuint buffer);
    void unbindBuffer(GLuint buffer);
    bool isBuffer(GLuint buffer);
    bool isBindedBuffer(GLenum target);
    GLvoid* getBindedBuffer(GLenum target);
    GLuint getBuffer(GLenum target);
    GLuint getIndexedBuffer(GLenum target, GLuint index);
    void getBufferSize(GLenum target,GLint* param);
    void getBufferSizeById(GLuint buffer,GLint* param);
    void getBufferUsage(GLenum target,GLint* param);
    bool setBufferData(GLenum target,GLsizeiptr size,const GLvoid* data,GLenum usage);
    bool setBufferSubData(GLenum target,GLintptr offset,GLsizeiptr size,const GLvoid* data);
    const char * getExtensionString();
    const char * getVendorString() const;
    const char * getRendererString() const;
    const char * getVersionString() const;
    void getGlobalLock();
    void releaseGlobalLock();
    virtual GLSupport*  getCaps(){return &s_glSupport;};
    virtual ~GLEScontext();
    virtual int getMaxTexUnits() = 0;
    virtual int getMaxCombinedTexUnits() { return getMaxTexUnits(); }
    virtual void drawValidate(void);

    // Default FBO emulation. Do not call this from GLEScontext context;
    // it needs dynamic dispatch (from GLEScmContext or GLESv2Context DLLs)
    // to pick up on the right functions.
    virtual void initDefaultFBO(
            GLint width, GLint height, GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
            GLuint* eglSurfaceRBColorId, GLuint* eglSurfaceRBDepthId,
            GLuint readWidth, GLint readHeight, GLint readColorFormat, GLint readDepthStencilFormat, GLint readMultisamples,
            GLuint* eglReadSurfaceRBColorId, GLuint* eglReadSurfaceRBDepthId);
    void initEmulatedEGLSurface(GLint width, GLint height,
                             GLint colorFormat, GLint depthstencilFormat, GLint multisamples,
                             GLuint rboColor, GLuint rboDepth);

    GLuint getDefaultFBOGlobalName() const { return m_defaultFBO; }
    bool isDefaultFBOBound(GLenum target) const { return !getFramebufferBinding(target); }
    bool hasEmulatedDefaultFBO() const { return m_defaultFBO != 0; }

    int getDefaultFBOColorFormat() const { return m_defaultFBOColorFormat; }
    int getDefaultFBOWidth() const { return m_defaultFBOWidth; }
    int getDefaultFBOHeight() const { return m_defaultFBOHeight; }
    int getDefaultFBOMultisamples() const { return m_defaultFBOSamples; }

    void setRenderbufferBinding(GLuint rb) { m_renderbuffer = rb; }
    GLuint getRenderbufferBinding() const { return m_renderbuffer; }
    void setFramebufferBinding(GLenum target, GLuint fb) {
        switch (target) {
        case GL_READ_FRAMEBUFFER:
            m_readFramebuffer = fb;
            break;
        case GL_DRAW_FRAMEBUFFER:
            m_drawFramebuffer = fb;
            break;
        case GL_FRAMEBUFFER:
            m_readFramebuffer = fb;
            m_drawFramebuffer = fb;
            break;
        default:
            m_drawFramebuffer = fb;
            break;
        }
    }
    GLuint getFramebufferBinding(GLenum target) const {
        switch (target) {
        case GL_READ_FRAMEBUFFER:
            return m_readFramebuffer;
        case GL_DRAW_FRAMEBUFFER:
        case GL_FRAMEBUFFER:
            return m_drawFramebuffer;
        }
        return m_drawFramebuffer;
    }

    void setEnable(GLenum item, bool isEnable);
    void setBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB,
            GLenum srcAlpha, GLenum dstAlpha);
    void setPixelStorei(GLenum pname, GLint param);

    void setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    void setPolygonOffset(GLfloat factor, GLfloat units);
    void setScissor(GLint x, GLint y, GLsizei width, GLsizei height);
    void setCullFace(GLenum mode);
    void setFrontFace(GLenum mode);

    void setDepthFunc(GLenum func);
    void setDepthMask(GLboolean flag);
    void setDepthRangef(GLclampf zNear, GLclampf zFar);
    void setLineWidth(GLfloat lineWidth);
    void setSampleCoverage(GLclampf value, GLboolean invert);

    void setStencilFuncSeparate(GLenum face, GLenum func, GLint ref,
            GLuint mask);
    void setStencilMaskSeparate(GLenum face, GLuint mask);
    void setStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail,
            GLenum zpass);

    void setColorMask(GLboolean red, GLboolean green, GLboolean blue,
            GLboolean alpha);

    void setClearColor(GLclampf red, GLclampf green, GLclampf blue,
            GLclampf alpha);
    void setClearDepth(GLclampf depth);
    void setClearStencil(GLint s);

    static GLDispatch& dispatcher(){return s_glDispatch;};

    static int getMaxLights(){return s_glSupport.maxLights;}
    static int getMaxClipPlanes(){return s_glSupport.maxClipPlane;}
    static int getMaxTexSize(){return s_glSupport.maxTexSize;}
    static Version glslVersion(){return s_glSupport.glslVersion;}
    static bool isAutoMipmapSupported(){return s_glSupport.GL_SGIS_GENERATE_MIPMAP;}
    static TextureTarget GLTextureTargetToLocal(GLenum target);
    static unsigned int findMaxIndex(GLsizei count,GLenum type,const GLvoid* indices);

    virtual bool glGetIntegerv(GLenum pname, GLint *params);
    virtual bool glGetBooleanv(GLenum pname, GLboolean *params);
    virtual bool glGetFloatv(GLenum pname, GLfloat *params);
    virtual bool glGetFixedv(GLenum pname, GLfixed *params);

    int getMajorVersion() const { return m_glesMajorVersion; }
    int getMinorVersion() const { return m_glesMinorVersion; }

    // FBO
    void initFBONameSpace(GlobalNameSpace* globalNameSpace,
            android::base::Stream* stream);
    bool isFBO(ObjectLocalName p_localName);
    ObjectLocalName genFBOName(ObjectLocalName p_localName = 0,
            bool genLocal = 0);
    void setFBOData(ObjectLocalName p_localName, ObjectDataPtr data);
    void setDefaultFBODrawBuffer(GLenum buffer);
    void setDefaultFBOReadBuffer(GLenum buffer);
    void deleteFBO(ObjectLocalName p_localName);
    FramebufferData* getFBOData(ObjectLocalName p_localName);
    ObjectDataPtr getFBODataPtr(ObjectLocalName p_localName);
    unsigned int getFBOGlobalName(ObjectLocalName p_localName);
    ObjectLocalName getFBOLocalName(unsigned int p_globalName);

    bool isVAO(ObjectLocalName p_localName);
    ObjectLocalName genVAOName(ObjectLocalName p_localName = 0,
            bool genLocal = 0);
    void deleteVAO(ObjectLocalName p_localName);
    unsigned int getVAOGlobalName(ObjectLocalName p_localName);
    ObjectLocalName getVAOLocalName(unsigned int p_globalName);

    // Snapshot save
    virtual void onSave(android::base::Stream* stream) const;
    virtual ObjectDataPtr loadObject(NamedObjectType type,
            ObjectLocalName localName, android::base::Stream* stream) const;
    // postLoad is triggered after setting up ShareGroup
    virtual void postLoad();
    virtual void restore();
protected:
    void initDefaultFboImpl(
        GLint width, GLint height,
        GLint colorFormat, GLint depthstencilFormat,
        GLint multisamples,
        GLuint* eglSurfaceRBColorId,
        GLuint* eglSurfaceRBDepthId);

    virtual void postLoadRestoreShareGroup();
    virtual void postLoadRestoreCtx();

    static void buildStrings(const char* baseVendor, const char* baseRenderer, const char* baseVersion, const char* version);

    void freeVAOState();
    void addVertexArrayObject(GLuint array);
    void removeVertexArrayObject(GLuint array);

    virtual bool needConvert(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct,GLESpointer* p,GLenum array_id) = 0;
    void convertDirect(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum array_id,GLESpointer* p);
    void convertDirectVBO(GLESConversionArrays& fArrs,GLint first,GLsizei count,GLenum array_id,GLESpointer* p);
    void convertIndirect(GLESConversionArrays& fArrs,GLsizei count,GLenum type,const GLvoid* indices,GLenum array_id,GLESpointer* p);
    void convertIndirectVBO(GLESConversionArrays& fArrs,GLsizei count,GLenum indices_type,const GLvoid* indices,GLenum array_id,GLESpointer* p);
    void initCapsLocked(const GLubyte * extensionString);
    virtual void initExtensionString() =0;

    bool                  m_needRestoreFromSnapshot = false;
    static emugl::Mutex   s_lock;
    static GLDispatch     s_glDispatch;
    bool                  m_initialized = false;
    unsigned int          m_activeTexture = 0;

    VAOStateMap           m_vaoStateMap;
    VAOStateRef           m_currVaoState;
    // Buffer binding state
    GLuint m_copyReadBuffer = 0;
    GLuint m_copyWriteBuffer = 0;
    GLuint m_pixelPackBuffer = 0;
    GLuint m_pixelUnpackBuffer = 0;
    GLuint m_transformFeedbackBuffer = 0;
    GLuint m_uniformBuffer = 0;
    GLuint m_atomicCounterBuffer = 0;
    GLuint m_dispatchIndirectBuffer = 0;
    GLuint m_drawIndirectBuffer = 0;
    GLuint m_shaderStorageBuffer = 0;
    std::vector<BufferBinding> m_indexedTransformFeedbackBuffers;
    std::vector<BufferBinding> m_indexedUniformBuffers;
    std::vector<BufferBinding> m_indexedAtomicCounterBuffers;
    std::vector<BufferBinding> m_indexedShaderStorageBuffers;

    GLint m_viewportX = 0;
    GLint m_viewportY = 0;
    GLsizei m_viewportWidth = 0;
    GLsizei m_viewportHeight = 0;

    GLfloat m_polygonOffsetFactor = 0.0f;
    GLfloat m_polygonOffsetUnits = 0.0f;

    GLint m_scissorX = 0;
    GLint m_scissorY = 0;
    GLsizei m_scissorWidth = 0;
    GLsizei m_scissorHeight = 0;

    std::unordered_map<GLenum, bool> m_glEnableList = {};

    GLenum m_blendSrcRgb = GL_ONE;
    GLenum m_blendDstRgb = GL_ZERO;
    GLenum m_blendSrcAlpha = GL_ONE;
    GLenum m_blendDstAlpha = GL_ZERO;

    std::unordered_map<GLenum, GLint> m_glPixelStoreiList = {};

    GLenum m_cullFace = GL_BACK;
    GLenum m_frontFace = GL_CCW;

    GLenum m_depthFunc = GL_LESS;
    GLboolean m_depthMask = GL_TRUE;
    GLclampf m_zNear = 0.0f;
    GLclampf m_zFar = 1.0f;

    GLfloat m_lineWidth = 1.0f;

    GLclampf m_sampleCoverageVal = 1.0f;
    GLboolean m_sampleCoverageInvert = GL_FALSE;

    enum {
        StencilFront = 0,
        StencilBack
    };
    struct {
        GLenum m_func = GL_ALWAYS;
        GLint m_ref = 0;
        GLuint m_funcMask = -1; // all bits set to 1
        GLuint m_writeMask = -1; // all bits set to 1
        GLenum m_sfail = GL_KEEP;
        GLenum m_dpfail = GL_KEEP;
        GLenum m_dppass = GL_KEEP;
    } m_stencilStates[2];

    bool m_colorMaskR = GL_TRUE;
    bool m_colorMaskG = GL_TRUE;
    bool m_colorMaskB = GL_TRUE;
    bool m_colorMaskA = GL_TRUE;

    GLclampf m_clearColorR = 0.0f;
    GLclampf m_clearColorG = 0.0f;
    GLclampf m_clearColorB = 0.0f;
    GLclampf m_clearColorA = 0.0f;

    GLclampf m_clearDepth = 1.0f;
    GLint m_clearStencil = 0;

    static std::string*   s_glExtensions;
    static GLSupport      s_glSupport;

    int m_glesMajorVersion = 1;
    int m_glesMinorVersion = 0;

    ShareGroupPtr         m_shareGroup;

    // Default FBO per-context state
    GLuint m_defaultFBO = 0;
    GLuint m_defaultReadFBO = 0;
    GLuint m_defaultRBColor = 0;
    GLuint m_defaultRBDepth = 0;
    GLint m_defaultFBOWidth = 0;
    GLint m_defaultFBOHeight = 0;
    GLint m_defaultFBOColorFormat = 0;
    GLint m_defaultFBOSamples = 0;
    GLenum m_defaultFBODrawBuffer = GL_COLOR_ATTACHMENT0;
    GLenum m_defaultFBOReadBuffer = GL_COLOR_ATTACHMENT0;
private:

    virtual void setupArr(const GLvoid* arr,GLenum arrayType,GLenum dataType,GLint size,GLsizei stride, GLboolean normalized, int pointsIndex = -1, bool isInt = false) = 0 ;

    GLenum                m_glError = GL_NO_ERROR;
    int                   m_maxTexUnits;
    unsigned int          m_maxUsedTexUnit = 0;
    textureUnitState*     m_texState = nullptr;
    unsigned int          m_arrayBuffer = 0;
    unsigned int          m_elementBuffer = 0;
    GLuint                m_renderbuffer = 0;
    GLuint                m_drawFramebuffer = 0;
    GLuint                m_readFramebuffer = 0;

    static std::string    s_glVendor;
    static std::string    s_glRenderer;
    static std::string    s_glVersion;

    NameSpace* m_fboNameSpace = nullptr;
    // m_vaoNameSpace is an empty shell that holds the names but not the data
    // TODO(yahan): consider moving the data into it?
    NameSpace* m_vaoNameSpace = nullptr;
};

#endif

