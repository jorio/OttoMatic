// TEXT MESH.C
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include <stdio.h>

/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct
{
	float x;
	float y;
	float w;
	float h;
	float xoff;
	float yoff;
	float xadv;
} AtlasGlyph;

/****************************/
/*    CONSTANTS             */
/****************************/

// This covers the basic multilingual plane (0000-FFFF)
#define MAX_CODEPOINT_PAGES 256

#define ATLAS_WIDTH 1024
#define ATLAS_HEIGHT 1024

#define TAB_STOP 60.0f

/****************************/
/*    VARIABLES             */
/****************************/

// The font material must be reloaded everytime a new GL context is created
static MetaObjectPtr gFontMaterial = NULL;

static bool gFontMetricsLoaded = false;
static float gFontLineHeight = 0;

static AtlasGlyph* gAtlasGlyphsPages[MAX_CODEPOINT_PAGES];

#pragma mark -

/***************************************************************/
/*                         UTF-8                               */
/***************************************************************/

static uint32_t ReadNextCodepointFromUTF8(const char** utf8TextPtr)
{
#define TRY_ADVANCE(t) do { if (!*t) return 0; else t++; } while(0)

	uint32_t codepoint = 0;
	const uint8_t* t = (const uint8_t*) *utf8TextPtr;

	if ((*t & 0b10000000) == 0)
	{
		// 1 byte code point, ASCII
		codepoint |= (*t & 0b01111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 1;
	}
	else if ((*t & 0b11100000) == 0b11000000)
	{
		// 2 byte code point
		codepoint |= (*t & 0b00011111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 2;
	}
	else if ((**utf8TextPtr & 0b11110000) == 0b11100000)
	{
		// 3 byte code point
		codepoint |= (*t & 0b00001111) << 12;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);
		*utf8TextPtr += 3;
	}
	else
	{
		// 4 byte code point
		codepoint |= (*t & 0b00000111) << 18;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 12;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111) << 6;	TRY_ADVANCE(t);
		codepoint |= (*t & 0b00111111);			TRY_ADVANCE(t);
		*utf8TextPtr += 4;
	}

	return codepoint;

#undef TRY_ADVANCE
}

/***************************************************************/
/*                       PARSE SFL                             */
/***************************************************************/

static void ParseSFL_SkipLine(const char** dataPtr)
{
	const char* data = *dataPtr;

	while (*data)
	{
		char c = data[0];
		data++;
		if (c == '\r' && *data != '\n')
			break;
		if (c == '\n')
			break;
	}

	GAME_ASSERT(*data);
	*dataPtr = data;
}

// Parse an SFL file produced by fontbuilder
static void ParseSFL(const char* data)
{
	int nArgs = 0;
	int nGlyphs = 0;
	int junk = 0;

	ParseSFL_SkipLine(&data);	// Skip font name

	nArgs = sscanf(data, "%d %f", &junk, &gFontLineHeight);
	GAME_ASSERT(nArgs == 2);
	ParseSFL_SkipLine(&data);

	ParseSFL_SkipLine(&data);	// Skip image filename

	nArgs = sscanf(data, "%d", &nGlyphs);
	GAME_ASSERT(nArgs == 1);
	ParseSFL_SkipLine(&data);

	for (int i = 0; i < nGlyphs; i++)
	{
		AtlasGlyph newGlyph;
		uint32_t codePoint = 0;
		nArgs = sscanf(
				data,
				"%d %f %f %f %f %f %f %f",
				&codePoint,
				&newGlyph.x,
				&newGlyph.y,
				&newGlyph.w,
				&newGlyph.h,
				&newGlyph.xoff,
				&newGlyph.yoff,
				&newGlyph.xadv);
		GAME_ASSERT(nArgs == 8);

		uint32_t codePointPage = codePoint >> 8;
		if (codePointPage >= MAX_CODEPOINT_PAGES)
		{
			printf("WARNING: codepoint 0x%x exceeds supported maximum (0x%x)\n", codePoint, MAX_CODEPOINT_PAGES * 256 - 1);
			continue;
		}

		if (gAtlasGlyphsPages[codePointPage] == NULL)
		{
			gAtlasGlyphsPages[codePointPage] = AllocPtrClear(sizeof(AtlasGlyph) * 256);
		}

		gAtlasGlyphsPages[codePointPage][codePoint & 0xFF] = newGlyph;

		ParseSFL_SkipLine(&data);
	}

	// Force monospaced numbers
	AtlasGlyph* asciiPage = gAtlasGlyphsPages[0];
	AtlasGlyph referenceNumber = asciiPage['4'];
	for (int c = '0'; c <= '9'; c++)
	{
		asciiPage[c].xoff += (referenceNumber.w - asciiPage[c].w) / 2.0f;
		asciiPage[c].xadv = referenceNumber.xadv;
	}
}

