/****************************/
/*        LZSS.C           */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/



/**********************/
/*     PROTOTYPES     */
/**********************/

static void InitLZSSMemory(void);



/****************************/
/*    CONSTANTS             */
/****************************/


#define RING_BUFF_SIZE		 		4096			/* size of ring buffer */

#define F		   		18							/* upper limit for match_length */
#define THRESHOLD		2   						/* encode string into position and length if match_length is greater than this */
#define LZSS_NIL		RING_BUFF_SIZE				/* index for root of binary search trees */


/****************************/
/*    VARIABLES             */
/****************************/

unsigned long 
		textsize = 0,								/* text size counter */
		codesize = 0,								/* code size counter */
		printcount = 0;								/* counter for reporting progress every 1K bytes */
		

unsigned char	text_buf[RING_BUFF_SIZE + F - 1];	/* ring buffer of size N,with extra F-1 bytes to facilitate string comparison */

short	*lson = nil,
		*rson = nil,
		*dad = nil; 								/* left & right children &	parents -- These constitute binary search trees. */
		






/*==============================================================================*/

long LZSS_Decode(short fRefNum, Ptr destPtr, long sourceSize)
{
short  		i, j, k, r;
unsigned short  flags;
Ptr			srcOriginalPtr;
unsigned char *sourcePtr,c;
long		decompSize = (long)destPtr;

	textsize = 0;						/* text size counter */
	codesize = 0;						/* code size counter */
	printcount = 0;						/* counter for reporting progress every 1K bytes */

				/* GET MEMORY FOR LZSS DATA */

	InitLZSSMemory();										// init internal stuff
	
	srcOriginalPtr = (Ptr)AllocPtr(sourceSize+1);
	if (srcOriginalPtr == nil)
		DoFatalAlert("\pCouldnt allocate memory for ZS pack buffer!");
	sourcePtr = (unsigned char *)srcOriginalPtr;
	
				/* READ LZSS DATA */
	
	FSRead(fRefNum,&sourceSize,srcOriginalPtr);

	
	
					/* DECOMPRESS IT */
	
	for (i = 0; i < (RING_BUFF_SIZE - F); i++)						// clear buff to "default char"? (BLG)
		text_buf[i] = ' ';

	r = RING_BUFF_SIZE - F;
	flags = 0;
	for ( ; ; )
	{
		if (((flags >>= 1) & 256) == 0)
		{
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			c = *sourcePtr++;					// get a source byte		
			flags = (unsigned short)c | 0xff00;							// uses higher byte cleverly
		}
													// to count eight
		if (flags & 1)
		{
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			c = *sourcePtr++;					// get a source byte
			*destPtr++ = c;
			text_buf[r++] = c;
			r &= (RING_BUFF_SIZE - 1);
		}
		else
		{
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			i = *sourcePtr++;					// get a source byte		
			if (--sourceSize < 0)				// see if @ end of source data
				break;
			j = *sourcePtr++;					// get a source byte		
				
			i |= ((j & 0xf0) << 4);
			j = (j & 0x0f) + THRESHOLD;
			for (k = 0; k <= j; k++)
			{
				c = text_buf[(i + k) & (RING_BUFF_SIZE - 1)];
				*destPtr++ = c;
				text_buf[r++] = c;
				r &= (RING_BUFF_SIZE - 1);
			}
		}
	}
	
	decompSize = (int)destPtr - decompSize;		// calc size of decompressed data
	
	
			/* CLEANUP */
			
	SafeDisposePtr(srcOriginalPtr);				// release the memory for packed buffer
	
	SafeDisposePtr((Ptr)lson);
	lson = nil;
	SafeDisposePtr((Ptr)rson);
	rson = nil;
	SafeDisposePtr((Ptr)dad);
	dad = nil;
	
	return(decompSize);
}

/********************* INIT LZSS MEMORY **********************/

static void InitLZSSMemory(void)
{
	if (lson == nil)
	{
		lson = (short *)AllocPtr(sizeof(short)*(RING_BUFF_SIZE + 1));
		if (lson == nil)
			DoFatalAlert("\pCouldnt alloc memory for lson!");
	}

	if (rson == nil)
	{
		rson = (short *)AllocPtr(sizeof(short)*(RING_BUFF_SIZE + 257));
		if (rson == nil)
			DoFatalAlert("\pCouldnt alloc memory for rson!");
	}
	
	if (dad == nil)
	{
		dad = (short *)AllocPtr(sizeof(short)*(RING_BUFF_SIZE + 1));
		if (dad == nil)
			DoFatalAlert("\pCouldnt alloc memory for dad!");
	}
}



