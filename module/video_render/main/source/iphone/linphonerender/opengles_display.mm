/*
 opengles_display.m
 Copyright (C) 2011 Belledonne Communications, Grenoble, France

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "opengles_display.h"
#include "shaders.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "trace.h"

//#define DEBUG_OPENGLES_DISPLAY

#if DEBUG_OPENGLES_DISPLAY
const char *g_opengles_display = NULL;
FILE *g_file_opengles_display = NULL;
#endif

using namespace yuntongxunwebrtc;

enum ImageType {
    REMOTE_IMAGE = 0,
    PREVIEW_IMAGE
};

/* helper functions */
static void check_GL_errors(const char* context);
static bool load_shaders(GLuint* program, GLint* uniforms);
static void allocate_gl_textures(struct opengles_display* gldisp, int w, int h, enum ImageType type);
static void load_orthographic_matrix(float left, float right, float bottom, float top, float near, float far, float* mat);
static unsigned int align_on_power_of_2(unsigned int value);
static bool update_textures_with_yuv(struct opengles_display* gldisp, enum ImageType type);
static void yuv_buf_init(MSPicture *buf, int w, int h, uint8_t *ptr);

#define CHECK_GL_ERROR

#ifdef CHECK_GL_ERROR
	#define GL_OPERATION(x)	\
		(x); \
		check_GL_errors(#x);
#else
	#define GL_OPERATION(x) \
		(x);
#endif

enum {
    UNIFORM_PROJ_MATRIX = 0,
    UNIFORM_ROTATION,
    UNIFORM_TEXTURE_Y,
    UNIFORM_TEXTURE_U,
    UNIFORM_TEXTURE_V,
    NUM_UNIFORMS
};

enum {
    ATTRIB_VERTEX = 0,
    ATTRIB_UV,
    NUM_ATTRIBS
};

enum {
    Y,
    U,
    V
};

struct opengles_display {    
	/* input: yuv image to display */
    pthread_mutex_t yuv_mutex;
	mblk_t *yuv[2];
	bool new_yuv_image[2];

	/* GL resources */
	bool glResourcesInitialized;
	GLuint program, textures[2][3];
	GLint uniforms[NUM_UNIFORMS];
	MSVideoSize allocatedTexturesSize[2];

	/* GL view size */
	GLint backingWidth;
	GLint backingHeight;

	/* runtime data */
	float uvx[2], uvy[2];
    MSVideoSize yuv_size[2];

	/* coordinates of for zoom-in */
	float zoom_factor;
	float zoom_cx;
	float zoom_cy;
};

struct opengles_display* ogl_display_new() {
	struct opengles_display* result =
		(struct opengles_display*) malloc(sizeof(struct opengles_display));
	if (result == 0) {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "Could not allocate OpenGL display structure\n");
		return 0;
	}
	memset(result, 0, sizeof(struct opengles_display));
	result->zoom_factor = 1;
	result->zoom_cx = result->zoom_cy = 0;
    
	pthread_mutex_init(&result->yuv_mutex, NULL);
	return result;
}

void ogl_display_free(struct opengles_display* gldisp) {
    int i;
    
	if (!gldisp) {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "%s called with null struct opengles_display", __FUNCTION__);
		return;
	}
    
    for(i=0; i<2; i++) {
        if (gldisp->yuv[i]) {
            if(gldisp->yuv[i]->data_ptr)
                free(gldisp->yuv[i]->data_ptr);
            free(gldisp->yuv[i]);
            gldisp->yuv[i] = NULL;
        }
    }
	pthread_mutex_destroy(&gldisp->yuv_mutex);

	free(gldisp);
}

void ogl_display_init(struct opengles_display* gldisp, int width, int height) {
	int i;
	static bool version_displayed = false;
    if (!gldisp) {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "%s called with null struct opengles_display", __FUNCTION__);
		return;
	}
    
    for(int i=0; i<2; i++)  {
        gldisp->yuv[i] = (mblk_t *)malloc(sizeof(mblk_t));
        memset(gldisp->yuv[i], 0, sizeof(mblk_t));
    }

    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "init opengles_display (%d x %d, gl initialized:%d)", width, height, gldisp->glResourcesInitialized);

	GL_OPERATION(glDisable(GL_DEPTH_TEST))

	gldisp->backingWidth = width;
	gldisp->backingHeight = height;
