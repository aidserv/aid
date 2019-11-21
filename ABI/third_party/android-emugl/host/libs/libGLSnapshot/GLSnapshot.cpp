#include "GLSnapshot.h"

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

#include <stdio.h>

#define DEBUG 0

#if DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while(0) \

#else
#define D(...)
#endif

namespace GLSnapshot {

GLSnapshotState::GLSnapshotState(const GLESv2Dispatch* gl) : mGL(gl) {
    D("init snapshot state");
}

void GLSnapshotState::getGlobalStateEnum(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<GLenum>& store = mGlobals[name].enums;
    store.resize(size);
    mGL->glGetIntegerv(name, (GLint*)&store[0]);
}

void GLSnapshotState::getGlobalStateByte(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<unsigned char>& store = mGlobals[name].bytes;
    store.resize(size);
    mGL->glGetBooleanv(name, (GLboolean*)&store[0]);
}

void GLSnapshotState::getGlobalStateInt(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<uint32_t>& store = mGlobals[name].ints;
    store.resize(size);
    mGL->glGetIntegerv(name, (GLint*)&store[0]);
}

void GLSnapshotState::getGlobalStateFloat(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<float>& store = mGlobals[name].floats;
    store.resize(size);
    mGL->glGetFloatv(name, (GLfloat*)&store[0]);
}

void GLSnapshotState::getGlobalStateInt64(GLenum name, int size) {
    D("save 0x%x", name);
    std::vector<uint64_t>& store = mGlobals[name].int64s;
    store.resize(size);
    mGL->glGetInteger64v(name, (GLint64*)&store[0]);
}

void GLSnapshotState::getGlobalStateEnable(GLenum name) {
    D("save 0x%x", name);
    mEnables[name] = mGL->glIsEnabled(name) == GL_TRUE;
}

void GLSnapshotState::save() {
    getGlobalStateEnable(GL_DEPTH_TEST);

    getGlobalStateFloat(GL_COLOR_CLEAR_VALUE, 4);
    getGlobalStateInt(GL_ACTIVE_TEXTURE, 1);
}

void GLSnapshotState::restore() {
    for (const auto& it : mEnables) {
        if (it.second) {
            mGL->glEnable(it.first);
        } else {
            mGL->glDisable(it.first);
        }
    }

    for (auto& it: mProgramNames) {
        GLShaderState& shaderState = mShaderState[it.first];
        it.second = mGL->glCreateShader(shaderState.type);
        if (shaderState.source.size()) {
            GLint len = shaderState.source.size();
            const char* source = shaderState.source.c_str();
            const char** sources = &source;
            mGL->glShaderSource(it.second, 1, sources, &len);
        }
        if (shaderState.compileStatus) {
            mGL->glCompileShader(it.second);
        }
    }

    std::vector<float> clearColor = mGlobals[GL_COLOR_CLEAR_VALUE].floats;
    mGL->glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

    mGL->glActiveTexture(mGlobals[GL_ACTIVE_TEXTURE].ints[0]);
}

GLuint GLSnapshotState::createShader(GLuint shader, GLenum shaderType) {
    GLuint shaderName = mProgramCounter++;
    mProgramNames[shaderName] = shader;
    mProgramNamesBack[shader] = shaderName;
    mShaderState[shaderName].type = shaderType;
    mShaderState[shaderName].source = "";
    mShaderState[shaderName].compileStatus = false;
    return shaderName;
}

GLuint GLSnapshotState::createProgram(GLuint program) {
    return program;
}


void GLSnapshotState::shaderString(GLuint shader, const GLchar* string) {
    mShaderState[mProgramNamesBack[shader]].source = std::string(string);
    
}

void GLSnapshotState::genBuffers(GLsizei n, GLuint* buffers) {
    return;
}

GLuint GLSnapshotState::getProgramName(GLuint name) {
    return mProgramNames[name];
}

} // namespace GLSnapshot
