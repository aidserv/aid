// Auto-generated with: android/scripts/gen-entries.py --mode=funcargs android/android-emugl/host/libs/libOpenGLESDispatch/gles2_only.entries --output=android/android-emugl/host/include/OpenGLESDispatch/gles2_only_functions.h
// DO NOT EDIT THIS FILE

#ifndef GLES2_ONLY_FUNCTIONS_H
#define GLES2_ONLY_FUNCTIONS_H

#define LIST_GLES2_ONLY_FUNCTIONS(X) \
  X(void, glBlendColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha), (red, green, blue, alpha)) \
  X(void, glStencilFuncSeparate, (GLenum face, GLenum func, GLint ref, GLuint mask), (face, func, ref, mask)) \
  X(void, glStencilMaskSeparate, (GLenum face, GLuint mask), (face, mask)) \
  X(void, glStencilOpSeparate, (GLenum face, GLenum fail, GLenum zfail, GLenum zpass), (face, fail, zfail, zpass)) \
  X(GLboolean, glIsProgram, (GLuint program), (program)) \
  X(GLboolean, glIsShader, (GLuint shader), (shader)) \
  X(void, glVertexAttrib1f, (GLuint indx, GLfloat x), (indx, x)) \
  X(void, glVertexAttrib1fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttrib2f, (GLuint indx, GLfloat x, GLfloat y), (indx, x, y)) \
  X(void, glVertexAttrib2fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttrib3f, (GLuint indx, GLfloat x, GLfloat y, GLfloat z), (indx, x, y, z)) \
  X(void, glVertexAttrib3fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttrib4f, (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w), (indx, x, y, z, w)) \
  X(void, glVertexAttrib4fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttribPointer, (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr), (indx, size, type, normalized, stride, ptr)) \
  X(void, glDisableVertexAttribArray, (GLuint index), (index)) \
  X(void, glEnableVertexAttribArray, (GLuint index), (index)) \
  X(void, glGetVertexAttribfv, (GLuint index, GLenum pname, GLfloat* params), (index, pname, params)) \
  X(void, glGetVertexAttribiv, (GLuint index, GLenum pname, GLint* params), (index, pname, params)) \
  X(void, glGetVertexAttribPointerv, (GLuint index, GLenum pname, GLvoid** pointer), (index, pname, pointer)) \
  X(void, glUniform1f, (GLint location, GLfloat x), (location, x)) \
  X(void, glUniform1fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform1i, (GLint location, GLint x), (location, x)) \
  X(void, glUniform1iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniform2f, (GLint location, GLfloat x, GLfloat y), (location, x, y)) \
  X(void, glUniform2fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform2i, (GLint location, GLint x, GLint y), (location, x, y)) \
  X(void, glUniform2iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniform3f, (GLint location, GLfloat x, GLfloat y, GLfloat z), (location, x, y, z)) \
  X(void, glUniform3fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform3i, (GLint location, GLint x, GLint y, GLint z), (location, x, y, z)) \
  X(void, glUniform3iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniform4f, (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w), (location, x, y, z, w)) \
  X(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform4i, (GLint location, GLint x, GLint y, GLint z, GLint w), (location, x, y, z, w)) \
  X(void, glUniform4iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniformMatrix2fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value)) \
  X(void, glUniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value)) \
  X(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value)) \
  X(void, glAttachShader, (GLuint program, GLuint shader), (program, shader)) \
  X(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name), (program, index, name)) \
  X(void, glCompileShader, (GLuint shader), (shader)) \
  X(GLuint, glCreateProgram, (), ()) \
  X(GLuint, glCreateShader, (GLenum type), (type)) \
  X(void, glDeleteProgram, (GLuint program), (program)) \
  X(void, glDeleteShader, (GLuint shader), (shader)) \
  X(void, glDetachShader, (GLuint program, GLuint shader), (program, shader)) \
  X(void, glLinkProgram, (GLuint program), (program)) \
  X(void, glUseProgram, (GLuint program), (program)) \
  X(void, glValidateProgram, (GLuint program), (program)) \
  X(void, glGetActiveAttrib, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name), (program, index, bufsize, length, size, type, name)) \
  X(void, glGetActiveUniform, (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name), (program, index, bufsize, length, size, type, name)) \
  X(void, glGetAttachedShaders, (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders), (program, maxcount, count, shaders)) \
  X(int, glGetAttribLocation, (GLuint program, const GLchar* name), (program, name)) \
  X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint* params), (program, pname, params)) \
  X(void, glGetProgramInfoLog, (GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog), (program, bufsize, length, infolog)) \
  X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint* params), (shader, pname, params)) \
  X(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog), (shader, bufsize, length, infolog)) \
  X(void, glGetShaderSource, (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source), (shader, bufsize, length, source)) \
  X(void, glGetUniformfv, (GLuint program, GLint location, GLfloat* params), (program, location, params)) \
  X(void, glGetUniformiv, (GLuint program, GLint location, GLint* params), (program, location, params)) \
  X(int, glGetUniformLocation, (GLuint program, const GLchar* name), (program, name)) \
  X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length), (shader, count, string, length)) \
  X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer), (target, framebuffer)) \
  X(void, glGenFramebuffers, (GLsizei n, GLuint* framebuffers), (n, framebuffers)) \
  X(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level), (target, attachment, textarget, texture, level)) \
  X(GLenum, glCheckFramebufferStatus, (GLenum target), (target)) \
  X(void, glDeleteFramebuffers, (GLsizei n, const GLuint* framebuffers), (n, framebuffers)) \
  X(GLboolean, glIsRenderbuffer, (GLuint renderbuffer), (renderbuffer)) \
  X(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer), (target, renderbuffer)) \
  X(void, glDeleteRenderbuffers, (GLsizei n, const GLuint * renderbuffers), (n, renderbuffers)) \
  X(void, glGenRenderbuffers, (GLsizei n, GLuint * renderbuffers), (n, renderbuffers)) \
  X(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height), (target, internalformat, width, height)) \
  X(void, glGetRenderbufferParameteriv, (GLenum target, GLenum pname, GLint * params), (target, pname, params)) \
  X(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer), (target, attachment, renderbuffertarget, renderbuffer)) \
  X(void, glGenerateMipmap, (GLenum target), (target)) \


#endif  // GLES2_ONLY_FUNCTIONS_H