#if DEBUG_OPENGLES_DISPLAY
    if (g_opengles_display) {
        g_file_opengles_display = fopen(g_opengles_display, "wb");
    }
#endif
    
	if (gldisp->glResourcesInitialized)
		return;

	// init textures
    for(i=0; i<2; i++) {
        GL_OPERATION(glGenTextures(3, gldisp->textures[i]))
        gldisp->allocatedTexturesSize[i].width = gldisp->allocatedTexturesSize[i].height = 0;
    }

	if (!version_displayed) {
		version_displayed = true;
		WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "OpenGL version string: %s", glGetString(GL_VERSION));
		WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "OpenGL extensions: %s",glGetString(GL_EXTENSIONS));
		WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "OpenGL vendor: %s", glGetString(GL_VENDOR));
		WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "OpenGL renderer: %s", glGetString(GL_RENDERER));
		WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "OpenGL version: %s", glGetString(GL_VERSION));
		WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "OpenGL GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	}
	load_shaders(&gldisp->program, gldisp->uniforms);
	check_GL_errors("load_shaders");

	gldisp->glResourcesInitialized = true;
}

void ogl_display_uninit(struct opengles_display* gldisp, bool freeGLresources) {
    int i;
    
    if (!gldisp) {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "%s called with null struct opengles_display", __FUNCTION__);
		return;
	}
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "uninit opengles_display (gl initialized:%d)\n", gldisp->glResourcesInitialized);
    for(i=0; i<2; i++) {
        if (gldisp->yuv[i]) {
            if(gldisp->yuv[i]->data_ptr)
                free(gldisp->yuv[i]->data_ptr);
            free(gldisp->yuv[i]);
            gldisp->yuv[i] = NULL;
        }
    }

	if (gldisp->glResourcesInitialized && freeGLresources) {
		// destroy gl resources
        for(i=0; i<2; i++) {
            GL_OPERATION(glDeleteTextures(3, gldisp->textures[i]));
            gldisp->allocatedTexturesSize[i].width = gldisp->allocatedTexturesSize[i].height = 0;
        }
		GL_OPERATION(glDeleteProgram(gldisp->program));
	}
#if DEBUG_OPENGLES_DISPLAY
    if (g_file_opengles_display) {
        fflush(g_file_opengles_display);
        fclose(g_file_opengles_display);
    }
#endif
    
	gldisp->glResourcesInitialized = false;
}

static void ogl_display_set_yuv(struct opengles_display* gldisp, mblk_t *yuv, enum ImageType type) {
	if (!gldisp) {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "%s called with null struct opengles_display", __FUNCTION__);
		return;
	}
    pthread_mutex_lock(&gldisp->yuv_mutex);
    if(gldisp->yuv[type])
    {
        if(gldisp->yuv[type]->h!=yuv->h || gldisp->yuv[type]->w != yuv->w)
        {
            free(gldisp->yuv[type]->data_ptr);
            gldisp->yuv[type]->data_ptr = (unsigned char *)malloc(yuv->datalen);
        }
        memcpy(gldisp->yuv[type]->data_ptr, yuv->data_ptr, yuv->datalen);
        gldisp->yuv[type]->h = yuv->h;
        gldisp->yuv[type]->w = yuv->w;
    }
    
	gldisp->new_yuv_image[type] = true;
    pthread_mutex_unlock(&gldisp->yuv_mutex);
}

void ogl_display_set_yuv_to_display(struct opengles_display* gldisp, mblk_t *yuv) {
	ogl_display_set_yuv(gldisp, yuv, REMOTE_IMAGE);
}

void ogl_display_set_preview_yuv_to_display(struct opengles_display* gldisp, mblk_t *yuv) {
	ogl_display_set_yuv(gldisp, yuv, PREVIEW_IMAGE);
}

