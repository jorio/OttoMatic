/****************************/
/*         MY GLOBALS       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#pragma once


#define	kGameID 		'Otto'

			/* SOME FLOATING POINT HELPERS */

#define EPS .0001					// a very small number which is useful for FP compares close to 0
#define	IS_ZERO(_x)  (fabs(_x) < EPS)
#define	SQUARED(x)					((x)*(x))

static inline int MinInt(int a, int b) { return a < b ? a : b; }
static inline int MaxInt(int a, int b) { return a < b ? b : a; }
static inline int ClampInt(int x, int lo, int hi) { return x < lo ? lo : x > hi ? hi : x; }

static inline float MinFloat(float a, float b) { return a < b ? a : b; }
static inline float MaxFloat(float a, float b) { return a < b ? b : a; }
static inline float ClampFloat(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
static inline float LerpFloat(float from, float to, float t) { return from * (1.0f - t) + to * t; }


		/* CLOSE ENOUGH TO ZERO */
		//
		// If float value is close enough to 0, then make it 0
		//

#define	CLOSE_ENOUGH_TO_ZERO(theFloat)	if (fabs(theFloat) < EPS) theFloat = 0;


		/*******************/
		/* 2D ARRAY MACROS */
		/*******************/

#define Alloc_2d_array(type, array, n, m)								\
{																		\
int _i;																	\
																		\
	array = (type **) AllocPtr((long)(n) * sizeof(type *));				\
	if (array == nil)													\
		DoFatalAlert("Alloc_2d_array failed!");						\
	array[0] = (type *) AllocPtr((long)(n) * (long)(m) * sizeof(type));	\
	if (array[0] == nil)												\
		DoFatalAlert("Alloc_2d_array failed!");						\
	for (_i = 1; _i < (n); _i++)										\
		array[_i] = array[_i-1] + (m);									\
}


#define Free_2d_array(array)				\
{											\
		SafeDisposePtr((Ptr)array[0]);		\
		SafeDisposePtr((Ptr)array);			\
		array = nil;						\
}


#define	PI					((float)3.1415926535898)
#define PI2					(2.0f*PI)

															
							// COLLISION SIDE INFO
							//=================================
							
#define	SIDE_BITS_TOP		(1)							// %000001	(r/l/b/t)
#define	SIDE_BITS_BOTTOM	(1<<1)						// %000010
#define	SIDE_BITS_LEFT		(1<<2)						// %000100 
#define	SIDE_BITS_RIGHT		(1<<3)						// %001000
#define	SIDE_BITS_FRONT		(1<<4)						// %010000 
#define	SIDE_BITS_BACK		(1<<5)						// %100000
#define	ALL_SOLID_SIDES		(SIDE_BITS_TOP|SIDE_BITS_BOTTOM|SIDE_BITS_LEFT|SIDE_BITS_RIGHT|\
							SIDE_BITS_FRONT|SIDE_BITS_BACK)
#define	SIDE_BITS_FENCE		(1<<6)						// not a real side, but returned by HandleCollision if a fence was hit


							// CBITS (32 BIT VALUES)
							//==================================

enum
{
	CBITS_TOP 			= SIDE_BITS_TOP,
	CBITS_BOTTOM 		= SIDE_BITS_BOTTOM,
	CBITS_LEFT 			= SIDE_BITS_LEFT,
	CBITS_RIGHT 		= SIDE_BITS_RIGHT,
	CBITS_FRONT 		= SIDE_BITS_FRONT,
	CBITS_BACK 			= SIDE_BITS_BACK,
	CBITS_ALLSOLID		= ALL_SOLID_SIDES,
	CBITS_NOTTOP		= SIDE_BITS_LEFT|SIDE_BITS_RIGHT|SIDE_BITS_FRONT|SIDE_BITS_BACK,
	CBITS_TOUCHABLE		= (1<<6),
	CBITS_ALWAYSTRIGGER = (1<<7)
};


							// CTYPES (32 BIT VALUES)
							//==================================

enum
{
	CTYPE_PLAYER	=	1,			// Me
	CTYPE_HUMAN		=	(1<<1),		// set if is a human
	CTYPE_BLOCKRAYS	=	(1<<2),		// if should block certain ray-intersect checks (typically destructible gates)
	CTYPE_TRIGGER2	=	(1<<3),		// Enemy Trigger
	CTYPE_TRIGGER	=	(1<<4),		// Trigger
	CTYPE_SKELETON	=	(1<<5),		// Skeleton
	CTYPE_MISC		=	(1<<6),		// Misc
	CTYPE_BLOCKSHADOW =	(1<<7),		// Shadows go over it
	CTYPE_WATER		=	(1<<8),		// if want to do auto water detect with HandleCollisions()
	CTYPE_POWERUP 	=	(1<<9),		// Powerup
	CTYPE_HURTME	= 	(1<<10),	// HURT ME
	CTYPE_HURTENEMY	= 	(1<<11),	// Hurt Enemy
	CTYPE_FENCE 	=	(1<<12),	// check fences
	CTYPE_TERRAIN	=	(1<<13),	// not an attribute, but just a flag passed to HandleCollisions()
	CTYPE_LIGHTNINGROD	=	(1<<14), // when want to give supernova priority
	CTYPE_ENEMY 	=	(1<<15),	// enemy
	CTYPE_BLOBPOW	 =	(1<<16),	// if it's a frozen blob powerup bloblue
	CTYPE_FLAMING	=	(1<<17),	// used with CTYPE_HURTME if item is flaming and might cause collision flame
	CTYPE_AUTOTARGETWEAPON	=	(1<<18),	// if auto target weapon fire
	CTYPE_BLOCKCAMERA =	(1<<19),	// camera goes over this
	CTYPE_WEAPON = (1<<20),			// weapon projectile
	CTYPE_MPLATFORM	= (1<<21),		// set if it's a moving platform
	CTYPE_IMPENETRABLE	= (1<<22),	// set if object must have high collision priority and cannot be pushed thru this
	CTYPE_IMPENETRABLE2	= (1<<23),	// set with CTYPE_IMPENETRABLE if dont want player to do coord=oldCoord when touched
	CTYPE_AUTOTARGETJUMP = (1<<24),	// if auto target when jumping
	CTYPE_HEATSEEKATTACT = (1<<25),	// if can attract heat seeking weapons
	CTYPE_MPLATFORM_FREEJUMP = (1<<26),	// don't factor in platform deltas when jumping off of it
};



							// OBJNODE STATUS BITS
							//==================================