/***************************************************************/
/*                       INIT/SHUTDOWN                         */
/***************************************************************/

void TextMesh_LoadMetrics(void)
{
	OSErr err;
	FSSpec spec;
	short refNum;

	GAME_ASSERT_MESSAGE(!gFontMetricsLoaded, "Metrics already loaded");

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":system:font.sfl", &spec);
	GAME_ASSERT(!err);
	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT(!err);

	// Get number of bytes until EOF
	long eof = 0;
	GetEOF(refNum, &eof);

	// Prep data buffer
	Ptr data = AllocPtrClear(eof+1);

	// Read file into data buffer
	err = FSRead(refNum, &eof, data);
	GAME_ASSERT(err == noErr);
	FSClose(refNum);

	// Parse metrics (gAtlasGlyphs) from SFL file
	memset(gAtlasGlyphsPages, 0, sizeof(gAtlasGlyphsPages));
	ParseSFL(data);

	// Nuke data buffer
	SafeDisposePtr(data);

	gFontMetricsLoaded = true;
}

void TextMesh_InitMaterial(bool redFont)
{
		/* CREATE FONT MATERIAL */

	GAME_ASSERT_MESSAGE(!gFontMaterial, "gFontMaterial already created");
	MOMaterialData matData;
	memset(&matData, 0, sizeof(matData));
	matData.setupInfo		= gGameViewInfoPtr;
	matData.flags			= BG3D_MATERIALFLAG_ALWAYSBLEND | BG3D_MATERIALFLAG_TEXTURED | BG3D_MATERIALFLAG_CLAMP_U | BG3D_MATERIALFLAG_CLAMP_V;
	matData.diffuseColor	= (OGLColorRGBA) {1, 1, 1, 1};
	matData.numMipmaps		= 1;
	matData.width			= (uint32_t) ATLAS_WIDTH;
	matData.height			= (uint32_t) ATLAS_HEIGHT;
	matData.textureName[0]	= OGL_TextureMap_LoadTGA(redFont? ":system:font2.tga": ":system:font1.tga", 0, nil, nil);
	gFontMaterial = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
}

void TextMesh_DisposeMaterial(void)
{
	MO_DisposeObjectReference(gFontMaterial);
	gFontMaterial = NULL;
}

void TextMesh_DisposeMetrics(void)
{
	if (!gFontMetricsLoaded)
		return;

	gFontMetricsLoaded = false;

	for (int i = 0; i < MAX_CODEPOINT_PAGES; i++)
	{
		if (gAtlasGlyphsPages[i])
		{
			SafeDisposePtr((Ptr) gAtlasGlyphsPages[i]);
			gAtlasGlyphsPages[i] = NULL;
		}
	}
}

/***************************************************************/
/*                MESH ALLOCATION/LAYOUT                       */
/***************************************************************/

static void TextMesh_ReallocateMesh(MOVertexArrayData* mesh, int numQuads)
{
	if (mesh->points)
	{
		SafeDisposePtr((Ptr) mesh->points);
		mesh->points = nil;
	}

	if (mesh->uvs[0])
	{
		SafeDisposePtr((Ptr) mesh->uvs[0]);
		mesh->uvs[0] = nil;
	}

	if (mesh->triangles)
	{
		SafeDisposePtr((Ptr) mesh->triangles);
		mesh->triangles = nil;
	}

	int numPoints = numQuads * 4;
	int numTriangles = numQuads * 2;

	if (numQuads != 0)
	{
		mesh->points = (OGLPoint3D *) AllocPtr(sizeof(OGLPoint3D) * numPoints);
		mesh->uvs[0] = (OGLTextureCoord *) AllocPtr(sizeof(OGLTextureCoord) * numPoints);
		mesh->triangles = (MOTriangleIndecies *) AllocPtr(sizeof(MOTriangleIndecies) * numTriangles);
	}
}

static void TextMesh_InitMesh(MOVertexArrayData* mesh, int numQuads)
{
	memset(mesh, 0, sizeof(*mesh));
	
	GAME_ASSERT(gFontMaterial);
	mesh->numMaterials = 1;
	mesh->materials[0] = gFontMaterial;

	TextMesh_ReallocateMesh(mesh, numQuads);
}

