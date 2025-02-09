/*********************************/
/*    OBJECT MANAGER 		     */
/* By Brian Greenstone      	 */
/* (c)2001 Pangea Software       */
/* (c)2023 Iliyas Jorio          */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void FlushObjectDeleteQueue(void);
static void DrawCollisionBoxes(ObjNode *theNode, Boolean old);
static void DrawBoundingBoxes(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	OBJ_DEL_Q_SIZE	200

#define OBJ_BUDGET		700

/**********************/
/*     VARIABLES      */
/**********************/

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;

ObjNode		*gCurrentNode,*gMostRecentlyAddedNode,*gNextNode;

static ObjNode	gObjNodeMemory[OBJ_BUDGET];
static Pool*	gObjNodePool;


NewObjectDefinitionType	gNewObjectDefinition;

OGLPoint3D	gCoord;
OGLVector3D	gDelta;

long		gNumObjsInDeleteQueue = 0;
ObjNode		*gObjectDeleteQueue[OBJ_DEL_Q_SIZE];

float		gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac;

int			gNumObjectNodes;

OGLMatrix4x4	*gCurrentObjMatrix;

//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CREATION ------

/************************ INIT OBJECT MANAGER **********************/

void InitObjectManager(void)
{
		/* INIT LINKED LIST */

	gCurrentNode = nil;
	gFirstNodePtr = nil;									// no node yet
	gNumObjectNodes = 0;

		/* INIT OBJECT POOL */

	SDL_memset(gObjNodeMemory, 0, sizeof(gObjNodeMemory));
	gObjNodePool = Pool_New(OBJ_BUDGET);

		/* INIT NEW OBJ DEF */

	SDL_memset(&gNewObjectDefinition, 0, sizeof(gNewObjectDefinition));
	gNewObjectDefinition.scale = 1;

#if _DEBUG
	SDL_Log("ObjNode pool: %d KB", (int)(sizeof(gObjNodeMemory) / 1024));
#endif
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode* MakeNewObject(NewObjectDefinitionType* newObjDef)
{
ObjNode	*newNodePtr = NULL;
long	slot;
uint32_t flags = newObjDef->flags;

			/* TRY TO GET AN OBJECT FROM THE POOL */

	int poolIndex = Pool_AllocateIndex(gObjNodePool);
	if (poolIndex >= 0)
	{
		newNodePtr = &gObjNodeMemory[poolIndex];		// point to pooled node
	}
	else
	{
		// pool full, allocate new node on heap
		newNodePtr = (ObjNode*) AllocPtr(sizeof(ObjNode));
	}

			/* MAKE SURE WE GOT ONE */

	GAME_ASSERT(newNodePtr);

			/* INITIALIZE ALL OF THE FIELDS */

	SDL_memset(newNodePtr, 0, sizeof(ObjNode));

	slot = newObjDef->slot;

	newNodePtr->Slot 		= slot;
	newNodePtr->Type 		= newObjDef->type;
	newNodePtr->Group 		= newObjDef->group;
	newNodePtr->MoveCall 	= newObjDef->moveCall;
	newNodePtr->CustomDrawFunction = newObjDef->drawCall;

	if (flags & STATUS_BIT_ONSPLINE)
		newNodePtr->SplineMoveCall = newObjDef->moveCall;				// save spline move routine
	else
		newNodePtr->SplineMoveCall = nil;

	newNodePtr->Genre = newObjDef->genre;
	newNodePtr->Coord = newNodePtr->InitCoord = newNodePtr->OldCoord = newObjDef->coord;		// save coords
	newNodePtr->StatusBits = flags;

	for (int i = 0; i < MAX_NODE_SPARKLES; i++)							// no sparkles
		newNodePtr->Sparkles[i] = -1;


	newNodePtr->BBox.isEmpty = true;


	newNodePtr->Rot.y =  newObjDef->rot;
	newNodePtr->Scale.x =
	newNodePtr->Scale.y =
	newNodePtr->Scale.z = newObjDef->scale;


	newNodePtr->BoundingSphereRadius = 100;


	newNodePtr->EffectChannel = -1;						// no streaming sound effect
	newNodePtr->ParticleGroup = -1;						// no particle group
	newNodePtr->ParticleMagicNum = 0;

	newNodePtr->TerrainItemPtr = nil;					// assume not a terrain item
	newNodePtr->SplineItemPtr = nil;					// assume not a spline item
	newNodePtr->SplineNum = 0;
	newNodePtr->SplineObjectIndex = -1;					// no index yet
	newNodePtr->SplinePlacement = 0;

	newNodePtr->Skeleton = nil;

	newNodePtr->ColorFilter.r =
	newNodePtr->ColorFilter.g =
	newNodePtr->ColorFilter.b =
	newNodePtr->ColorFilter.a = 1.0;

			/* MAKE SURE SCALE != 0 */

	if (newNodePtr->Scale.x == 0.0f)
		newNodePtr->Scale.x = 0.0001f;
	if (newNodePtr->Scale.y == 0.0f)
		newNodePtr->Scale.y = 0.0001f;
	if (newNodePtr->Scale.z == 0.0f)
		newNodePtr->Scale.z = 0.0001f;


		/* NO VAPOR TRAILS YET */

	for (int i = 0; i < MAX_JOINTS; i++)
		newNodePtr->VaporTrails[i] = -1;


				/* INSERT NODE INTO LINKED LIST */

	newNodePtr->StatusBits |= STATUS_BIT_DETACHED;		// its not attached to linked list yet
	AttachObject(newNodePtr, false);

	gNumObjectNodes++;


				/* AUTO CHAIN */

	if (newObjDef->autoChain)
	{
		newObjDef->autoChain->ChainNode = newNodePtr;
		newObjDef->autoChain = newNodePtr;
	}


				/* CLEANUP */

	gMostRecentlyAddedNode = newNodePtr;					// remember this
	return(newNodePtr);
}

/************* MAKE NEW DISPLAY GROUP OBJECT *************/
//
// This is an ObjNode who's BaseGroup is a group, therefore these objects
// can be transformed (scale,rot,trans).
//

ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newObj;
Byte	group,type;


	newObjDef->genre = DISPLAY_GROUP_GENRE;

	newObj = MakeNewObject(newObjDef);
	if (newObj == nil)
		DoFatalAlert("MakeNewDisplayGroupObject: MakeNewObject failed!");

			/* MAKE BASE GROUP & ADD GEOMETRY TO IT */

	CreateBaseGroup(newObj);											// create group object
	group = newObjDef->group;											// get group #
	type = newObjDef->type;												// get type #

	if (type >= gNumObjectsInBG3DGroupList[group])							// see if illegal
	{
		Str255	s;

		DoAlert("MakeNewDisplayGroupObject: type > gNumObjectsInGroupList[]!");

		NumToStringC(group, s);
		DoAlert(s);
		NumToStringC(type,s);
		DoFatalAlert(s);
	}

	AttachGeometryToDisplayGroupObject(newObj,gBG3DGroupList[group][type]);


			/* SET BOUNDING BOX */

	newObj->BBox = gObjectGroupBBoxList[group][type];	// get this model's local bounding box


	return(newObj);
}




/******************* RESET DISPLAY GROUP OBJECT *********************/
//
// If the ObjNode's "Type" field has changed, call this to dispose of
// the old BaseGroup and create a new one with the correct model attached.
//

void ResetDisplayGroupObject(ObjNode *theNode)
{
	DisposeObjectBaseGroup(theNode);									// dispose of old group
	CreateBaseGroup(theNode);											// create new group object

	if (theNode->Type >= gNumObjectsInBG3DGroupList[theNode->Group])							// see if illegal
		DoFatalAlert("ResetDisplayGroupObject: type > gNumObjectsInGroupList[]!");

	AttachGeometryToDisplayGroupObject(theNode,gBG3DGroupList[theNode->Group][theNode->Type]);	// attach geometry to group

			/* SET BOUNDING BOX */

	theNode->BBox = gObjectGroupBBoxList[theNode->Group][theNode->Type];
}



/************************* ADD GEOMETRY TO DISPLAY GROUP OBJECT ***********************/
//
// Attaches a geometry object to the BaseGroup object. MakeNewDisplayGroupObject must have already been
// called which made the group & transforms.
//

void AttachGeometryToDisplayGroupObject(ObjNode *theNode, MetaObjectPtr geometry)
{
	MO_AppendToGroup(theNode->BaseGroup, geometry);
}



/***************** CREATE BASE GROUP **********************/
//
// The base group contains the base transform matrix plus any other objects you want to add into it.
// This routine creates the new group and then adds a transform matrix.
//
// The base is composed of BaseGroup & BaseTransformObject.
//

void CreateBaseGroup(ObjNode *theNode)
{
OGLMatrix4x4			transMatrix,scaleMatrix,rotMatrix;
MOMatrixObject			*transObject;


				/* CREATE THE BASE GROUP OBJECT */

	theNode->BaseGroup = MO_CreateNewObjectOfType(MO_TYPE_GROUP, 0, nil);
	if (theNode->BaseGroup == nil)
		DoFatalAlert("CreateBaseGroup: MO_CreateNewObjectOfType failed!");


					/* SETUP BASE MATRIX */

	if ((theNode->Scale.x == 0) || (theNode->Scale.y == 0) || (theNode->Scale.z == 0))
		DoFatalAlert("CreateBaseGroup: A scale component == 0");


	OGLMatrix4x4_SetScale(&scaleMatrix, theNode->Scale.x, theNode->Scale.y,		// make scale matrix
							theNode->Scale.z);

	OGLMatrix4x4_SetRotate_XYZ(&rotMatrix, theNode->Rot.x, theNode->Rot.y,		// make rotation matrix
								 theNode->Rot.z);

	OGLMatrix4x4_SetTranslate(&transMatrix, theNode->Coord.x, theNode->Coord.y,	// make translate matrix
							 theNode->Coord.z);

	OGLMatrix4x4_Multiply(&scaleMatrix,											// mult scale & rot matrices
						 &rotMatrix,
						 &theNode->BaseTransformMatrix);

	OGLMatrix4x4_Multiply(&theNode->BaseTransformMatrix,						// mult by trans matrix
						 &transMatrix,
						 &theNode->BaseTransformMatrix);


					/* CREATE A MATRIX XFORM */

	transObject = MO_CreateNewObjectOfType(MO_TYPE_MATRIX, 0, &theNode->BaseTransformMatrix);	// make matrix xform object
	if (transObject == nil)
		DoFatalAlert("CreateBaseGroup: MO_CreateNewObjectOfType/Matrix Failed!");

	MO_AttachToGroupStart(theNode->BaseGroup, transObject);						// add to base group
	theNode->BaseTransformObject = transObject;									// keep extra LEGAL ref (remember to dispose later)
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT PROCESSING ------


/*******************************  MOVE OBJECTS **************************/

void MoveObjects(void)
{
ObjNode		*thisNodePtr;

			/* CALL SOUND MAINTENANCE HERE FOR CONVENIENCE */

	DoSoundMaintenance();


	if (gFirstNodePtr == nil)								// see if there are any objects
		return;

	thisNodePtr = gFirstNodePtr;

	do
	{
				/* VERIFY NODE */

		if (thisNodePtr->CType == INVALID_NODE_FLAG)
			DoFatalAlert("MoveObjects: CType == INVALID_NODE_FLAG");

		gCurrentNode = thisNodePtr;							// set current object node
		gNextNode	 = thisNodePtr->NextNode;				// get next node now (cuz current node might get deleted)


		if (gGamePaused && !(thisNodePtr->StatusBits & STATUS_BIT_MOVEINPAUSE))
			goto next;


				/* UPDATE ANIMATION */

		UpdateSkeletonAnimation(thisNodePtr);


					/* NEXT TRY TO MOVE IT */

		if (!(thisNodePtr->StatusBits & STATUS_BIT_ONSPLINE))		// make sure don't call a move call if on spline
		{
			if ((!(thisNodePtr->StatusBits & STATUS_BIT_NOMOVE)) &&	(thisNodePtr->MoveCall != nil))
			{
				KeepOldCollisionBoxes(thisNodePtr);					// keep old boxes & other stuff
				thisNodePtr->MoveCall(thisNodePtr);				// call object's move routine
			}
		}

next:
		thisNodePtr = gNextNode;							// next node
	}
	while (thisNodePtr != nil);



			/* FLUSH THE DELETE QUEUE */

	FlushObjectDeleteQueue();
}



/**************************** DRAW OBJECTS ***************************/

void DrawObjects(void)
{
ObjNode		*theNode;
short		i,numTriMeshes;
unsigned long	statusBits;
Boolean			noLighting = false;
Boolean			noZBuffer = false;
Boolean			noZWrites = false;
Boolean			glow = false;
Boolean			noCullFaces = false;
Boolean			texWrap = false;
Boolean			noFog = false;
Boolean			clipAlpha= false;
float			cameraX, cameraZ;
short			skelType;

	if (gFirstNodePtr == nil)									// see if there are any objects
		return;


				/* FIRST DO OUR CULLING */

	CullTestAllObjects();

	theNode = gFirstNodePtr;


			/* GET CAMERA COORDS */

	cameraX = gGameViewInfoPtr->cameraPlacement.cameraLocation.x;
	cameraZ = gGameViewInfoPtr->cameraPlacement.cameraLocation.z;

			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/
	do
	{
		ObjNode* nextNode = theNode->NextNode;					// save next node in case object deletes itself in custom draw call

		statusBits = theNode->StatusBits;						// get obj's status bits

		if (statusBits & (STATUS_BIT_ISCULLED|STATUS_BIT_HIDDEN))	// see if is culled or hidden
			goto next;


		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;

		gGlobalTransparency = theNode->ColorFilter.a;			// get global transparency
		if (gGlobalTransparency <= 0.0f)						// see if invisible
			goto next;

		gGlobalColorFilter.r = theNode->ColorFilter.r;			// set color filter
		gGlobalColorFilter.g = theNode->ColorFilter.g;
		gGlobalColorFilter.b = theNode->ColorFilter.b;


			/******************/
			/* CHECK AUTOFADE */
			/******************/

		if (gAutoFadeStartDist != 0.0f)							// see if this level has autofade
		{
			if (statusBits & STATUS_BIT_AUTOFADE)
			{
				float		dist;

				dist = CalcQuickDistance(cameraX, cameraZ, theNode->Coord.x, theNode->Coord.z);			// see if in fade zone

				if (!theNode->BBox.isEmpty)
					dist += (theNode->BBox.max.x - theNode->BBox.min.x) * .2f;		// adjust dist based on size of object in order to fade big objects closer

				if (dist >= gAutoFadeStartDist)
				{
					dist -= gAutoFadeStartDist;							// calc xparency %
					dist *= gAutoFadeRange_Frac;
					if (dist < 0.0f)
						goto next;

					gGlobalTransparency -= dist;
					if (gGlobalTransparency <= 0.0f)
					{
						theNode->StatusBits |= STATUS_BIT_ISCULLED;		// set culled flag to that any related Sparkles wont be drawn either
						goto next;
					}
				}
			}
		}


			/*******************/
			/* CHECK BACKFACES */
			/*******************/

		if (statusBits & STATUS_BIT_KEEPBACKFACES)
		{
			if (!noCullFaces)
			{
				glDisable(GL_CULL_FACE);
				noCullFaces = true;
			}
		}
		else
		if (noCullFaces)
		{
			noCullFaces = false;
			glEnable(GL_CULL_FACE);
		}


			/*********************/
			/* CHECK NULL SHADER */
			/*********************/

		if (statusBits & STATUS_BIT_NOLIGHTING)
		{
			if (!noLighting)
			{
				OGL_DisableLighting();
				noLighting = true;
			}
		}
		else
		if (noLighting)
		{
			noLighting = false;
			OGL_EnableLighting();
		}

 			/****************/
			/* CHECK NO FOG */
			/****************/

		if (gGameViewInfoPtr->useFog)
		{
			if (statusBits & STATUS_BIT_NOFOG)
			{
				if (!noFog)
				{
					glDisable(GL_FOG);
					noFog = true;
				}
			}
			else
			if (noFog)
			{
				noFog = false;
				glEnable(GL_FOG);
			}
		}

			/********************/
			/* CHECK GLOW BLEND */
			/********************/

		if (statusBits & STATUS_BIT_GLOW)
		{
			if (!glow)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glow = true;
			}
		}
		else
		if (glow)
		{
			glow = false;
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}


			/**********************/
			/* CHECK TEXTURE WRAP */
			/**********************/

		if (statusBits & STATUS_BIT_NOTEXTUREWRAP)
		{
			if (!texWrap)
			{
				gGlobalMaterialFlags |= BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V;
				texWrap = true;
			}
		}
		else
		if (texWrap)
		{
			texWrap = false;
		    gGlobalMaterialFlags &= ~(BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);
		}




			/****************/
			/* CHECK ZWRITE */
			/****************/

		if (statusBits & STATUS_BIT_NOZWRITES)
		{
			if (!noZWrites)
			{
				glDepthMask(GL_FALSE);
				noZWrites = true;
			}
		}
		else
		if (noZWrites)
		{
			glDepthMask(GL_TRUE);
			noZWrites = false;
		}


			/*****************/
			/* CHECK ZBUFFER */
			/*****************/

		if (statusBits & STATUS_BIT_NOZBUFFER)
		{
			if (!noZBuffer)
			{
				glDisable(GL_DEPTH_TEST);
				noZBuffer = true;
			}
		}
		else
		if (noZBuffer)
		{
			noZBuffer = false;
			glEnable(GL_DEPTH_TEST);
		}


			/*****************************/
			/* CHECK EDGE ALPHA CLIPPING */
			/*****************************/

		if ((statusBits & STATUS_BIT_CLIPALPHA) && (gGlobalTransparency == 1.0f))
		{
			if (!clipAlpha)
			{
				glAlphaFunc(GL_EQUAL, 1);	// draw any pixel who's Alpha == 1, skip semi-transparent pixels
				clipAlpha = true;
			}
		}
		else
		if (clipAlpha)
		{
			clipAlpha = false;
			glAlphaFunc(GL_NOTEQUAL, 0);	// draw any pixel who's Alpha != 0
		}


			/* AIM AT CAMERA */

		if (statusBits & STATUS_BIT_AIMATCAMERA)
		{
			theNode->Rot.y = PI+CalcYAngleFromPointToPoint(theNode->Rot.y,
														theNode->Coord.x, theNode->Coord.z,
														cameraX, cameraZ);

			UpdateObjectTransforms(theNode);

		}



			/************************/
			/* SHOW COLLISION BOXES */
			/************************/

		if (gDebugMode == 2)
		{
			DrawCollisionBoxes(theNode,false);
//			DrawBoundingBoxes(theNode);
		}


			/***************************/
			/* SEE IF DO U/V TRANSFORM */
			/***************************/

		if (statusBits & STATUS_BIT_UVTRANSFORM)
		{
			glMatrixMode(GL_TEXTURE);					// set texture matrix
			glTranslatef(theNode->TextureTransformU, theNode->TextureTransformV, 0);
			glMatrixMode(GL_MODELVIEW);
		}




			/***********************/
			/* SUBMIT THE GEOMETRY */
			/***********************/

		gCurrentObjMatrix = &theNode->BaseTransformMatrix;			// get global pointer to our matrix

 		if (noLighting || (theNode->Scale.y == 1.0f))				// if scale == 1 or no lighting, then dont need to normalize vectors
 			glDisable(GL_NORMALIZE);
 		else
 			glEnable(GL_NORMALIZE);

		if (theNode->CustomDrawFunction)							// if has custom draw function, then override and use that
			goto custom_draw;

		MOMaterialObject	*overrideTexture = nil;
		MOMaterialObject	*oldTexture = nil;

		switch(theNode->Genre)
		{
			case	SKELETON_GENRE:
					UpdateSkinnedGeometry(theNode);													// update skeleton geometry

					if (theNode == gPlayerInfo.objNode)
						UpdatePlayerMotionBlur(theNode);
			
					numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
					skelType = theNode->Type;

					overrideTexture = theNode->Skeleton->overrideTexture;							// get any override texture ref (illegal ref)

					for (i = 0; i < numTriMeshes; i++)												// submit each trimesh of it
					{
						if (overrideTexture)														// set override texture
						{
							if (gLocalTriMeshesOfSkelType[skelType][i].numMaterials > 0)
							{
								oldTexture = gLocalTriMeshesOfSkelType[skelType][i].materials[0];		// get the normal texture for this mesh
								gLocalTriMeshesOfSkelType[skelType][i].materials[0] = overrideTexture;	// set the override one temporarily
							}
						}

						MO_DrawGeometry_VertexArray(&gLocalTriMeshesOfSkelType[skelType][i]);

						if (overrideTexture && oldTexture)											// see if need to set texture back to normal
							gLocalTriMeshesOfSkelType[skelType][i].materials[0] = oldTexture;
					}
					break;

			case	DISPLAY_GROUP_GENRE:
					if (theNode->BaseGroup)
					{
						MO_DrawObject(theNode->BaseGroup);
					}
					break;

			case	SPRITE_GENRE:
					if (theNode->SpriteMO)
					{
						OGL_PushState();								// keep state

						SetInfobarSpriteState(true);

						theNode->SpriteMO->objectData.coord = theNode->Coord;	// update Meta Object's coord info
						theNode->SpriteMO->objectData.scaleX = theNode->Scale.x;
						theNode->SpriteMO->objectData.scaleY = theNode->Scale.y;

						MO_DrawObject(theNode->SpriteMO);
						OGL_PopState();									// restore state
					}
					break;

			case	TEXTMESH_GENRE:
					if (theNode->BaseGroup)
					{
						OGL_PushState();								// keep state
						SetInfobarSpriteState(true);
						MO_DrawObject(theNode->BaseGroup);

						if (gDebugMode == 2)
						{
							TextMesh_DrawExtents(theNode);
						}

						OGL_PopState();									// restore state
					}
					break;

			case	CUSTOM_GENRE:
custom_draw:
					if (theNode->CustomDrawFunction)
					{
						theNode->CustomDrawFunction(theNode);
					}
					break;
		}


				/***************************/
				/* SEE IF END UV TRANSFORM */
				/***************************/

		if (statusBits & STATUS_BIT_UVTRANSFORM)
		{
			glMatrixMode(GL_TEXTURE);					// set texture matrix
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
		}



			/* NEXT NODE */
next:
		theNode = nextNode;
	}while (theNode != nil);


				/*****************************/
				/* RESET SETTINGS TO DEFAULT */
				/*****************************/

	if (noLighting)
		OGL_EnableLighting();

	if (gGameViewInfoPtr->useFog)
	{
		if (noFog)
			glEnable(GL_FOG);
	}

	if (glow)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (noZBuffer)
	{
		glEnable(GL_DEPTH_TEST);
//		glDepthMask(GL_TRUE);
	}

	if (noZWrites)
		glDepthMask(GL_TRUE);

	if (noCullFaces)
		glEnable(GL_CULL_FACE);

	if (texWrap)
	    gGlobalMaterialFlags &= ~(BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);

	if (clipAlpha)
		glAlphaFunc(GL_NOTEQUAL, 0);


	gGlobalTransparency = 			// reset this in case it has changed
	gGlobalColorFilter.r =
	gGlobalColorFilter.g =
	gGlobalColorFilter.b = 1.0;

	glEnable(GL_NORMALIZE);
}


/************************ DRAW COLLISION BOXES ****************************/

static void DrawCollisionBoxes(ObjNode *theNode, Boolean old)
{
float				left,right,top,bottom,front,back;

	OGL_PushState();

			/* SET LINE MATERIAL */

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);


		/* SCAN EACH COLLISION BOX */

	const CollisionBoxType* c = old ? theNode->OldCollisionBoxes : theNode->CollisionBoxes;

	for (int i = 0; i < theNode->NumCollisionBoxes; i++)
	{
			/* GET BOX PARAMS */

		left 	= c[i].left;
		right 	= c[i].right;
		top 	= c[i].top;
		bottom 	= c[i].bottom;
		front 	= c[i].front;
		back 	= c[i].back;

			/* DRAW TOP */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, top, back);
		glColor3f(1,1,0);
		glVertex3f(left, top, front);
		glVertex3f(right, top, front);
		glColor3f(1,0,0);
		glVertex3f(right, top, back);
		glEnd();

			/* DRAW BOTTOM */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(left, bottom, front);
		glVertex3f(right, bottom, front);
		glColor3f(1,0,0);
		glVertex3f(right, bottom, back);
		glEnd();


			/* DRAW LEFT */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, top, back);
		glColor3f(1,0,0);
		glVertex3f(left, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(left, bottom, front);
		glVertex3f(left, top, front);
		glEnd();


			/* DRAW RIGHT */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(right, top, back);
		glVertex3f(right, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(right, bottom, front);
		glVertex3f(right, top, front);
		glEnd();

	}



	if (gRocketShipHotZone[0].x != 0 && gRocketShipHotZone[0].y != 0)
	{
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < 4; i++)
		{
			float x = gRocketShipHotZone[i].x;
			float z = gRocketShipHotZone[i].y;
			float y = 10 + GetTerrainY(x, z);
			glVertex3f(x,y,z);
		}
		glEnd();
	}


	OGL_PopState();
}