enum
{
	STATUS_BIT_ONGROUND		=	1,			// Player is on anything solid (terrain or objnode)
	STATUS_BIT_NOTEXTUREWRAP =	(1<<1),		// if pin textures so they dont wrap
	STATUS_BIT_DONTCULL		=	(1<<2),		// set if don't want to perform custom culling on this object
	STATUS_BIT_NOCOLLISION  = 	(1<<3),		// set if want collision code to skip testing against this object
	STATUS_BIT_NOMOVE  		= 	(1<<4),		// dont call object's move function
	STATUS_BIT_MOVEINPAUSE	 = 	(1<<5),		// object's move function is called even as the game is paused
	STATUS_BIT_HIDDEN		=	(1<<6),		// dont draw object
	STATUS_BIT_KEEPBACKFACES	=	(1<<7),	// keep both sides
	STATUS_BIT_UNDERWATER 	=	(1<<8),		// set if underwater
	STATUS_BIT_ROTXZY		 = 	(1<<9),		// set if want to do x->z->y ordered rotation
	STATUS_BIT_ISCULLED		 = 	(1<<10),	// set if culling function deemed it culled
	STATUS_BIT_GLOW			=	(1<<11),	// use additive blending for glow effect
	STATUS_BIT_ROTYZX		=  (1<<12),		// 
	STATUS_BIT_NOLIGHTING	 =  (1<<13),	// used when want to render object will NULL shading (no lighting)
	STATUS_BIT_ALWAYSCULL	 =  (1<<14),	// to force a cull-check
	STATUS_BIT_CLIPALPHA 	 =  (1<<15), 	// set if want to modify alphafunc
	STATUS_BIT_DELETE		=	(1<<16),	// set by a function which cannot actually delete an object but we want to tell system to delete it at its convenience
	STATUS_BIT_NOZBUFFER	=	(1<<17),	// set when want to turn off z buffer
	STATUS_BIT_NOZWRITES	=	(1<<18),	// set when want to turn off z buffer writes
	STATUS_BIT_NOFOG		=	(1<<19),
	STATUS_BIT_AUTOFADE		=	(1<<20),	// calculate fade xparency value for object when rendering
	STATUS_BIT_DETACHED		=	(1<<21),	// set if objnode is free-floating and not attached to linked list
	STATUS_BIT_ONSPLINE		=	(1<<22),	// if objnode is attached to spline
	STATUS_BIT_REVERSESPLINE =	(1<<23),	// if going reverse direction on spline
	STATUS_BIT_SAUCERTARGET =	(1<<24),	// if saucer is targeted to this
	STATUS_BIT_AIMATCAMERA 	=	(1<<25),		// if need to aim at current player's camera (for sprite billboards)
	STATUS_BIT_USEALIGNMENTMATRIX =  (1<<26),	// use AlignmentMatrix instead of Rot x,y,z for object alignment
	STATUS_BIT_UVTRANSFORM	=	(1<<27),		// do uv transform on object when drawing
	STATUS_BIT_ROTZXY 		= 	(1<<28)
};



	/* POWERUP & WEAPON TYPES */

enum
{	
			/* guns */
			
	WEAPON_TYPE_STUNPULSE	= 0,
	WEAPON_TYPE_FREEZE,
	WEAPON_TYPE_FLAME,
	WEAPON_TYPE_GROWTH,
	WEAPON_TYPE_FLARE,
	
			/* non-guns */
			
	WEAPON_TYPE_FIST,
	WEAPON_TYPE_SUPERNOVA,
	WEAPON_TYPE_DART,
	
	NUM_WEAPON_TYPES
};

	
enum
{
	POW_TYPE_STUNPULSE,
	POW_TYPE_HEALTH,
	POW_TYPE_JUMPJET,
	POW_TYPE_FUEL,
	POW_TYPE_SUPERNOVA,
	POW_TYPE_FREEZE,
	POW_TYPE_MAGNET,
	POW_TYPE_GROWTH,
	POW_TYPE_FLAME,
	POW_TYPE_FLARE,
	POW_TYPE_DART,
	POW_TYPE_FREELIFE,

	NUM_POW_TYPES
};