static const AtlasGlyph* GetGlyphFromCodepoint(uint32_t c)
{
	uint32_t page = c >> 8;

	if (page >= MAX_CODEPOINT_PAGES)
	{
		page = 0; // ascii
		c = '?';
	}

	if (!gAtlasGlyphsPages[page])
	{
		page = 0; // ascii
		c = '#';
	}

	return &gAtlasGlyphsPages[page][c & 0xFF];
}

void TextMesh_Update(const char* text, int align, ObjNode* textNode)
{
	GAME_ASSERT_MESSAGE(gFontMaterial, "Can't lay out text if the font material isn't loaded!");
	GAME_ASSERT_MESSAGE(gFontMetricsLoaded, "Can't lay out text if the font metrics aren't loaded!");

	//-----------------------------------
	// Get mesh from ObjNode

	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);
	GAME_ASSERT(textNode->BaseGroup);
	GAME_ASSERT(textNode->BaseGroup->objectData.numObjectsInGroup >= 2);

	MetaObjectPtr			metaObject			= textNode->BaseGroup->objectData.groupContents[1];
	MetaObjectHeader*		metaObjectHeader	= metaObject;
	MOVertexArrayObject*	vertexObject		= metaObject;
	MOVertexArrayData*		mesh				= &vertexObject->objectData;

	GAME_ASSERT(metaObjectHeader->type == MO_TYPE_GEOMETRY);
	GAME_ASSERT(metaObjectHeader->subType == MO_GEOMETRY_SUBTYPE_VERTEXARRAY);

	//-----------------------------------

	float S = .5f;
	float x = 0;
	float y = 0;
	float z = 0;
	float spacing = 0;

	// Compute number of quads and line width
	float lineWidth = 0;
	int numQuads = 0;
	for (const char* utftext = text; *utftext; )
	{
		uint32_t c = ReadNextCodepointFromUTF8(&utftext);

		if (c == '\n')		// TODO: line widths for strings containing line breaks aren't supported yet
		{
			lineWidth = 0;
			continue;
		}

		if (c == '\t')
		{
			lineWidth = TAB_STOP * ceilf((lineWidth+1.0f) / TAB_STOP);
			continue;
		}

		const AtlasGlyph* g = GetGlyphFromCodepoint(c);
		lineWidth += S*(g->xadv + spacing);
		if (c != ' ')
			numQuads++;
	}

	// Adjust start x for text alignment
	if (align == kTextMeshAlignCenter)
		x -= lineWidth * .5f;
	else if (align == kTextMeshAlignRight)
		x -= lineWidth;

	float x0 = x;

	// Adjust y for ascender
	y -= S*(gFontLineHeight * .3f);

	// Save extents
	textNode->LeftOff	= x;
	textNode->RightOff	= x + lineWidth;
	textNode->TopOff	= y + S*gFontLineHeight*0.3f;
	textNode->BottomOff	= y + S*gFontLineHeight*1.25f;

	// Ensure mesh has capacity for quads
	if (textNode->TextQuadCapacity < numQuads)
	{
		textNode->TextQuadCapacity = numQuads * 2;		// avoid reallocating often if text keeps growing
		TextMesh_ReallocateMesh(mesh, textNode->TextQuadCapacity);
	}

	// Set # of triangles and points
	mesh->numTriangles = numQuads*2;
	mesh->numPoints = numQuads*4;

	GAME_ASSERT(mesh->numTriangles >= numQuads*2);
	GAME_ASSERT(mesh->numPoints >= numQuads*4);

	if (numQuads == 0)
		return;

	GAME_ASSERT(mesh->uvs);
	GAME_ASSERT(mesh->triangles);
	GAME_ASSERT(mesh->numMaterials == 1);
	GAME_ASSERT(mesh->materials[0]);

	// Create a quad for each character
	int t = 0;
	int p = 0;
	for (const char* utftext = text; *utftext; )
	{
		uint32_t c = ReadNextCodepointFromUTF8(&utftext);

		if (c == '\n')
		{
			x = x0;
			y += gFontLineHeight * S;
			continue;
		}

		if (c == '\t')
		{
			x = TAB_STOP * ceilf((x+1.0f) / TAB_STOP);
			continue;
		}

		const AtlasGlyph g = *GetGlyphFromCodepoint(c);

		if (c == ' ')
		{
			x += S*(g.xadv + spacing);
			continue;
		}

		float qx = x + S*(g.xoff + g.w*.5f);
		float qy = y + S*(g.yoff + g.h*.5f);

		mesh->triangles[t + 0].vertexIndices[0] = p + 0;
		mesh->triangles[t + 0].vertexIndices[1] = p + 1;
		mesh->triangles[t + 0].vertexIndices[2] = p + 2;
		mesh->triangles[t + 1].vertexIndices[0] = p + 0;
		mesh->triangles[t + 1].vertexIndices[1] = p + 2;
		mesh->triangles[t + 1].vertexIndices[2] = p + 3;
		mesh->points[p + 0] = (OGLPoint3D) { qx - S*g.w*.5f, qy - S*g.h*.5f, z };
		mesh->points[p + 1] = (OGLPoint3D) { qx + S*g.w*.5f, qy - S*g.h*.5f, z };
		mesh->points[p + 2] = (OGLPoint3D) { qx + S*g.w*.5f, qy + S*g.h*.5f, z };
		mesh->points[p + 3] = (OGLPoint3D) { qx - S*g.w*.5f, qy + S*g.h*.5f, z };
		mesh->uvs[0][p + 0] = (OGLTextureCoord) { g.x/ATLAS_WIDTH,		g.y/ATLAS_HEIGHT };
		mesh->uvs[0][p + 1] = (OGLTextureCoord) { (g.x+g.w)/ATLAS_WIDTH,	g.y/ATLAS_HEIGHT };
		mesh->uvs[0][p + 2] = (OGLTextureCoord) { (g.x+g.w)/ATLAS_WIDTH,	(g.y+g.h)/ATLAS_HEIGHT };
		mesh->uvs[0][p + 3] = (OGLTextureCoord) { g.x/ATLAS_WIDTH,		(g.y+g.h)/ATLAS_HEIGHT };

		x += S*(g.xadv + spacing);
		t += 2;
		p += 4;
	}

	GAME_ASSERT(p == mesh->numPoints);
}