static void ogl_display_render_type(struct opengles_display* gldisp, enum ImageType type, bool clear, float vpx, float vpy, float vpw, float vph, int orientation, int contentMode) {
 	if (!gldisp) {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, 0, "%s called with null struct opengles_display", __FUNCTION__);
		return;
	}
	if (!gldisp->yuv[type] || !gldisp->glResourcesInitialized) {
		return;
	}
    
    GL_OPERATION(glUseProgram(gldisp->program))
    
	pthread_mutex_lock(&gldisp->yuv_mutex);
	if (gldisp->new_yuv_image[type]) {
    	update_textures_with_yuv(gldisp, type);
		gldisp->new_yuv_image[type] = false;
	}
	pthread_mutex_unlock(&gldisp->yuv_mutex);
    
	float uLeft, uRight, vTop, vBottom;

	uLeft = vBottom = 0.0f;
	uRight = gldisp->uvx[type];
	vTop = gldisp->uvy[type]; 

	GLfloat squareUvs[] = {
		uLeft, vTop,
		uRight, vTop,
		uLeft, vBottom,
		uRight, vBottom
    };
    
    if (clear) {
        GL_OPERATION(glViewport(0, 0, gldisp->backingWidth, gldisp->backingHeight));
        GL_OPERATION(glClearColor(0, 0, 0, 1))
        GL_OPERATION(glClear(GL_COLOR_BUFFER_BIT))
    }
    
    GLfloat squareVertices[8];
    
	// drawing surface dimensions
	int screenW = gldisp->backingWidth;
	int screenH = gldisp->backingHeight;    
	if (orientation == 90 || orientation == 270) {
		screenW = screenH;
		screenH = gldisp->backingWidth;	    	
	}

	int x=vpx,y=vpy,w=0,h=0;
	// Fill the smallest dimension, then compute the other one using the image ratio
    if (UIViewContentModeScaleAspectFit == contentMode) {
        if (screenW <= screenH) {
            float ratio = (gldisp->yuv_size[type].height) / (float)(gldisp->yuv_size[type].width);
            w = screenW * vpw;
            h = w * ratio;
            if (h > screenH) {
                w *= screenH /(float) h;
                h = screenH;
            }
            x = vpx * gldisp->backingWidth;
            y = vpy * gldisp->backingHeight;
        } else {
            
            float ratio = gldisp->yuv_size[type].width / (float)gldisp->yuv_size[type].height;
            h = screenH * vph;
            w = h * ratio;
            if (w > screenW) {
                h *= screenW / (float)w;
                w = screenW;
            }
            x = vpx * screenW;
            y = vpy * screenH;
            
        }
    } else if (UIViewContentModeScaleAspectFill == contentMode) {
        
        if (screenW <= screenH) {
            float ratio = gldisp->yuv_size[type].width / (float)gldisp->yuv_size[type].height;
            h = screenH * vph;
            w = h * ratio;
            x = vpx * screenW;
            y = vpy * screenH;
        } else {
            float ratio = (gldisp->yuv_size[type].height) / (float)(gldisp->yuv_size[type].width);
            w = screenW * vpw;
            h = w * ratio;
            x = vpx * gldisp->backingWidth;
            y = vpy * gldisp->backingHeight;
        }
    } else {
        if (gldisp->yuv_size[type].height>0 && gldisp->yuv_size[type].width>0) {
            w=screenW;
            h=screenH;
        }
    }
    
    squareVertices[0] = (x - w * 0.5) / screenW - 0.;
    squareVertices[1] = (y - h * 0.5) / screenH - 0.;
    squareVertices[2] = (x + w * 0.5) / screenW - 0.;
    squareVertices[3] = (y - h * 0.5) / screenH - 0.;
    squareVertices[4] = (x - w * 0.5) / screenW - 0.;
    squareVertices[5] = (y + h * 0.5) / screenH - 0.;
    squareVertices[6] = (x + w * 0.5) / screenW - 0.;
    squareVertices[7] = (y + h * 0.5) / screenH - 0.;

    
    GL_OPERATION(glViewport(0, 0, gldisp->backingWidth, gldisp->backingHeight))
    
	GLfloat mat[16];
	#define VP_SIZE 1.0f
	if (type == REMOTE_IMAGE) {
		float scale_factor = 1.0 / gldisp->zoom_factor;
		float vpDim = (VP_SIZE * scale_factor) / 2;

        #define ENSURE_RANGE_A_INSIDE_RANGE_B(a, aSize, bMin, bMax) \
        if (2*aSize >= (bMax - bMin)) \
            a = 0; \
        else if ((a - aSize < bMin) || (a + aSize > bMax)) {  \
            float diff; \
            if (a - aSize < bMin) diff = bMin - (a - aSize); \
            else diff = bMax - (a + aSize); \
            a += diff; \
        }
        
        ENSURE_RANGE_A_INSIDE_RANGE_B(gldisp->zoom_cx, vpDim, squareVertices[0], squareVertices[2])
        ENSURE_RANGE_A_INSIDE_RANGE_B(gldisp->zoom_cy, vpDim, squareVertices[1], squareVertices[7])
       
        load_orthographic_matrix(
			gldisp->zoom_cx - vpDim, 
			gldisp->zoom_cx + vpDim, 
			gldisp->zoom_cy - vpDim, 
			gldisp->zoom_cy + vpDim, 
			0, 0.5, mat);
	} else {
		load_orthographic_matrix(- VP_SIZE * 0.5, VP_SIZE * 0.5, - VP_SIZE * 0.5, VP_SIZE * 0.5, 0, 0.5, mat);
	}
	
	GL_OPERATION(glUniformMatrix4fv(gldisp->uniforms[UNIFORM_PROJ_MATRIX], 1, GL_FALSE, mat))
    
