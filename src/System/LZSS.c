/****************************/
/*        LZSS.C           */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    CONSTANTS             */
/****************************/

#define RING_BUFF_SIZE		 		4096			/* size of ring buffer */

#define F		   		18							/* upper limit for match_length */
#define THRESHOLD		2   						/* encode string into position and length if match_length is greater than this */
#define LZSS_NIL		RING_BUFF_SIZE				/* index for root of binary search trees */

/*==============================================================================*/

long LZSS_Decode(short fRefNum, Ptr destPtr, long sourceSize)
{
short  		i, j, k, r;
unsigned short  flags;
Ptr			srcOriginalPtr;
unsigned char *sourcePtr,c;
Ptr			initialDestPtr = destPtr;

				/* GET MEMORY FOR LZSS DATA */

	// ring buffer of size N, with extra F-1 bytes to facilitate string comparison
	uint8_t	*text_buf	= (uint8_t*)AllocPtr(RING_BUFF_SIZE + F - 1);

	// ZS pack buffer
	srcOriginalPtr = (Ptr)AllocPtr(sourceSize+1);

	sourcePtr = (unsigned char *)srcOriginalPtr;

	GAME_ASSERT(text_buf);

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

	size_t decompSize = destPtr - initialDestPtr;		// calc size of decompressed data


			/* CLEANUP */

	SafeDisposePtr(srcOriginalPtr);				// release the memory for packed buffer
	SafeDisposePtr((Ptr)text_buf);

	return (long) decompSize;
}
