//
// bg3d.h
//

#ifndef __BG3D
#define __BG3D

#include "metaobjects.h"
#include "skeletonobj.h"

#define	MAX_MULTITEXTURE_LAYERS		4			// max # of multi texture layers supported
												// WARNING: changing this may alter file format!!
												
#define	MAX_BG3D_MATERIALS			400			// max # of materials in a bg3d file

#define	MAX_BG3D_GROUPS			((int)MODEL_GROUP_SKELETONBASE+(int)MAX_SKELETON_TYPES)	// skeletons are @ end of list, so can use these counts for max #
#define	MAX_OBJECTS_IN_GROUP	100

		/***********************/
		/* BG3D FILE CONTAINER */
		/***********************/

typedef struct
{
	int					numMaterials;
	MOMaterialObject	*materials[MAX_BG3D_MATERIALS];	// references to all of the materials used in file
	MetaObjectPtr		root;							// the root object or group containing all geometry in file
}BG3DFileContainer;



		/* BG3D HEADER */
		
typedef struct
{
	char			headerString[16];			// header string
	NumVersion		version;					// version of file
}BG3DHeaderType;


	/* BG3D MATERIAL FLAGS */
		
enum
{
	BG3D_MATERIALFLAG_TEXTURED		= 	1,
	BG3D_MATERIALFLAG_ALWAYSBLEND	=	(1<<1),	// set if always want to GL_BLEND this texture when drawn
	BG3D_MATERIALFLAG_CLAMP_U		=	(1<<2),
	BG3D_MATERIALFLAG_CLAMP_V		=	(1<<3),
	BG3D_MATERIALFLAG_MULTITEXTURE	=	(1<<4)
	
};


		/* TAG TYPES */
		
enum
{
	BG3D_TAGTYPE_MATERIALFLAGS,
	BG3D_TAGTYPE_MATERIALDIFFUSECOLOR,
	BG3D_TAGTYPE_TEXTUREMAP,
	BG3D_TAGTYPE_GROUPSTART,
	BG3D_TAGTYPE_GROUPEND,	
	BG3D_TAGTYPE_GEOMETRY,
	BG3D_TAGTYPE_VERTEXARRAY,
	BG3D_TAGTYPE_NORMALARRAY,
	BG3D_TAGTYPE_UVARRAY,
	BG3D_TAGTYPE_COLORARRAY,
	BG3D_TAGTYPE_TRIANGLEARRAY,
	BG3D_TAGTYPE_ENDFILE
};


typedef struct
{
	uint32_t	width,height;					// dimensions of texture
	int32_t		srcPixelFormat;					// OGL format (GL_RGBA, etc.) for internal
	int32_t		dstPixelFormat;					// format for VRAM
	uint32_t	bufferSize;						// size of texture data to follow
	uint32_t	reserved[4];					// for future use
}BG3DTextureHeader;


	/* GEOMETRY TYPES */
	
enum
{
	BG3D_GEOMETRYTYPE_VERTEXELEMENTS

};


		/* BG3D GEOMETRY HEADER */
		
typedef struct
{
	uint32_t	type;								// geometry type
	int32_t		numMaterials;						// # material layers
	uint32_t	layerMaterialNum[MAX_MULTITEXTURE_LAYERS];	// index into material list
	uint32_t	flags;								// flags
	uint32_t	numPoints;							// (if applicable)
	uint32_t	numTriangles;						// (if applicable)
	uint32_t	reserved[4];						// for future use
}BG3DGeometryHeader;


//-----------------------------------


void InitBG3DManager(void);
void ImportBG3D(FSSpec *spec, int groupNum);
void DisposeBG3DContainer(int groupNum);
void DisposeAllBG3DContainers(void);
void BG3D_SetContainerMaterialFlags(short group, short type, short geometryNum, u_long flags);
void	ConvertTexture24To16(u_char *srcBuff24, u_short *destBuff16, int width, int height);
void BG3D_SphereMapGeomteryMaterial(short group, short type, short geometryNum, u_short combineMode, u_short envMapNum);
void SetSphereMapInfoOnVertexArrayData(MOVertexArrayData *va, u_short combineMode, u_short envMapNum);
void BG3D_PlaneMapGeomteryMaterial(short group, short type, short geometryNum, u_short combineMode, u_short envMapNum);

#endif