#define degreesToRadians(d) (2.0 * 3.14157 * d / 360.0)
    float rad = degreesToRadians(orientation);
    
    GL_OPERATION(glUniform1f(gldisp->uniforms[UNIFORM_ROTATION], rad))
    
    GL_OPERATION(glActiveTexture(GL_TEXTURE0))
    GL_OPERATION(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][Y]))
    GL_OPERATION(glGenerateMipmap(GL_TEXTURE_2D))
	GL_OPERATION(glUniform1i(gldisp->uniforms[UNIFORM_TEXTURE_Y], 0))
    GL_OPERATION(glActiveTexture(GL_TEXTURE1))
    GL_OPERATION(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][U]))
    GL_OPERATION(glGenerateMipmap(GL_TEXTURE_2D))
	GL_OPERATION(glUniform1i(gldisp->uniforms[UNIFORM_TEXTURE_U], 1))
    GL_OPERATION(glActiveTexture(GL_TEXTURE2))
    GL_OPERATION(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][V]))
    GL_OPERATION(glGenerateMipmap(GL_TEXTURE_2D))
	GL_OPERATION(glUniform1i(gldisp->uniforms[UNIFORM_TEXTURE_V], 2))
    
    //NSLog(@"jiazyjiazy squareVertices[0]=%f,squareVertices[1]=%f,squareVertices[2]=%f,squareVertices[3]=%f,squareVertices[4]=%f,squareVertices[5]=%f,squareVertices[6]=%f,squareVertices[7]=%f, gldisp->backingWidth=%d, gldisp->backingHeight=%d, screenW=%d, screenH=%d",squareVertices[0],squareVertices[1],squareVertices[2],squareVertices[3],squareVertices[4],squareVertices[5],squareVertices[6],squareVertices[7], gldisp->backingWidth, gldisp->backingHeight,screenW,screenH);
    
	GL_OPERATION(glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices))
	GL_OPERATION(glEnableVertexAttribArray(ATTRIB_VERTEX))
	GL_OPERATION(glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, 1, 0, squareUvs))
	GL_OPERATION(glEnableVertexAttribArray(ATTRIB_UV))
    
	GL_OPERATION(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4))   
    
}

void ogl_display_render(struct opengles_display* gldisp, int orientation, int contentMode) {
   ogl_display_render_type(gldisp, REMOTE_IMAGE, true, 0, 0, 1, 1, orientation, contentMode);
    // preview image already have the correct orientation
	//hubin ogl_display_render_type(gldisp, PREVIEW_IMAGE, FALSE, 0.4f, -0.4f, 0.2f, 0.2f, 0);
}