/************************ DRAW BOUNDING BOXES ****************************/

static void DrawBoundingBoxes(ObjNode *theNode)
{
float	left,right,top,bottom,front,back;
int		i;

			/* SET LINE MATERIAL */

	glDisable(GL_TEXTURE_2D);


			/***************************/
			/* TRANSFORM BBOX TO WORLD */
			/***************************/

	if (theNode->Genre == SKELETON_GENRE)			// skeletons are already oriented, just need translation
	{
		left 	= theNode->BBox.min.x + theNode->Coord.x;
		right 	= theNode->BBox.max.x + theNode->Coord.x;
		top 	= theNode->BBox.max.y + theNode->Coord.y;
		bottom 	= theNode->BBox.min.y + theNode->Coord.y;
		front 	= theNode->BBox.max.z + theNode->Coord.z;
		back 	= theNode->BBox.min.z + theNode->Coord.z;
	}
	else											// non-skeletons need full transform
	{
		OGLPoint3D	corners[8];

		corners[0].x = theNode->BBox.min.x;			// top far left
		corners[0].y = theNode->BBox.max.y;
		corners[0].z = theNode->BBox.min.z;

		corners[1].x = theNode->BBox.max.x;			// top far right
		corners[1].y = theNode->BBox.max.y;
		corners[1].z = theNode->BBox.min.z;

		corners[2].x = theNode->BBox.max.x;			// top near right
		corners[2].y = theNode->BBox.max.y;
		corners[2].z = theNode->BBox.max.z;

		corners[3].x = theNode->BBox.min.x;			// top near left
		corners[3].y = theNode->BBox.max.y;
		corners[3].z = theNode->BBox.max.z;

		corners[4].x = theNode->BBox.min.x;			// bottom far left
		corners[4].y = theNode->BBox.min.y;
		corners[4].z = theNode->BBox.min.z;

		corners[5].x = theNode->BBox.max.x;			// bottom far right
		corners[5].y = theNode->BBox.min.y;
		corners[5].z = theNode->BBox.min.z;

		corners[6].x = theNode->BBox.max.x;			// bottom near right
		corners[6].y = theNode->BBox.min.y;
		corners[6].z = theNode->BBox.max.z;

		corners[7].x = theNode->BBox.min.x;			// bottom near left
		corners[7].y = theNode->BBox.min.y;
		corners[7].z = theNode->BBox.max.z;

		OGLPoint3D_TransformArray(corners, &theNode->BaseTransformMatrix, corners, 8);

					/* FIND NEW BOUNDS */

		left 	= corners[0].x;
		for (i = 1; i < 8; i ++)
			if (corners[i].x < left)
				left = corners[i].x;

		right 	= corners[0].x;
		for (i = 1; i < 8; i ++)
			if (corners[i].x > right)
				right = corners[i].x;

		bottom 	= corners[0].y;
		for (i = 1; i < 8; i ++)
			if (corners[i].y < bottom)
				bottom = corners[i].y;

		top 	= corners[0].y;
		for (i = 1; i < 8; i ++)
			if (corners[i].y > top)
				top = corners[i].y;

		back 	= corners[0].z;
		for (i = 1; i < 8; i ++)
			if (corners[i].z < back)
				back = corners[i].z;


		front 	= corners[0].z;
		for (i = 1; i < 8; i ++)
			if (corners[i].z > front)
				front = corners[i].z;
	}


		/* DRAW TOP */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(left, top, back);
	glColor3f(1,1,0);
	glVertex3f(left, top, front);
	glVertex3f(right, top, front);
	glColor3f(1,0,0);
	glVertex3f(right, top, back);
	glEnd();

		/* DRAW BOTTOM */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(left, bottom, back);
	glColor3f(1,1,0);
	glVertex3f(left, bottom, front);
	glVertex3f(right, bottom, front);
	glColor3f(1,0,0);
	glVertex3f(right, bottom, back);
	glEnd();


		/* DRAW LEFT */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(left, top, back);
	glColor3f(1,0,0);
	glVertex3f(left, bottom, back);
	glColor3f(1,1,0);
	glVertex3f(left, bottom, front);
	glVertex3f(left, top, front);
	glEnd();


		/* DRAW RIGHT */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(right, top, back);
	glVertex3f(right, bottom, back);
	glColor3f(1,1,0);
	glVertex3f(right, bottom, front);
	glVertex3f(right, top, front);
	glEnd();
}


