// Auto-generated with: android/scripts/gen-entries.py --mode=translator_passthrough android/android-emugl/host/libs/libOpenGLESDispatch/gles31_only.entries --output=android/android-emugl/host/libs/Translator/GLES_V2/GLESv31Imp.cpp
// This file is best left unedited.
// Try to make changes through gen_translator in gen-entries.py,
// and/or parcel out custom functionality in separate code.
GL_APICALL void GL_APIENTRY glGetBooleani_v(GLenum target, GLuint index, GLboolean * data) {
    GET_CTX_V2();
    ctx->dispatcher().glGetBooleani_v(target, index, data);
}

GL_APICALL void GL_APIENTRY glMemoryBarrier(GLbitfield barriers) {
    GET_CTX_V2();
    ctx->dispatcher().glMemoryBarrier(barriers);
}

GL_APICALL void GL_APIENTRY glMemoryBarrierByRegion(GLbitfield barriers) {
    GET_CTX_V2();
    ctx->dispatcher().glMemoryBarrierByRegion(barriers);
}

GL_APICALL void GL_APIENTRY glGenProgramPipelines(GLsizei n, GLuint * pipelines) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);
    ctx->dispatcher().glGenProgramPipelines(n, pipelines);
}

GL_APICALL void GL_APIENTRY glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);
    ctx->dispatcher().glDeleteProgramPipelines(n, pipelines);
}

GL_APICALL void GL_APIENTRY glBindProgramPipeline(GLuint pipeline) {
    GET_CTX_V2();
    ctx->dispatcher().glBindProgramPipeline(pipeline);
}

GL_APICALL void GL_APIENTRY glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetProgramPipelineiv(pipeline, pname, params);

    switch (pname) {
    case GL_ACTIVE_PROGRAM:
    case GL_VERTEX_SHADER:
    case GL_FRAGMENT_SHADER:
    case GL_COMPUTE_SHADER: {
        GLint programName = *params;
        GLint localProgramName =
            ctx->shareGroup()->getLocalName(NamedObjectType::SHADER_OR_PROGRAM, programName);
        *params = localProgramName;
        break;
    }
    default:
        break;
    }
}

GL_APICALL void GL_APIENTRY glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    GET_CTX_V2();
    ctx->dispatcher().glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
}

GL_APICALL void GL_APIENTRY glValidateProgramPipeline(GLuint pipeline) {
    GET_CTX_V2();
    ctx->dispatcher().glValidateProgramPipeline(pipeline);
}

GL_APICALL GLboolean GL_APIENTRY glIsProgramPipeline(GLuint pipeline) {
    GET_CTX_V2_RET(0);
    GLboolean glIsProgramPipelineRET = ctx->dispatcher().glIsProgramPipeline(pipeline);
    return glIsProgramPipelineRET;
}

GL_APICALL void GL_APIENTRY glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glUseProgramStages(pipeline, stages, globalProgramName);
    }
}

GL_APICALL void GL_APIENTRY glActiveShaderProgram(GLuint pipeline, GLuint program) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glActiveShaderProgram(pipeline, globalProgramName);
    }
}

extern "C" GL_APICALL GLuint GL_APIENTRY glCreateShaderProgramv(GLenum type, GLsizei count, const char ** strings) {
    GET_CTX_V2_RET(0);

    GLuint glCreateShaderProgramvRET = ctx->dispatcher().glCreateShaderProgramv(type, count, strings);

    GLint sep = GL_FALSE;
    GLint linkstatus = GL_FALSE;
    ctx->dispatcher().glGetProgramiv(glCreateShaderProgramvRET, GL_PROGRAM_SEPARABLE, &sep);
    ctx->dispatcher().glGetProgramiv(glCreateShaderProgramvRET, GL_LINK_STATUS, &linkstatus);

    const GLuint localProgramName =
        ctx->shareGroup()->genName(ShaderProgramType::PROGRAM, 0, true, glCreateShaderProgramvRET);

    ProgramData* progdata = new ProgramData();
    progdata->setLinkStatus(GL_TRUE);

    ctx->shareGroup()->setObjectData(NamedObjectType::SHADER_OR_PROGRAM, localProgramName, ObjectDataPtr(progdata));

    return localProgramName;
}