static void check_GL_errors(const char* context) {
	 int maxIterations=10;
    GLenum error;
    while (((error = glGetError()) != GL_NO_ERROR) && maxIterations > 0)
    {
        switch(error)
        {
            case GL_INVALID_ENUM:
                printf("[%2d]GL error: '%s' -> GL_INVALID_ENUM\n", maxIterations, context);
                WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "[%2d]GL error: '%s' -> GL_INVALID_ENUM\n", maxIterations, context); break;
            case GL_INVALID_VALUE:
                printf("[%2d]GL error: '%s' -> GL_INVALID_VALUE\n", maxIterations, context);
                WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "[%2d]GL error: '%s' -> GL_INVALID_VALUE\n", maxIterations, context); break;
            case GL_INVALID_OPERATION:
                printf("[%2d]GL error: '%s' -> GL_INVALID_OPERATION\n", maxIterations, context);
                WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "[%2d]GL error: '%s' -> GL_INVALID_OPERATION\n", maxIterations, context); break;
            case GL_OUT_OF_MEMORY:
                printf("[%2d]GL error: '%s' -> GL_OUT_OF_MEMORY\n", maxIterations, context);
                WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "[%2d]GL error: '%s' -> GL_OUT_OF_MEMORY\n", maxIterations, context); break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                printf("[%2d]GL error: '%s' -> GL_INVALID_FRAMEBUFFER_OPERATION\n", maxIterations, context);
                WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "[%2d]GL error: '%s' -> GL_INVALID_FRAMEBUFFER_OPERATION\n", maxIterations, context); break;
            default:
                printf("[%2d]GL error: '%s' -> %x\n", maxIterations, context, error);
                WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "[%2d]GL error: '%s' -> %x\n", maxIterations, context, error);
        }
        maxIterations--;
    }
}

static bool load_shaders(GLuint* program, GLint* uniforms) {
#include "yuv2rgb.vs.h"
#include "yuv2rgb.fs.h"
	yuv2rgb_fs_len = yuv2rgb_fs_len;
	yuv2rgb_vs_len = yuv2rgb_vs_len;
	
    GLuint vertShader, fragShader;
    *program = glCreateProgram();

    if (!compileShader(&vertShader, GL_VERTEX_SHADER, (const char*)yuv2rgb_vs))
        return false;
    if (!compileShader(&fragShader, GL_FRAGMENT_SHADER, (const char*)yuv2rgb_fs))
        return false;

    GL_OPERATION(glAttachShader(*program, vertShader))
    GL_OPERATION(glAttachShader(*program, fragShader))

    GL_OPERATION(glBindAttribLocation(*program, ATTRIB_VERTEX, "position"))
    GL_OPERATION(glBindAttribLocation(*program, ATTRIB_UV, "uv"))

    if (!linkProgram(*program))
        return false;

    uniforms[UNIFORM_PROJ_MATRIX] = glGetUniformLocation(*program, "proj_matrix");
    uniforms[UNIFORM_ROTATION] = glGetUniformLocation(*program, "rotation");
    uniforms[UNIFORM_TEXTURE_Y] = glGetUniformLocation(*program, "t_texture_y");
    uniforms[UNIFORM_TEXTURE_U] = glGetUniformLocation(*program, "t_texture_u");
    uniforms[UNIFORM_TEXTURE_V] = glGetUniformLocation(*program, "t_texture_v");

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return true;
}

static void load_orthographic_matrix(float left, float right, float bottom, float top, float near, float far, float* mat)
{
    float r_l = right - left;
    float t_b = top - bottom;
    float f_n = far - near;
    float tx = - (right + left) / (right - left);
    float ty = - (top + bottom) / (top - bottom);
    float tz = - (far + near) / (far - near);

    mat[0] = (2.0f / r_l);
    mat[1] = mat[2] = mat[3] = 0.0f;

    mat[4] = 0.0f;
    mat[5] = (2.0f / t_b);
    mat[6] = mat[7] = 0.0f;

    mat[8] = mat[9] = 0.0f;
    mat[10] = -2.0f / f_n;
    mat[11] = 0.0f;

    mat[12] = tx;
    mat[13] = ty;
    mat[14] = tz;
    mat[15] = 1.0f;
}

static void allocate_gl_textures(struct opengles_display* gldisp, int w, int h, enum ImageType type) {
	GL_OPERATION(glActiveTexture(GL_TEXTURE0))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][Y]))
    GL_OPERATION(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST))
    GL_OPERATION(glGenerateMipmap(GL_TEXTURE_2D))
    GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0))
    GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
	GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0))

	GL_OPERATION(glActiveTexture(GL_TEXTURE1))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][U]))
    GL_OPERATION(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST))
    GL_OPERATION(glGenerateMipmap(GL_TEXTURE_2D))
    GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w >> 1, h >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
	GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w >> 1, h >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0))

	GL_OPERATION(glActiveTexture(GL_TEXTURE2))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][V]))
    GL_OPERATION(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST))
    GL_OPERATION(glGenerateMipmap(GL_TEXTURE_2D))
    GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w >> 1, h >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0))
    GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
    GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
	GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
	GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w >> 1, h >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0))

	gldisp->allocatedTexturesSize[type].width =  w;
	gldisp->allocatedTexturesSize[type].height =  h;

    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, 0, "%s: allocated new textures[%d] (%d x %d)\n", __FUNCTION__, type, w, h);
}

