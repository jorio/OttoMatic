//
// ogl_support.h
//

#pragma once

#define	MAX_FILL_LIGHTS		4
#define	MAX_TEXTURES		300


#define	USE_GL_COLOR_MATERIAL	1

#if USE_GL_COLOR_MATERIAL
	#define SetColor4fv(colorVV)		glColor4fv((GLfloat *)colorVV)		// set current diffuse color
	#define SetColor4f(r, g, b, a)		\
	do {																		\
		glColor4f(r, g, b, a);													\
		glEnable(GL_COLOR_MATERIAL);											\
	} while(0)
#else
	#define SetColor4fv(colorVV)		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, (GLfloat *)colorVV)
	#define SetColor4f(r, g, b, a) \
	do {																		\
		GLfloat	c[4];															\
		c[0] = r;	c[1] = g; c[2] = b; c[3] = a;								\
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, c);				\
	} while(0)
#endif

		/* 4x4 MATRIX INDECIES */
		//
		// These are in (row,col) format (standard math notation).
		// Example: M30 --> 4th row, 1st column.
		//
enum
{
	M00	= 0,
	M10,
	M20,
	M30,
	M01,
	M11,
	M21,
	M31,
	M02,
	M12,
	M22,
	M32,
	M03,
	M13,
	M23,
	M33
};

		/* 3x3 MATRIX INDECIES */
enum
{
	N00	= 0,
	N10,
	N20,
	N01,
	N11,
	N21,
	N02,
	N12,
	N22
};


		/* 3D STRUCTURES */
		
typedef struct
{
	float 	x,y,z,w;
}OGLPoint4D;
		
typedef struct
{
	GLfloat	x,y,z;
}OGLPoint3D;

typedef struct
{
	GLfloat	x,y;
}OGLPoint2D;

typedef struct
{
	GLfloat	x,y,z;
}OGLVector3D;

typedef struct
{
	GLfloat	x,y;
}OGLVector2D;

typedef struct
{
	GLfloat	u,v;
}OGLTextureCoord;

typedef struct
{
	GLfloat	r,g,b;
}OGLColorRGB;

typedef struct
{
	GLfloat	r,g,b,a;
}OGLColorRGBA;

typedef struct
{
	GLubyte	r,g,b,a;
}OGLColorRGBA_Byte;

typedef struct
{
	GLfloat	value[16];
}OGLMatrix4x4;

typedef struct
{
	GLfloat	value[9];
}OGLMatrix3x3;

typedef struct
{
	OGLVector3D 					normal;
	float 							constant;
}OGLPlaneEquation;

typedef struct
{
	OGLPoint3D			point;
	OGLTextureCoord		uv;
	OGLColorRGBA		color;
}OGLVertex;

typedef struct
{
	OGLPoint3D 						cameraLocation;				/*  Location point of the camera 	*/
	OGLPoint3D 						pointOfInterest;			/*  Point of interest 				*/
	OGLVector3D 					upVector;					/*  "up" vector 					*/
}OGLCameraPlacement;

typedef struct
{
	OGLPoint3D 			min;
	OGLPoint3D 			max;
	Boolean 			isEmpty;
}OGLBoundingBox;


typedef struct
{
	OGLPoint3D 			origin;
	float 				radius;
	Boolean 			isEmpty;
}OGLBoundingSphere;


typedef struct
{
	float	top,bottom,left,right;
}OGLRect;

//========================

typedef	struct
{
	Boolean					clearBackBuffer;
	OGLColorRGBA			clearColor;
	Rect					clip;			// left = amount to clip off left, etc.
}OGLViewDefType;


typedef	struct
{
	Boolean			usePhong;
	Boolean			useFog;
	float			fogStart;
	float			fogEnd;
	float			fogDensity;
	short			fogMode;
	Boolean			redFont;
}OGLStyleDefType;


typedef struct
{
	OGLPoint3D				from;
	OGLPoint3D				to;
	OGLVector3D				up;
	float					hither;
	float					yon;
	float					fov;
}OGLCameraDefType;

typedef	struct
{
	OGLColorRGBA		ambientColor;
	int					numFillLights;
	OGLVector3D			fillDirection[MAX_FILL_LIGHTS];
	OGLColorRGBA		fillColor[MAX_FILL_LIGHTS];
}OGLLightDefType;


		/* OGLSetupInputType */
		
typedef struct
{
	OGLViewDefType		view;
	OGLStyleDefType		styles;
	OGLCameraDefType	camera;
	OGLLightDefType		lights;
}OGLSetupInputType;


		/* OGLSetupOutputType */

typedef struct
{
	Boolean					isActive;
	Rect					clip;				// not pane size, but clip:  left = amount to clip off left
	OGLLightDefType			lightList;
	OGLCameraPlacement		cameraPlacement;	
	float					fov,hither,yon;
	Boolean					useFog;
	Boolean					clearBackBuffer;
}OGLSetupOutputType;


enum
{
	kLoadTextureFlags_GrayscaleIsAlpha = 1 << 0,
	kLoadTextureFlags_KeepOriginalAlpha = 1 << 1,
};


extern	float					gAnaglyphFocallength;
extern	float					gAnaglyphEyeSeparation;
extern	Byte					gAnaglyphPass;


//=====================================================================

void OGL_Boot(void);
void OGL_Shutdown(void);
void OGL_NewViewDef(OGLSetupInputType *viewDef);
void OGL_SetupWindow(OGLSetupInputType *viewDef);
void OGL_DisposeWindowSetup(void);
void OGL_DrawScene(void (*drawRoutine)(void));
void OGL_Camera_SetPlacementAndUpdateMatrices(void);
void OGL_MoveCameraFromTo(float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ);
void OGL_MoveCameraFrom(float fromDX, float fromDY, float fromDZ);
void OGL_UpdateCameraFromToUp(OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up);
void OGL_UpdateCameraFromTo(const OGLPoint3D *from, const OGLPoint3D *to);
void OGL_Texture_SetOpenGLTexture(GLuint textureName);
GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType);
GLuint OGL_TextureMap_LoadTGA(const char* path, int flags, int* width, int* height);
GLenum _OGL_CheckError(const char* file, int line);
#define OGL_CheckError() _OGL_CheckError(__FILE__, __LINE__)

void OGL_GetCurrentViewport(int *x, int *y, int *w, int *h);

void OGL_PushState(void);
void OGL_PopState(void);

void OGL_EnableLighting(void);
void OGL_DisableLighting(void);