/***************************************************************/
/*                    API IMPLEMENTATION                       */
/***************************************************************/

ObjNode *TextMesh_NewEmpty(int capacity, NewObjectDefinitionType* newObjDef)
{
	MOVertexArrayData mesh;
	TextMesh_InitMesh(&mesh, capacity);

	newObjDef->genre = TEXTMESH_GENRE;
	newObjDef->flags = STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOFOG | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_NOZBUFFER | STATUS_BIT_GLOW;
	ObjNode* textNode = MakeNewObject(newObjDef);

	// Attach color mesh
	MetaObjectPtr meshMO = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &mesh);

	CreateBaseGroup(textNode);
	AttachGeometryToDisplayGroupObject(textNode, meshMO);

	textNode->TextQuadCapacity = capacity;

	// Dispose of extra reference to mesh
	MO_DisposeObjectReference(meshMO);

	UpdateObjectTransforms(textNode);

	return textNode;
}

ObjNode *TextMesh_New(const char *text, int align, NewObjectDefinitionType* newObjDef)
{
	ObjNode* textNode = TextMesh_NewEmpty(0, newObjDef);
	TextMesh_Update(text, align, textNode);
	return textNode;
}

float TextMesh_GetCharX(const char* text, int n)
{
	float x = 0;
	for (const char *utftext = text;
		*utftext && n;
		n--)
	{
		uint32_t c = ReadNextCodepointFromUTF8(&utftext);

		if (c == '\n')		// TODO: line widths for strings containing line breaks aren't supported yet
			continue;

		x += GetGlyphFromCodepoint(c)->xadv;
	}
	return x;
}

OGLRect TextMesh_GetExtents(ObjNode* textNode)
{
	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);

	return (OGLRect)
	{
		.left		= textNode->Coord.x + textNode->Scale.x * textNode->LeftOff,
		.right		= textNode->Coord.x + textNode->Scale.x * textNode->RightOff,
		.top		= textNode->Coord.y + textNode->Scale.y * textNode->TopOff,
		.bottom		= textNode->Coord.y + textNode->Scale.y * textNode->BottomOff,
	};
}

void TextMesh_DrawExtents(ObjNode* textNode)
{
	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);

	OGL_PushState();								// keep state
	SetInfobarSpriteState(true);
	glDisable(GL_TEXTURE_2D);

	OGLRect extents = TextMesh_GetExtents(textNode);
	float z = textNode->Coord.z;

	glColor4f(1,1,1,1);
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents.left,		extents.top,	z);
	glVertex3f(extents.right,		extents.top,	z);
	glColor4f(0,.5,1,1);
	glVertex3f(extents.right,		extents.bottom,	z);
	glVertex3f(extents.left,		extents.bottom,	z);
	glEnd();

	OGL_PopState();
}