static unsigned int align_on_power_of_2(unsigned int value) {
	int i;
	/* browse all power of 2 value, and find the one just >= value */
	for(i=0; i<32; i++) {
		unsigned int c = 1 << i;
		if (value <= c)
			return c;
	}
	return 0;
}

static bool update_textures_with_yuv(struct opengles_display* gldisp, enum ImageType type) {
	unsigned int aligned_yuv_w, aligned_yuv_h;
	MSPicture yuvbuf;
    
    yuv_buf_init(&yuvbuf, gldisp->yuv[type]->w, gldisp->yuv[type]->h, gldisp->yuv[type]->data_ptr);
    
	if (yuvbuf.w == 0 || yuvbuf.h == 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, 0, "Incoherent image size: %dx%d\n", yuvbuf.w, yuvbuf.h);
		return false;
	}
	aligned_yuv_w = align_on_power_of_2(yuvbuf.w);
	aligned_yuv_h = align_on_power_of_2(yuvbuf.h);

	/* check if we need to adjust texture sizes */
	if (aligned_yuv_w != gldisp->allocatedTexturesSize[type].width ||
		aligned_yuv_h != gldisp->allocatedTexturesSize[type].height) {
		allocate_gl_textures(gldisp, aligned_yuv_w, aligned_yuv_h, type);
	}
	gldisp->uvx[type] = yuvbuf.w / (float)(gldisp->allocatedTexturesSize[type].width+1);
	gldisp->uvy[type] = yuvbuf.h / (float)(gldisp->allocatedTexturesSize[type].height+1);

	/* upload Y plane */
	GL_OPERATION(glActiveTexture(GL_TEXTURE0))
    GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][Y]))
	GL_OPERATION(glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0, yuvbuf.w, yuvbuf.h,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvbuf.planes[Y]))
	GL_OPERATION(glUniform1i(gldisp->uniforms[UNIFORM_TEXTURE_Y], 0))

	/* upload U plane */
	GL_OPERATION(glActiveTexture(GL_TEXTURE1))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][U]))
	GL_OPERATION(glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0, yuvbuf.w >> 1, yuvbuf.h >> 1,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvbuf.planes[U]))
	GL_OPERATION(glUniform1i(gldisp->uniforms[UNIFORM_TEXTURE_U], 1))

	/* upload V plane */
	GL_OPERATION(glActiveTexture(GL_TEXTURE2))
	GL_OPERATION(glBindTexture(GL_TEXTURE_2D, gldisp->textures[type][V]))
	GL_OPERATION(glTexSubImage2D(GL_TEXTURE_2D, 0,
			0, 0, yuvbuf.w >> 1, yuvbuf.h >> 1,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvbuf.planes[V]))
	GL_OPERATION(glUniform1i(gldisp->uniforms[UNIFORM_TEXTURE_V], 2))

	gldisp->yuv_size[type].width = yuvbuf.w;
	gldisp->yuv_size[type].height = yuvbuf.h;

	return true;
}

void ogl_display_zoom(struct opengles_display* gldisp, float* params) {
	gldisp->zoom_factor = params[0];
	gldisp->zoom_cx = params[1] - 0.5;
	gldisp->zoom_cy = params[2] - 0.5;
}

static void yuv_buf_init(MSPicture *buf, int w, int h, uint8_t *ptr){
	int ysize,usize;
	ysize=w*(h & 0x1 ? h +1 : h);
	usize=ysize/4;
	buf->w=w;
	buf->h=h;
	buf->planes[0]=ptr;
	buf->planes[1]=buf->planes[0]+ysize;
	buf->planes[2]=buf->planes[1]+usize;
	buf->planes[3]=0;
	buf->strides[0]=w;
	buf->strides[1]=w/2;
	buf->strides[2]=buf->strides[1];
	buf->strides[3]=0;
#if DEBUG_OPENGLES_DISPLAY
    if (g_file_opengles_display) {
        fwrite(ptr, ysize+usize+usize, 1, g_file_opengles_display);
        fflush(g_file_opengles_display);
    }
#endif
    
}
