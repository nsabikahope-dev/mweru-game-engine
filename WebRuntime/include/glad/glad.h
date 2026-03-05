#pragma once
/*
 * Emscripten glad redirect
 *
 * When building with Emscripten, all OpenGL ES 3.0 (WebGL2) functions are
 * directly available — no loader step is required.  This header replaces the
 * desktop glad loader so that Engine sources compile unchanged.
 */
#include <GLES3/gl3.h>

/* Types and stubs for the glad loader — always succeeds on Emscripten */
typedef void* (*GLADloadfunc)(const char* name);
typedef void* (*GLADuserptrloadfunc)(void* userptr, const char* name);

#ifndef gladLoadGL
static inline int gladLoadGL(GLADloadfunc fn) { (void)fn; return 1; }
#endif