GL_APICALL void GL_APIENTRY glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform1f(globalProgramName, location, v0);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform2f(globalProgramName, location, v0, v1);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform3f(globalProgramName, location, v0, v1, v2);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform4f(globalProgramName, location, v0, v1, v2, v3);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform1i(globalProgramName, location, v0);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform2i(globalProgramName, location, v0, v1);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform3i(globalProgramName, location, v0, v1, v2);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform4i(globalProgramName, location, v0, v1, v2, v3);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform1ui(globalProgramName, location, v0);
    }
}

extern "C" GL_APICALL void GL_APIENTRY glProgramUniform2ui(GLuint program, GLint location, GLint v0, GLuint v1) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform2ui(globalProgramName, location, v0, v1);
    }
}

extern "C" GL_APICALL void GL_APIENTRY glProgramUniform3ui(GLuint program, GLint location, GLint v0, GLint v1, GLuint v2) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform3ui(globalProgramName, location, v0, v1, v2);
    }
}

extern "C" GL_APICALL void GL_APIENTRY glProgramUniform4ui(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLuint v3) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform4ui(globalProgramName, location, v0, v1, v2, v3);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform1fv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform2fv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform3fv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform4fv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform1iv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform2iv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform3iv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform4iv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform1uiv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform2uiv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform3uiv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniform4uiv(globalProgramName, location, count, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix2fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix3fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix4fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix2x3fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix3x2fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix2x4fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix4x2fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix3x4fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramUniformMatrix4x3fv(globalProgramName, location, count, transpose, value);
    }
}

GL_APICALL void GL_APIENTRY glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetProgramInterfaceiv(globalProgramName, programInterface, pname, params);
    }
}

GL_APICALL void GL_APIENTRY glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei * length, GLint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetProgramResourceiv(globalProgramName, programInterface, index, propCount, props, bufSize, length, params);
    }
}

GL_APICALL GLuint GL_APIENTRY glGetProgramResourceIndex(GLuint program, GLenum programInterface, const char * name) {
    GET_CTX_V2_RET(0);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        GLuint glGetProgramResourceIndexRET = ctx->dispatcher().glGetProgramResourceIndex(globalProgramName, programInterface, name);
    return glGetProgramResourceIndexRET;
    } else return 0;
}

GL_APICALL GLint GL_APIENTRY glGetProgramResourceLocation(GLuint program, GLenum programInterface, const char * name) {
    GET_CTX_V2_RET(0);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        GLint glGetProgramResourceLocationRET = ctx->dispatcher().glGetProgramResourceLocation(globalProgramName, programInterface, name);
    return glGetProgramResourceLocationRET;
    } else return 0;
}

GL_APICALL void GL_APIENTRY glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, char * name) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetProgramResourceName(globalProgramName, programInterface, index, bufSize, length, name);
    }
}

GL_APICALL void GL_APIENTRY glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalTextureName = ctx->shareGroup()->getGlobalName(NamedObjectType::TEXTURE, texture);
        ctx->dispatcher().glBindImageTexture(unit, globalTextureName, level, layered, layer, access, format);
    }
}

GL_APICALL void GL_APIENTRY glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    GET_CTX_V2();
    ctx->dispatcher().glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}

GL_APICALL void GL_APIENTRY glDispatchComputeIndirect(GLintptr indirect) {
    GET_CTX_V2();
    ctx->dispatcher().glDispatchComputeIndirect(indirect);
}

extern "C" GL_APICALL void GL_APIENTRY glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLintptr stride) {
    GET_CTX_V2();

    ctx->bindIndexedBuffer(0, bindingindex, buffer, offset, 0, stride);
    if (ctx->shareGroup().get()) {
        const GLuint globalBufferName = ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer);
        ctx->dispatcher().glBindVertexBuffer(bindingindex, globalBufferName, offset, stride);
    }
}

