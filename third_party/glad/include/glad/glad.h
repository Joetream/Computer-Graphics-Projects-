#pragma once
#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*GLADloadproc)(const char* name);

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef char GLchar;
typedef std::ptrdiff_t GLintptr;
typedef std::ptrdiff_t GLsizeiptr;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_ALWAYS 0x0207
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_FRONT_AND_BACK 0x0408
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_BLEND 0x0BE2
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_TEXTURE_2D 0x0DE1
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_POINT 0x1B00
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_RGB8 0x8051
#define GL_RGBA8 0x8058
#define GL_MULTISAMPLE 0x809D
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_SRGB8 0x8C41
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000

int gladLoadGLLoader(GLADloadproc load);

// GL 1.0 / 1.1
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClear(GLbitfield mask);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void glStencilMask(GLuint mask);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPixelStorei(GLenum pname, GLint param);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
void glGenTextures(GLsizei n, GLuint* textures);
void glDeleteTextures(GLsizei n, const GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);

// GL 1.3 - 3.3 Core Extensions loaded dynamically
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (*PFNGLGENERATEMIPMAPPROC)(GLenum target);
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void (*PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint* arrays);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (*PFNGLVERTEXATTRIBDIVISORPROC)(GLuint index, GLuint divisor);
typedef void (*PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDPROC)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum type);
typedef void (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (*PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (*PFNGLUNIFORM2FVPROC)(GLint location, GLsizei count, const GLfloat* value);
typedef void (*PFNGLUNIFORM3FVPROC)(GLint location, GLsizei count, const GLfloat* value);
typedef void (*PFNGLUNIFORM4FVPROC)(GLint location, GLsizei count, const GLfloat* value);
typedef void (*PFNGLUNIFORMMATRIX3FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, const GLfloat* value);

extern PFNGLACTIVETEXTUREPROC glad_glActiveTexture;
extern PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap;
extern PFNGLGENBUFFERSPROC glad_glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glad_glBindBuffer;
extern PFNGLBUFFERDATAPROC glad_glBufferData;
extern PFNGLBUFFERSUBDATAPROC glad_glBufferSubData;
extern PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer;
extern PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor;
extern PFNGLDRAWARRAYSINSTANCEDPROC glad_glDrawArraysInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced;
extern PFNGLCREATESHADERPROC glad_glCreateShader;
extern PFNGLDELETESHADERPROC glad_glDeleteShader;
extern PFNGLSHADERSOURCEPROC glad_glShaderSource;
extern PFNGLCOMPILESHADERPROC glad_glCompileShader;
extern PFNGLGETSHADERIVPROC glad_glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC glad_glCreateProgram;
extern PFNGLDELETEPROGRAMPROC glad_glDeleteProgram;
extern PFNGLATTACHSHADERPROC glad_glAttachShader;
extern PFNGLLINKPROGRAMPROC glad_glLinkProgram;
extern PFNGLUSEPROGRAMPROC glad_glUseProgram;
extern PFNGLGETPROGRAMIVPROC glad_glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog;
extern PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation;
extern PFNGLUNIFORM1IPROC glad_glUniform1i;
extern PFNGLUNIFORM1FPROC glad_glUniform1f;
extern PFNGLUNIFORM2FVPROC glad_glUniform2fv;
extern PFNGLUNIFORM3FVPROC glad_glUniform3fv;
extern PFNGLUNIFORM4FVPROC glad_glUniform4fv;
extern PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv;

#define glActiveTexture glad_glActiveTexture
#define glGenerateMipmap glad_glGenerateMipmap
#define glGenBuffers glad_glGenBuffers
#define glDeleteBuffers glad_glDeleteBuffers
#define glBindBuffer glad_glBindBuffer
#define glBufferData glad_glBufferData
#define glBufferSubData glad_glBufferSubData
#define glGenVertexArrays glad_glGenVertexArrays
#define glDeleteVertexArrays glad_glDeleteVertexArrays
#define glBindVertexArray glad_glBindVertexArray
#define glEnableVertexAttribArray glad_glEnableVertexAttribArray
#define glVertexAttribPointer glad_glVertexAttribPointer
#define glVertexAttribDivisor glad_glVertexAttribDivisor
#define glDrawArraysInstanced glad_glDrawArraysInstanced
#define glDrawElementsInstanced glad_glDrawElementsInstanced
#define glCreateShader glad_glCreateShader
#define glDeleteShader glad_glDeleteShader
#define glShaderSource glad_glShaderSource
#define glCompileShader glad_glCompileShader
#define glGetShaderiv glad_glGetShaderiv
#define glGetShaderInfoLog glad_glGetShaderInfoLog
#define glCreateProgram glad_glCreateProgram
#define glDeleteProgram glad_glDeleteProgram
#define glAttachShader glad_glAttachShader
#define glLinkProgram glad_glLinkProgram
#define glUseProgram glad_glUseProgram
#define glGetProgramiv glad_glGetProgramiv
#define glGetProgramInfoLog glad_glGetProgramInfoLog
#define glGetUniformLocation glad_glGetUniformLocation
#define glUniform1i glad_glUniform1i
#define glUniform1f glad_glUniform1f
#define glUniform2fv glad_glUniform2fv
#define glUniform3fv glad_glUniform3fv
#define glUniform4fv glad_glUniform4fv
#define glUniformMatrix3fv glad_glUniformMatrix3fv
#define glUniformMatrix4fv glad_glUniformMatrix4fv

#ifdef __cplusplus
}
#endif