/********************* MOVE STATIC OBJECT **********************/

void MoveStaticObject(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	UpdateShadow(theNode);										// prime it
}

/********************* MOVE STATIC OBJECT2 **********************/
//
// Keeps object conformed to terrain curves
//

void MoveStaticObject2(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	RotateOnTerrain(theNode, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(theNode);

	if (theNode->NumCollisionBoxes > 0)
		CalcObjectBoxFromNode(theNode);

	UpdateShadow(theNode);										// prime it
}


/********************* MOVE STATIC OBJECT 3 **********************/
//
// Keeps at current terrain height
//

void MoveStaticObject3(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - (theNode->BBox.min.y * theNode->Scale.y);

	UpdateObjectTransforms(theNode);

	if (theNode->NumCollisionBoxes > 0)
		CalcObjectBoxFromNode(theNode);

	UpdateShadow(theNode);										// prime it
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT DELETING ------


/******************** DELETE ALL OBJECTS ********************/

void DeleteAllObjects(void)
{
	while (gFirstNodePtr != nil)
		DeleteObject(gFirstNodePtr);

	FlushObjectDeleteQueue();
}


/************************ DELETE OBJECT ****************/

void DeleteObject(ObjNode	*theNode)
{
int		i;

	if (theNode == nil)								// see if passed a bogus node
		return;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
	{
		DoFatalAlert(
			"Attempted to Double Delete an Object.  Object was already deleted!  genre=%d group=%d type=%d",
			theNode->Genre, theNode->Group, theNode->Type);
	}

			/* RECURSIVE DELETE OF CHAIN NODE & SHADOW NODE */
			//
			// should do these first so that base node will have appropriate nextnode ptr
			// since it's going to be used next pass thru the moveobjects loop.  This
			// assumes that all chained nodes are later in list.
			//

	if (theNode->ChainNode)
		DeleteObject(theNode->ChainNode);

	if (theNode->ShadowNode)
		DeleteObject(theNode->ShadowNode);


			/* SEE IF NEED TO FREE UP SPECIAL MEMORY */

	switch(theNode->Genre)
	{
		case	SKELETON_GENRE:
				FreeSkeletonBaseData(theNode->Skeleton);		// purge all alloced memory for skeleton data
				theNode->Skeleton = nil;
				break;

		case	SPRITE_GENRE:
				MO_DisposeObjectReference(theNode->SpriteMO);	// dispose reference to sprite meta object
		   		theNode->SpriteMO = nil;
				break;

	}

	for (i = 0; i < MAX_NODE_SPARKLES; i++)				// free sparkles
	{
		if (theNode->Sparkles[i] != -1)
		{
			DeleteSparkle(theNode->Sparkles[i]);
			theNode->Sparkles[i] = -1;
		}
	}

			/* SEE IF STOP SOUND CHANNEL */

	StopObjectStreamEffect(theNode);


		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */

	DisposeObjectBaseGroup(theNode);


			/* REMOVE NODE FROM LINKED LIST */

	DetachObject(theNode, false);


			/* SEE IF MARK AS NOT-IN-USE IN ITEM LIST */

	if (theNode->TerrainItemPtr)
	{
		theNode->TerrainItemPtr->flags &= ~ITEM_FLAGS_INUSE;		// clear the "in use" flag
	}

		/* OR, IF ITS A SPLINE ITEM, THEN UPDATE SPLINE OBJECT LIST */

	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
	{
		RemoveFromSplineObjectList(theNode);
	}


			/* DELETE THE NODE BY ADDING TO DELETE QUEUE */

	theNode->CType = INVALID_NODE_FLAG;							// INVALID_NODE_FLAG indicates its deleted


	gObjectDeleteQueue[gNumObjsInDeleteQueue++] = theNode;
	if (gNumObjsInDeleteQueue >= OBJ_DEL_Q_SIZE)
		FlushObjectDeleteQueue();

}


/****************** DETACH OBJECT ***************************/
//
// Simply detaches the objnode from the linked list, patches the links
// and keeps the original objnode intact.
//

void DetachObject(ObjNode *theNode, Boolean subrecurse)
{
	if (theNode == nil)
		return;

	if (theNode->StatusBits & STATUS_BIT_DETACHED)	// make sure not already detached
		return;

	if (theNode == gNextNode)						// if its the next node to be moved, then fix things
		gNextNode = theNode->NextNode;

	if (theNode->PrevNode == nil)					// special case 1st node
	{
		gFirstNodePtr = theNode->NextNode;
		if (gFirstNodePtr)
			gFirstNodePtr->PrevNode = nil;
	}
	else
	if (theNode->NextNode == nil)					// special case last node
	{
		theNode->PrevNode->NextNode = nil;
	}
	else											// generic middle deletion
	{
		theNode->PrevNode->NextNode = theNode->NextNode;
		theNode->NextNode->PrevNode = theNode->PrevNode;
	}

	theNode->PrevNode = nil;						// seal links on original node
	theNode->NextNode = nil;

	theNode->StatusBits |= STATUS_BIT_DETACHED;

			/* SUBRECURSE CHAINS & SHADOW */

	if (subrecurse)
	{
		if (theNode->ChainNode)
			DetachObject(theNode->ChainNode, subrecurse);

		if (theNode->ShadowNode)
			DetachObject(theNode->ShadowNode, subrecurse);
	}
}


/****************** ATTACH OBJECT ***************************/

void AttachObject(ObjNode *theNode, Boolean recurse)
{
short	slot;

	if (theNode == nil)
		return;

	if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if already attached
		return;

	slot = theNode->Slot;

	if (gFirstNodePtr == nil)						// special case only entry
	{
		gFirstNodePtr = theNode;
		theNode->PrevNode = nil;
		theNode->NextNode = nil;
	}

			/* INSERT AS FIRST NODE */
	else
	if (slot < gFirstNodePtr->Slot)
	{
		theNode->PrevNode = nil;					// no prev
		theNode->NextNode = gFirstNodePtr; 			// next pts to old 1st
		gFirstNodePtr->PrevNode = theNode; 			// old pts to new 1st
		gFirstNodePtr = theNode;
	}
		/* SCAN FOR INSERTION PLACE */
	else
	{
		ObjNode	 *reNodePtr, *scanNodePtr;

		reNodePtr = gFirstNodePtr;
		scanNodePtr = gFirstNodePtr->NextNode;		// start scanning for insertion slot on 2nd node

		while (scanNodePtr != nil)
		{
			if (slot < scanNodePtr->Slot)					// INSERT IN MIDDLE HERE
			{
				theNode->NextNode = scanNodePtr;
				theNode->PrevNode = reNodePtr;
				reNodePtr->NextNode = theNode;
				scanNodePtr->PrevNode = theNode;
				goto out;
			}
			reNodePtr = scanNodePtr;
			scanNodePtr = scanNodePtr->NextNode;			// try next node
		}

		theNode->NextNode = nil;							// TAG TO END
		theNode->PrevNode = reNodePtr;
		reNodePtr->NextNode = theNode;
	}
out:


	theNode->StatusBits &= ~STATUS_BIT_DETACHED;



			/* SUBRECURSE CHAINS & SHADOW */

	if (recurse)
	{
		if (theNode->ChainNode)
			AttachObject(theNode->ChainNode, recurse);

		if (theNode->ShadowNode)
			AttachObject(theNode->ShadowNode, recurse);
	}

}


/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(void)
{
	for (int i = 0; i < gNumObjsInDeleteQueue; i++)
	{
		ObjNode* node = gObjectDeleteQueue[i];
		GAME_ASSERT(node != NULL);

		ptrdiff_t poolIndex = node - gObjNodeMemory;

		if (poolIndex >= 0 && poolIndex < OBJ_BUDGET)
		{
			// node is pooled, put back into pool
			Pool_ReleaseIndex(gObjNodePool, poolIndex);
		}
		else
		{
			// node was allocated on heap
			SafeDisposePtr((Ptr) node);
		}
	}

	gNumObjectNodes -= gNumObjsInDeleteQueue;

	gNumObjsInDeleteQueue = 0;
}


/****************** DISPOSE OBJECT BASE GROUP **********************/

void DisposeObjectBaseGroup(ObjNode *theNode)
{
	if (theNode->BaseGroup != nil)
	{
		MO_DisposeObjectReference(theNode->BaseGroup);

		theNode->BaseGroup = nil;
	}

	if (theNode->BaseTransformObject != nil)							// also nuke extra ref to transform object
	{
		MO_DisposeObjectReference(theNode->BaseTransformObject);
		theNode->BaseTransformObject = nil;
	}
}




//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT INFORMATION ------


/********************** GET OBJECT INFO *********************/

void GetObjectInfo(ObjNode *theNode)
{
	gCoord = theNode->Coord;
	gDelta = theNode->Delta;

}


/********************** UPDATE OBJECT *********************/

void UpdateObject(ObjNode *theNode)
{
	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

	theNode->Coord = gCoord;
	theNode->Delta = gDelta;
	UpdateObjectTransforms(theNode);

	CalcObjectBoxFromNode(theNode);


		/* UPDATE ANY SHADOWS */

	UpdateShadow(theNode);
}



/****************** UPDATE OBJECT TRANSFORMS *****************/
//
// This updates the skeleton object's base translate & rotate transforms
//

void UpdateObjectTransforms(ObjNode *theNode)
{
OGLMatrix4x4	m,m2;
OGLMatrix4x4	mx,my,mz,mxz;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

				/********************/
				/* SET SCALE MATRIX */
				/********************/

	OGLMatrix4x4_SetScale(&m, theNode->Scale.x,	theNode->Scale.y, theNode->Scale.z);


			/*****************************/
			/* NOW ROTATE & TRANSLATE IT */
			/*****************************/

	uint32_t bits = theNode->StatusBits;

				/* USE ALIGNMENT MATRIX */

	if (bits & STATUS_BIT_USEALIGNMENTMATRIX)
	{
		m2 = theNode->AlignmentMatrix;
	}
	else
	{
					/* DO XZY ROTATION */

		if (bits & STATUS_BIT_ROTXZY)
		{

			OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
			OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
			OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

			OGLMatrix4x4_Multiply(&mx,&mz, &mxz);
			OGLMatrix4x4_Multiply(&mxz,&my, &m2);
		}
					/* DO YZX ROTATION */

		else
		if (bits & STATUS_BIT_ROTYZX)
		{
			OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
			OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
			OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

			OGLMatrix4x4_Multiply(&my,&mz, &mxz);
			OGLMatrix4x4_Multiply(&mxz,&mx, &m2);
		}

					/* DO ZXY ROTATION */

		else
		if (bits & STATUS_BIT_ROTZXY)
		{
			OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
			OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
			OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

			OGLMatrix4x4_Multiply(&mz,&mx, &mxz);
			OGLMatrix4x4_Multiply(&mxz,&my, &m2);
		}

					/* STANDARD XYZ ROTATION */
		else
		{
			OGLMatrix4x4_SetRotate_XYZ(&m2, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
		}
	}


	m2.value[M03] = theNode->Coord.x;
	m2.value[M13] = theNode->Coord.y;
	m2.value[M23] = theNode->Coord.z;

	OGLMatrix4x4_Multiply(&m,&m2, &theNode->BaseTransformMatrix);


				/* UPDATE TRANSFORM OBJECT */

	SetObjectTransformMatrix(theNode);
}


/***************** SET OBJECT TRANSFORM MATRIX *******************/
//
// This call simply resets the base transform object so that it uses the latest
// base transform matrix
//

void SetObjectTransformMatrix(ObjNode *theNode)
{
MOMatrixObject	*mo = theNode->BaseTransformObject;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if invalid
		return;

	if (mo)				// see if this has a trans obj
	{
		mo->matrix =  theNode->BaseTransformMatrix;
	}
}




/********************* FIND CLOSEST CTYPE *****************************/

ObjNode* FindClosestCType(const OGLPoint3D *pt, uint32_t ctype)
{
ObjNode		*thisNodePtr,*best = nil;
float		minDist = 10000000;


	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;

		if (thisNodePtr->CType & ctype)
		{
			float d = CalcQuickDistance(pt->x, pt->z, thisNodePtr->Coord.x, thisNodePtr->Coord.z);
			if (d < minDist)
			{
				minDist = d;
				best = thisNodePtr;
			}
		}
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(best);
}


/********************* FIND CLOSEST CTYPE 3D *****************************/

ObjNode* FindClosestCType3D(const OGLPoint3D* pt, uint32_t ctype)
{
ObjNode		*thisNodePtr,*best = nil;
float		minDist = 10000000;


	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;

		if (thisNodePtr->CType & ctype)
		{
			float d = OGLPoint3D_Distance(pt, &thisNodePtr->Coord);
			if (d < minDist)
			{
				minDist = d;
				best = thisNodePtr;
			}
		}
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(best);
}