GL_APICALL void GL_APIENTRY glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    GET_CTX_V2();

    ctx->setVertexAttribBindingIndex(attribindex, bindingindex);
    ctx->dispatcher().glVertexAttribBinding(attribindex, bindingindex);
}

GL_APICALL void GL_APIENTRY glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GET_CTX_V2();

    ctx->setVertexAttribFormat(attribindex, size, type, normalized, relativeoffset, false);
    ctx->dispatcher().glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
}

GL_APICALL void GL_APIENTRY glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GET_CTX_V2();

    ctx->setVertexAttribFormat(attribindex, size, type, GL_FALSE, relativeoffset, true);
    ctx->dispatcher().glVertexAttribIFormat(attribindex, size, type, relativeoffset);
}

GL_APICALL void GL_APIENTRY glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
    GET_CTX_V2();

    ctx->setVertexAttribDivisor(bindingindex, divisor);
    ctx->dispatcher().glVertexBindingDivisor(bindingindex, divisor);
}

GL_APICALL void GL_APIENTRY glDrawArraysIndirect(GLenum mode, const void * indirect) {
    GET_CTX_V2();
    ctx->dispatcher().glDrawArraysIndirect(mode, indirect);
}

GL_APICALL void GL_APIENTRY glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect) {
    GET_CTX_V2();
    ctx->dispatcher().glDrawElementsIndirect(mode, type, indirect);
}

GL_APICALL void GL_APIENTRY glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GET_CTX_V2();

    GLint err = GL_NO_ERROR;
    GLenum format, type;
    GLESv2Validate::getCompatibleFormatTypeForInternalFormat(internalformat, &format, &type);
    sPrepareTexImage2D(target, 0, (GLint)internalformat, width, height, 0, format, type, NULL, &type, (GLint*)&internalformat, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    ctx->dispatcher().glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}

GL_APICALL void GL_APIENTRY glSampleMaski(GLuint maskNumber, GLbitfield mask) {
    GET_CTX_V2();
    ctx->dispatcher().glSampleMaski(maskNumber, mask);
}

GL_APICALL void GL_APIENTRY glGetMultisamplefv(GLenum pname, GLuint index, GLfloat * val) {
    GET_CTX_V2();
    ctx->dispatcher().glGetMultisamplefv(pname, index, val);
}

GL_APICALL void GL_APIENTRY glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {
    GET_CTX_V2();
    ctx->dispatcher().glFramebufferParameteri(target, pname, param);
}

GL_APICALL void GL_APIENTRY glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetFramebufferParameteriv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetTexLevelParameterfv(target, level, pname, params);

    if (!ctx->shareGroup().get()) return;

    TextureData* texData = getTextureTargetData(target);

    if (!texData) return;

    switch (pname) {
    case GL_TEXTURE_INTERNAL_FORMAT:
        // Need the correct internal format if the texture has not been initialized at all yet.
        if (!texData->hasStorage) {
            *params = texData->internalFormat;
        }

        if (texData->compressed) {
            *params = texData->compressedFormat;
        }
        break;
    case GL_TEXTURE_COMPRESSED:
        if (texData->compressed) {
            *params = GL_TRUE;
        }
        break;
    default:
        break;
    }
}

GL_APICALL void GL_APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetTexLevelParameteriv(target, level, pname, params);

    if (!ctx->shareGroup().get()) return;

    TextureData* texData = getTextureTargetData(target);

    if (!texData) return;

    switch (pname) {
    case GL_TEXTURE_INTERNAL_FORMAT:
        // Need the correct internal format if the texture has not been initialized at all yet.
        if (!texData->hasStorage) {
            *params = texData->internalFormat;
        }

        if (texData->compressed) {
            *params = texData->compressedFormat;
        }
        break;
    case GL_TEXTURE_COMPRESSED:
        if (texData->compressed) {
            *params = GL_TRUE;
        }
        break;
    default:
        break;
    }
}

