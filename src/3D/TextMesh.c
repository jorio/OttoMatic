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

#define ATLAS_WIDTH 1024
#define ATLAS_HEIGHT 1024

#define TAB_STOP 60.0f

const uint16_t kMacRomanToUnicode[256] =
{
0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
' ',    '!',    '"',    '#',    '$',    '%',    '&',    '\'',   '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
'0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
'@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',    'X',    'Y',    'Z',    '[',    '\\',   ']',    '^',    '_',
'`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
'p',    'q',    'r',    's',    't',    'u',    'v',    'w',    'x',    'y',    'z',    '{',    '|',    '}',    '~',    0x007f,
0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,
0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8,
0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,
0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,
};

/****************************/
/*    VARIABLES             */
/****************************/

// The font material must be reloaded everytime a new GL context is created
static MetaObjectPtr gFontMaterial = NULL;

static bool gFontMetricsLoaded = false;
static float gFontLineHeight = 0;
static AtlasGlyph gAtlasGlyphs[256];

#pragma mark -

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
		int codePoint = 0;
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

		int macRomanCodePoint = 0;
		for (int j = 0; j < 256; j++)
		{
			if (kMacRomanToUnicode[j] == codePoint)
			{
				macRomanCodePoint = j;
				break;
			}
		}

		if (macRomanCodePoint != 0)
			gAtlasGlyphs[macRomanCodePoint] = newGlyph;

		ParseSFL_SkipLine(&data);
	}

	// Force monospaced numbers
	AtlasGlyph referenceNumber = gAtlasGlyphs['4'];
	for (int c = '0'; c <= '9'; c++)
	{
		gAtlasGlyphs[c].xoff += (referenceNumber.w - gAtlasGlyphs[c].w) / 2.0f;
		gAtlasGlyphs[c].xadv = referenceNumber.xadv;
	}
}

/***************************************************************/
/*                       INIT/SHUTDOWN                         */
/***************************************************************/

void TextMesh_Init(OGLSetupOutputType* setupInfo)
{
	OSErr err;
	FSSpec spec;
	short refNum;

		/* CREATE FONT MATERIAL */

	GAME_ASSERT_MESSAGE(!gFontMaterial, "gFontMaterial already created");
	MOMaterialData matData;
	memset(&matData, 0, sizeof(matData));
	matData.setupInfo		= setupInfo;
	matData.flags			= BG3D_MATERIALFLAG_ALWAYSBLEND | BG3D_MATERIALFLAG_TEXTURED | BG3D_MATERIALFLAG_CLAMP_U | BG3D_MATERIALFLAG_CLAMP_V;
	matData.diffuseColor	= (OGLColorRGBA) {1, 1, 1, 1};
	matData.numMipmaps		= 1;
	matData.width			= (u_long) ATLAS_WIDTH;
	matData.height			= (u_long) ATLAS_HEIGHT;
	matData.textureName[0]	= OGL_TextureMap_LoadTGA(":system:font.tga", 0);
	gFontMaterial = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);

		/* LOAD METRICS FILE */

	if (!gFontMetricsLoaded)
	{
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
		ParseSFL(data);

		// Nuke data buffer
		SafeDisposePtr(data);

		gFontMetricsLoaded = true;
	}
}

void TextMesh_Shutdown(void)
{
	MO_DisposeObjectReference(gFontMaterial);
	gFontMaterial = NULL;
}

/***************************************************************/
/*                MESH ALLOCATION/LAYOUT                       */
/***************************************************************/

static MOVertexArrayData TextMesh_AllocateMesh(int numQuads)
{
	MOVertexArrayData mesh;
	memset(&mesh, 0, sizeof(mesh));
	mesh.numMaterials = 1;
	mesh.materials[0] = MO_GetNewReference(gFontMaterial);
	mesh.numTriangles = numQuads*2;
	mesh.numPoints = numQuads*4;
	mesh.points = (OGLPoint3D *)AllocPtr(sizeof(OGLPoint3D) * mesh.numPoints);
	mesh.uvs[0] = (OGLTextureCoord *)AllocPtr(sizeof(OGLTextureCoord) * mesh.numPoints);
	mesh.triangles = (MOTriangleIndecies *)AllocPtr(sizeof(MOTriangleIndecies) * mesh.numTriangles);
	return mesh;
}

static MOVertexArrayData TextMesh_SetMesh(const char* text, int align, MOVertexArrayData* recycleMesh)
{
	float S = .5f;
	float x = 0;
	float y = 0;
	float z = 0;
	float spacing = 0;

//	GAME_ASSERT(gAtlasGlyphs);
	GAME_ASSERT(gFontMaterial);

	// Compute number of quads and line width
	float lineWidth = 0;
	int numQuads = 0;
	for (const char* c = text; *c; c++)
	{
		if (*c == '\n')		// TODO: line widths for strings containing line breaks aren't supported yet
			continue;
		if (*c == '\t')		// TODO: line widths for strings containing tabs aren't supported yet
			continue;

		const AtlasGlyph g = gAtlasGlyphs[(uint8_t)*c];
		lineWidth += S*(g.xadv + spacing);
		if (*c != ' ')
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

	// Create the mesh
	MOVertexArrayData mesh;
	if (recycleMesh)
	{
		mesh = *recycleMesh;
		GAME_ASSERT(mesh.numTriangles >= numQuads*2);
		GAME_ASSERT(mesh.numPoints >= numQuads*4);
		GAME_ASSERT(mesh.uvs);
		GAME_ASSERT(mesh.triangles);
		GAME_ASSERT(mesh.materials[0]);
		mesh.numTriangles = numQuads*2;
		mesh.numPoints = numQuads*4;
	}
	else
	{
		mesh = TextMesh_AllocateMesh(numQuads);
	}

	// Create a quad for each character
	int t = 0;
	int p = 0;
	for (const char* c = text; *c; c++)
	{
		if (*c == '\n')
		{
			x = x0;
			y += gFontLineHeight * S;
			continue;
		}

		if (*c == '\t')
		{
			x = TAB_STOP * ceilf((x+1.0f) / TAB_STOP);
			continue;
		}

		const AtlasGlyph g = gAtlasGlyphs[(uint8_t)*c];

		if (*c == ' ')
		{
			x += S*(g.xadv + spacing);
			continue;
		}

		float qx = x + S*(g.xoff + g.w*.5f);
		float qy = y + S*(g.yoff + g.h*.5f);

		mesh.triangles[t + 0].vertexIndices[0] = p + 0;
		mesh.triangles[t + 0].vertexIndices[1] = p + 1;
		mesh.triangles[t + 0].vertexIndices[2] = p + 2;
		mesh.triangles[t + 1].vertexIndices[0] = p + 0;
		mesh.triangles[t + 1].vertexIndices[1] = p + 2;
		mesh.triangles[t + 1].vertexIndices[2] = p + 3;
		mesh.points[p + 0] = (OGLPoint3D) { qx - S*g.w*.5f, qy - S*g.h*.5f, z };
		mesh.points[p + 1] = (OGLPoint3D) { qx + S*g.w*.5f, qy - S*g.h*.5f, z };
		mesh.points[p + 2] = (OGLPoint3D) { qx + S*g.w*.5f, qy + S*g.h*.5f, z };
		mesh.points[p + 3] = (OGLPoint3D) { qx - S*g.w*.5f, qy + S*g.h*.5f, z };
		mesh.uvs[0][p + 0] = (OGLTextureCoord) { g.x/ATLAS_WIDTH,		g.y/ATLAS_HEIGHT };
		mesh.uvs[0][p + 1] = (OGLTextureCoord) { (g.x+g.w)/ATLAS_WIDTH,	g.y/ATLAS_HEIGHT };
		mesh.uvs[0][p + 2] = (OGLTextureCoord) { (g.x+g.w)/ATLAS_WIDTH,	(g.y+g.h)/ATLAS_HEIGHT };
		mesh.uvs[0][p + 3] = (OGLTextureCoord) { g.x/ATLAS_WIDTH,		(g.y+g.h)/ATLAS_HEIGHT };

		x += S*(g.xadv + spacing);
		t += 2;
		p += 4;
	}

	GAME_ASSERT(p == mesh.numPoints);

	return mesh;
}

/***************************************************************/
/*                    API IMPLEMENTATION                       */
/***************************************************************/

ObjNode *TextMesh_NewEmpty(int capacity, NewObjectDefinitionType* newObjDef)
{
	MOVertexArrayData mesh = TextMesh_AllocateMesh(capacity);
	mesh.numPoints = 0;
	mesh.numTriangles = 0;

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
	ObjNode* textNode = TextMesh_NewEmpty(strlen(text), newObjDef);
	TextMesh_Update(text, align, textNode);
	return textNode;
}

void TextMesh_Update(const char* text, int align, ObjNode* textNode)
{
	GAME_ASSERT(textNode->Genre == TEXTMESH_GENRE);
	GAME_ASSERT(textNode->BaseGroup);
	GAME_ASSERT(textNode->BaseGroup->objectData.numObjectsInGroup >= 2);

	MetaObjectPtr object = textNode->BaseGroup->objectData.groupContents[1];
	MetaObjectHeader* objHead = object;
	MOVertexArrayObject* vObj = object;

	GAME_ASSERT(objHead->type == MO_TYPE_GEOMETRY);
	GAME_ASSERT(objHead->subType == MO_GEOMETRY_SUBTYPE_VERTEXARRAY);

	// Reset capacity
	vObj->objectData.numPoints = 4 * textNode->TextQuadCapacity;
	vObj->objectData.numTriangles = 2 * textNode->TextQuadCapacity;

	vObj->objectData = TextMesh_SetMesh(text, align, &vObj->objectData);
}

float TextMesh_GetCharX(const char* text, int n)
{
	float x = 0;
	for (const char *c = text; *c && n; c++, n--)
	{
		if (*c == '\n')		// TODO: line widths for strings containing line breaks aren't supported yet
			continue;

		x += gAtlasGlyphs[(uint8_t)*c].xadv;
	}
	return x;
}
