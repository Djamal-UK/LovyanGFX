/*----------------------------------------------------------------------------/
/ TJpgDec - Tiny JPEG Decompressor R0.01c                     (C)ChaN, 2019
/-----------------------------------------------------------------------------/
/ The TJpgDec is a generic JPEG decompressor module for tiny embedded systems.
/ This is a free software that opened for education, research and commercial
/  developments under license policy of following terms.
/
/  Copyright (C) 2019, ChaN, all right reserved.
/
/ * The TJpgDec module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Oct 04, 2011 R0.01  First release.
/ Feb 19, 2012 R0.01a Fixed decompression fails when scan starts with an escape seq.
/ Sep 03, 2012 R0.01b Added JD_TBLCLIP option.
/ Mar 16, 2019 R0.01c Supprted stdint.h.
/-----------------------------------------------------------------------------/
/ original source is here : http://elm-chan.org/fsw/tjpgd/00index.html
/
/ Modified for LGFX  by lovyan03, 2020
/ add support grayscale jpeg
/ add bayer pattern
/ tweak for 32bit processor
/----------------------------------------------------------------------------*/

#include "lgfx_tjpgd.h"

#include <string.h> // for memcpy memset


/*-----------------------------------------------*/
/* Zigzag-order to raster-order conversion table */
/*-----------------------------------------------*/

//#define ZIG(n)	Zig[n]

static const uint8_t Zig[64] = {	/* Zigzag-order to raster-order conversion table */
	 0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};



/*-------------------------------------------------*/
/* Input scale factor of Arai algorithm            */
/* (scaled up 16 bits for fixed point operations)  */
/*-------------------------------------------------*/

//#define IPSF(n)	Ipsf[n]

static const uint16_t Ipsf[64] = {	/* See also aa_idct.png */
	(uint16_t)(1.00000*8192), (uint16_t)(1.38704*8192), (uint16_t)(1.30656*8192), (uint16_t)(1.17588*8192), (uint16_t)(1.00000*8192), (uint16_t)(0.78570*8192), (uint16_t)(0.54120*8192), (uint16_t)(0.27590*8192),
	(uint16_t)(1.38704*8192), (uint16_t)(1.92388*8192), (uint16_t)(1.81226*8192), (uint16_t)(1.63099*8192), (uint16_t)(1.38704*8192), (uint16_t)(1.08979*8192), (uint16_t)(0.75066*8192), (uint16_t)(0.38268*8192),
	(uint16_t)(1.30656*8192), (uint16_t)(1.81226*8192), (uint16_t)(1.70711*8192), (uint16_t)(1.53636*8192), (uint16_t)(1.30656*8192), (uint16_t)(1.02656*8192), (uint16_t)(0.70711*8192), (uint16_t)(0.36048*8192),
	(uint16_t)(1.17588*8192), (uint16_t)(1.63099*8192), (uint16_t)(1.53636*8192), (uint16_t)(1.38268*8192), (uint16_t)(1.17588*8192), (uint16_t)(0.92388*8192), (uint16_t)(0.63638*8192), (uint16_t)(0.32442*8192),
	(uint16_t)(1.00000*8192), (uint16_t)(1.38704*8192), (uint16_t)(1.30656*8192), (uint16_t)(1.17588*8192), (uint16_t)(1.00000*8192), (uint16_t)(0.78570*8192), (uint16_t)(0.54120*8192), (uint16_t)(0.27590*8192),
	(uint16_t)(0.78570*8192), (uint16_t)(1.08979*8192), (uint16_t)(1.02656*8192), (uint16_t)(0.92388*8192), (uint16_t)(0.78570*8192), (uint16_t)(0.61732*8192), (uint16_t)(0.42522*8192), (uint16_t)(0.21677*8192),
	(uint16_t)(0.54120*8192), (uint16_t)(0.75066*8192), (uint16_t)(0.70711*8192), (uint16_t)(0.63638*8192), (uint16_t)(0.54120*8192), (uint16_t)(0.42522*8192), (uint16_t)(0.29290*8192), (uint16_t)(0.14932*8192),
	(uint16_t)(0.27590*8192), (uint16_t)(0.38268*8192), (uint16_t)(0.36048*8192), (uint16_t)(0.32442*8192), (uint16_t)(0.27590*8192), (uint16_t)(0.21678*8192), (uint16_t)(0.14932*8192), (uint16_t)(0.07612*8192)
};



/*---------------------------------------------*/
/* Conversion table for fast clipping process  */
/*---------------------------------------------*/

#if JD_TBLCLIP

//#define BYTECLIP(v) Clip8[(uint16_t)(v) & 0x3FF]

static const uint8_t Clip8[1024] = {
	/* 0..255 */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
	/* 256..511 */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	/* -512..-257 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* -256..-1 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#else	/* JD_TBLCLIP */

static inline int32_t BYTECLIP (
	int32_t val
)
{
	if (val < 0) val = 0;
	else if (val > 255) val = 255;
	return val;
}

#endif


/*---------------------------------------------*/
/* Output 4x4 bayer pattern table              */
/*---------------------------------------------*/

static const int_fast8_t Bayer[16] = { 0, 4, 1, 5,-2, 2,-1, 3, 1, 5, 0, 4,-1, 3,-2, 2};


/*-----------------------------------------------------------------------*/
/* Allocate a memory block from memory pool                              */
/*-----------------------------------------------------------------------*/

static uint8_t* alloc_pool (	/* Pointer to allocated memory block (NULL:no memory available) */
	lgfxJdec* jd,		/* Pointer to the decompressor object */
	uint_fast16_t nd		/* Number of bytes to allocate */
)
{
	uint8_t *rp = 0;


	nd = (nd + 3) & ~3;			/* Align block size to the word boundary */

	if (jd->sz_pool >= nd) {
		jd->sz_pool -= nd;
		rp = jd->pool;			/* Get start of available memory pool */
		jd->pool = (rp + nd);	/* Allocate requierd bytes */
	}

	return rp;	/* Return allocated memory block (NULL:no memory to allocate) */
}




/*-----------------------------------------------------------------------*/
/* Create de-quantization and prescaling tables with a DQT segment       */
/*-----------------------------------------------------------------------*/

static int32_t create_qt_tbl (	/* 0:OK, !0:Failed */
	lgfxJdec* jd,				/* Pointer to the decompressor object */
	const uint8_t* data,	/* Pointer to the quantizer tables */
	uint_fast16_t ndata		/* Size of input data */
)
{
	const uint8_t* dataend = data + ndata;
	do {	/* Process all tables in the segment */
		size_t d = *data++;							/* Get table property */
		if (d & 0xF0) return JDR_FMT1;				/* Err: not 8-bit resolution */
		int32_t *pb = (int32_t*)alloc_pool(jd, 64 * sizeof (int32_t));/* Allocate a memory block for the table */
		if (!pb) return JDR_MEM1;					/* Err: not enough memory */
		jd->qttbl[d & 3] = pb;						/* Register the table */
		for (size_t i = 0; i < 64; ++i) {			/* Load the table */
			uint_fast8_t z = Zig[i];						/* Zigzag-order to raster-order conversion */
			pb[z] = (int32_t)((uint32_t)data[i] * Ipsf[z]);	/* Apply scale factor of Arai algorithm to the de-quantizers */
		}
	} while (dataend != (data += 64));

	return JDR_OK;
}




/*-----------------------------------------------------------------------*/
/* Create huffman code tables with a DHT segment                         */
/*-----------------------------------------------------------------------*/

static int32_t create_huffman_tbl (	/* 0:OK, !0:Failed */
	lgfxJdec* jd,					/* Pointer to the decompressor object */
	const uint8_t* data,		/* Pointer to the packed huffman tables */
	int_fast16_t ndata				/* Size of input data */
)
{
	uint_fast16_t np;
	uint8_t *pb, *pd;
	uint16_t *ph;

	do {	/* Process all tables in the segment */
		uint_fast8_t d = *data++;			/* Get table number and class */
		if (d & 0xEE) return JDR_FMT1;		/* Err: invalid class/number */
		uint_fast8_t cls = d >> 4;			/* class = dc(0)/ac(1), table number = 0/1 */
		uint_fast8_t num = d & 0x0F;
		pb = alloc_pool(jd, 16);			/* Allocate a memory block for the bit distribution table */
		if (!pb) return JDR_MEM1;			/* Err: not enough memory */
		jd->huffbits[num][cls] = pb - 1;
		np = 0;
		size_t i = 0;
		do {								/* Load number of patterns for 1 to 16-bit code */
			np += (pb[i] = data[i]);		/* Get sum of code words for each code */
		} while (++i < 16);

		ph = (uint16_t*)alloc_pool(jd, np * sizeof (uint16_t));/* Allocate a memory block for the code word table */
		if (!ph) return JDR_MEM1;			/* Err: not enough memory */
		jd->huffcode[num][cls] = ph - 1;
		uint_fast16_t hc = 0;
		i = 0;
		do {								/* Re-build huffman code word table */
			size_t b = pb[i];
			while (b--) *ph++ = hc++;
			hc <<= 1;
		} while (++i < 16);

		pd = alloc_pool(jd, np);			/* Allocate a memory block for the decoded data */
		if (!pd) return JDR_MEM1;			/* Err: not enough memory */
		jd->huffdata[num][cls] = pd - 1;

		memcpy(pd, data += 16, np);			/* Load decoded data corresponds to each code ward */
		data += np;
	} while (ndata -= 17 + np);

	return JDR_OK;
}




/*-----------------------------------------------------------------------*/
/* Extract N bits from input stream                                      */
/*-----------------------------------------------------------------------*/

static int32_t bitext (	/* >=0: extracted data, <0: error code */
	lgfxJdec* jd,		/* Pointer to the decompressor object */
	uint_fast8_t nbit		/* Number of bits to extract (1 to 11) */
)
{
	uint8_t *dp;
	uint_fast8_t msk;
	uint32_t v;

	msk = jd->dmsk; dp = jd->dptr;
	v = 0;

	for (;;) {
		if (!msk) {				/* Next byte? */
			uint8_t *dpend = jd->dpend;
			if (++dp == dpend) {	/* No input data is available, re-fill input buffer */
				dp = jd->inbuf;	/* Top of input buffer */
				jd->dpend = dpend = dp + jd->infunc(jd, dp, JD_SZBUF);
				if (dp == dpend) return 0 - (int32_t)JDR_INP;	/* Err: read error or wrong stream termination */
			}
			if (*dp == 0xff) {		/* Is start of flag sequence? */
				if (++dp == dpend) {	/* No input data is available, re-fill input buffer */
					dp = jd->inbuf;	/* Top of input buffer */
					jd->dpend = dpend = dp + jd->infunc(jd, dp, JD_SZBUF);
					if (dp == dpend) return 0 - (int32_t)JDR_INP;	/* Err: read error or wrong stream termination */
				}
				if (*dp != 0) return 0 - (int32_t)JDR_FMT1;	/* Err: unexpected flag is detected (may be collapted data) */
				*dp = 0xff;			/* The flag is a data 0xFF */
			}
			msk = 8;			/* Read from MSB */
		}
		uint_fast8_t s = *dp;	/* Get next data byte */
		if (msk >= nbit) {
			msk -= nbit;
			jd->dmsk = msk; jd->dptr = dp;
			return v + ((s >> msk) & ((1 << nbit) - 1));	/* Get bits */
		}
		nbit -= msk;
		v += (s & ((1 << msk) - 1)) << nbit;	/* Get bits */
		msk = 0;
	}
}


/*-----------------------------------------------------------------------*/
/* Extract a huffman decoded data from input stream                      */
/*-----------------------------------------------------------------------*/

static int32_t huffext (	/* >=0: decoded data, <0: error code */
	lgfxJdec* jd,				/* Pointer to the decompressor object */
	const uint8_t* hbits,	/* Pointer to the bit distribution table */
	const uint16_t* hcode,	/* Pointer to the code word table */
	const uint8_t* hdata	/* Pointer to the data table */
)
{
	uint8_t *dp;
	uint_fast8_t msk, bl;
	uint32_t v;

	msk = jd->dmsk; dp = jd->dptr;
	v = 0;
	bl = 16;	/* Max code length */
	for (;;) {
		if (!msk) {				/* Next byte? */
			uint8_t *dpend = jd->dpend;
			if (++dp == dpend) {	/* No input data is available, re-fill input buffer */
				dp = jd->inbuf;	/* Top of input buffer */
				jd->dpend = dpend = dp + jd->infunc(jd, dp, JD_SZBUF);
				if (dp == dpend) return 0 - (int32_t)JDR_INP;	/* Err: read error or wrong stream termination */
			}
			if (*dp == 0xff) {		/* Is start of flag sequence? */
				if (++dp == dpend) {	/* No input data is available, re-fill input buffer */
					dp = jd->inbuf;	/* Top of input buffer */
					jd->dpend = dpend = dp + jd->infunc(jd, dp, JD_SZBUF);
					if (dp == dpend) return 0 - (int32_t)JDR_INP;	/* Err: read error or wrong stream termination */
				}
				if (*dp != 0) return 0 - (int32_t)JDR_FMT1;	/* Err: unexpected flag is detected (may be collapted data) */
				*dp = 0xff;			/* The flag is a data 0xFF */
			}
			msk = 8;			/* Read from MSB */
		}
		uint_fast8_t s = *dp;	/* Get next data byte */
		do {
			v = (v << 1) + ((s >> (--msk)) & 1);	/* Get a bit */
			size_t nd = *++hbits;
			if (nd) {
				do {	/* Search the code word in this bit length */
					++hdata;
					if (v == *++hcode) goto huffext_match;	/* Matched? */
				} while (--nd);
			}
			if (!--bl) return 0 - (int32_t)JDR_FMT1;	/* Err: code not found (may be collapted data) */
		} while (msk);
	}
huffext_match:
	jd->dmsk = msk;
	jd->dptr = dp;
	return *hdata;					/* Return the decoded data */
}




/*-----------------------------------------------------------------------*/
/* Apply Inverse-DCT in Arai Algorithm (see also aa_idct.png)            */
/*-----------------------------------------------------------------------*/

static void block_idct (
	int32_t* src,	/* Input block data (de-quantized and pre-scaled for Arai Algorithm) */
	uint8_t* dst	/* Pointer to the destination to store the block as byte array */
)
{
	const int32_t M13 = (int32_t)(1.41421*256), M2 = (int32_t)(1.08239*256), M4 = (int32_t)(2.61313*256), M5 = (int32_t)(1.84776*256);
	int32_t v0, v1, v2, v3, v4, v5, v6, v7;
	int32_t t10, t11, t12, t13;

	/* Process columns */
	for (int i = 0; i < 8; ++i) {
		/* Get and Process the odd elements */
		v4 = src[8 * 7];
		v5 = src[8 * 1];
		v6 = src[8 * 5];
		v7 = src[8 * 3];

		t10 = v5 - v4;
		t11 = v5 + v4;
		t12 = v6 - v7;
		v7 += v6;
		v5 = (t11 - v7) * M13 >> 8;
		t13 = (t10 + t12) * M5 >> 8;
		v6 = t13 - ((t12 * M4 >> 8) + (v7 += t11));
		v4 = t13 - ((t10 * M2 >> 8) + (v5 -= v6));

		/* Get and Process the even elements */
		v0 = src[8 * 0];
		v2 = src[8 * 4];
		t10 = v0 + v2;
		t12 = v0 - v2;

		v1 = src[8 * 2];
		v3 = src[8 * 6];
		t11 = (v1 - v3) * M13 >> 8;
		v3 += v1;
		t11 -= v3;

		v0 = t10 + v3;
		v3 = t10 - v3;
		v1 = t12 + t11;
		v2 = t12 - t11;

		/* Write-back transformed values */
		src[8 * 0] = v0 + v7;
		src[8 * 7] = v0 - v7;
		src[8 * 1] = v1 + v6;
		src[8 * 6] = v1 - v6;
		src[8 * 2] = v2 + v5;
		src[8 * 5] = v2 - v5;
		src[8 * 3] = v3 + v4;
		src[8 * 4] = v3 - v4;

		++src;	/* Next column */
	}

	/* Process rows */
	src -= 8;
	for (int i = 0; i < 8; ++i) {
		/* Get and Process the odd elements */
		v4 = src[1];
		v5 = src[7] + v4;
		v4 = (v4 << 1) - v5;

		v6 = src[5];
		v7 = src[3] + v6;
		v6 = (v6 << 1) - v7;

		v7 += v5;
		v5 = (v5 << 1) - v7;

		t13 = v4 + v6;
		t13 = t13 * M5 >> 8;
		v6 = v6 * M4 >> 8;
		v6 += v7;
		v6 = t13 - v6;
		v5 = v5 * M13 >> 8;
		v5 -= v6;
		v4 = v4 * M2 >> 8;
		v4 += v5;
		v4 = t13 - v4;

		/* Get and Process the even elements */
		v0 = src[0] + (128L << 8);	/* remove DC offset (-128) here */
		v2 = src[4];
		t10 = v0 + v2;
		t12 = v0 - v2;

		v1 = src[2];
		v3 = src[6] + v1;
		t11 = (v1 << 1) - v3;
		t11 = t11 * M13 >> 8;
		t11 -= v3;

		v0 = t10 + v3;
		v3 = t10 - v3;
		v1 = t12 + t11;
		v2 = t12 - t11;

		/* Descale the transformed values 8 bits and output */
#if defined (ESP32) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S2) || defined (ESP_PLATFORM)
		int32_t d0 = (v0 + v7) >> 8;
		int32_t d7 = (v0 - v7) >> 8;
		int32_t d1 = (v1 + v6) >> 8;
		int32_t d6 = (v1 - v6) >> 8;
		int32_t d2 = (v2 + v5) >> 8;
		int32_t d5 = (v2 - v5) >> 8;
		int32_t d3 = (v3 + v4) >> 8;
		int32_t d4 = (v3 - v4) >> 8;

		if (d0 < 0) d0 = 0; else if (d0 > 255) d0 = 255;
		if (d1 < 0) d1 = 0; else if (d1 > 255) d1 = 255;
		if (d2 < 0) d2 = 0; else if (d2 > 255) d2 = 255;
		if (d3 < 0) d3 = 0; else if (d3 > 255) d3 = 255;
		if (d4 < 0) d4 = 0; else if (d4 > 255) d4 = 255;
		if (d5 < 0) d5 = 0; else if (d5 > 255) d5 = 255;
		if (d6 < 0) d6 = 0; else if (d6 > 255) d6 = 255;
		if (d7 < 0) d7 = 0; else if (d7 > 255) d7 = 255;

		dst[0] = d0;
		dst[1] = d1;
		dst[2] = d2;
		dst[3] = d3;
		dst[4] = d4;
		dst[5] = d5;
		dst[6] = d6;
		dst[7] = d7;
#else
		dst[0] = BYTECLIP((v0 + v7) >> 8);
		dst[7] = BYTECLIP((v0 - v7) >> 8);
		dst[1] = BYTECLIP((v1 + v6) >> 8);
		dst[6] = BYTECLIP((v1 - v6) >> 8);
		dst[2] = BYTECLIP((v2 + v5) >> 8);
		dst[5] = BYTECLIP((v2 - v5) >> 8);
		dst[3] = BYTECLIP((v3 + v4) >> 8);
		dst[4] = BYTECLIP((v3 - v4) >> 8);
#endif
		dst += 8;
		src += 8;	/* Next row */
	}
}




/*-----------------------------------------------------------------------*/
/* Load all blocks in the MCU into working buffer                        */
/*-----------------------------------------------------------------------*/

static JRESULT mcu_load (
	lgfxJdec* jd		/* Pointer to the decompressor object */
)
{
	int32_t *tmp = (int32_t*)jd->workbuf;	/* Block working buffer for de-quantize and IDCT */
	int32_t b, d, e;
	uint32_t blk, nby, nbc;
	uint8_t *bp;
	const uint8_t *hb, *hd;
	const uint16_t *hc;


	nby = jd->msx * jd->msy;		/* Number of Y blocks (1, 2 or 4) */
	nbc = jd->comps_in_frame - 1;	/* Number of C blocks (2 or 0(grayscale)) */
	bp = jd->mcubuf;				/* Pointer to the first block */

	for (blk = 0; blk < nby + nbc; ++blk) {
		size_t cmp = (blk < nby) ? 0 : blk - nby + 1;	/* Component number 0:Y, 1:Cb, 2:Cr */
		size_t id = cmp ? 1 : 0;				/* Huffman table ID of the component */

		/* Extract a DC element from input stream */
		hb = jd->huffbits[id][0];				/* Huffman table for the DC element */
		hc = jd->huffcode[id][0];
		hd = jd->huffdata[id][0];
		b = huffext(jd, hb, hc, hd);			/* Extract a huffman coded data (bit length) */
		if (b < 0) return (JRESULT)(-b);		/* Err: invalid code or input */
		d = jd->dcv[cmp];						/* DC value of previous block */
		if (b) {								/* If there is any difference from previous block */
			e = bitext(jd, b);					/* Extract data bits */
			if (e < 0) return (JRESULT)(-e);	/* Err: input */
			b = 1 << (b - 1);					/* MSB position */
			if (!(e & b)) e -= (b << 1) - 1;	/* Restore sign if needed */
			d += e;								/* Get current value */
			jd->dcv[cmp] = d;					/* Save current DC value for next block */
		}
		const int32_t *dqf = jd->qttbl[jd->qtid[cmp]];			/* De-quantizer table ID for this component */
		tmp[0] = d * dqf[0] >> 8;				/* De-quantize, apply scale factor of Arai algorithm and descale 8 bits */

		/* Extract following 63 AC elements from input stream */
		memset(&tmp[1], 0, 63*sizeof(int32_t));	/* Clear rest of elements */
		hb = jd->huffbits[id][1];				/* Huffman table for the AC elements */
		hc = jd->huffcode[id][1];
		hd = jd->huffdata[id][1];
		uint_fast8_t i = 1;					/* Top of the AC elements */
		do {
			b = huffext(jd, hb, hc, hd);		/* Extract a huffman coded value (zero runs and bit length) */
			if (b == 0) break;					/* EOB? */
			if (b < 0) return (JRESULT)(-b);	/* Err: invalid code or input error */
			i += b >> 4;						/* Number of leading zero elements   Skip zero elements */
			if (b &= 0x0F) {					/* Bit length */
				d = bitext(jd, b);				/* Extract data bits */
				if (d < 0) return (JRESULT)(-d);/* Err: input device */
				b = 1 << (b - 1);				/* MSB position */
				if (!(d & b)) d -= (b << 1) - 1;/* Restore negative value if needed */
				uint_fast8_t z = Zig[i];		/* Zigzag-order to raster-order converted index */
				tmp[z] = d * dqf[z] >> 8;		/* De-quantize, apply scale factor of Arai algorithm and descale 8 bits */
			}
		} while (++i < 64);		/* Next AC element */

		if (JD_USE_SCALE && jd->scale == 3) {
			*bp = (uint8_t)((*tmp >> 8) + 128);	/* If scale ratio is 1/8, IDCT can be ommited and only DC element is used */
		} else {
			block_idct(tmp, bp);		/* Apply IDCT and store the block to the MCU buffer */
		}

		bp += 64;				/* Next block */
	}

	return JDR_OK;	/* All blocks have been loaded successfully */
}




/*-----------------------------------------------------------------------*/
/* Output an MCU: Convert YCrCb to RGB and output it in RGB form         */
/*-----------------------------------------------------------------------*/

static JRESULT mcu_output (
	lgfxJdec* jd,		/* Pointer to the decompressor object */
	uint32_t (*outfunc)(lgfxJdec*, void*, JRECT*),	/* RGB output function */
	uint32_t x,		/* MCU position in the image (left of the MCU) */
	uint32_t y		/* MCU position in the image (top of the MCU) */
)
{
	const int_fast16_t FP_SHIFT = 8;
	uint32_t ix, iy, mx, my, rx, ry;
	int32_t yy, cb, cr;
	uint8_t *py, *pc, *rgb24;
	JRECT rect;

	mx = jd->msx << 3; my = jd->msy << 3;					/* MCU size (pixel) */
	rx = (mx < jd->width - x) ? mx : jd->width - x;	/* Output rectangular size (it may be clipped at right/bottom end) */
	ry = (my < jd->height - y) ? my : jd->height - y;

	if (JD_USE_SCALE) {
		rx >>= jd->scale; ry >>= jd->scale;
		if (!rx || !ry) return JDR_OK;					/* Skip this MCU if all pixel is to be rounded off */
		x >>= jd->scale; y >>= jd->scale;
	}
	rect.left = x; rect.right = x + rx - 1;				/* Rectangular area in the frame buffer */
	rect.top = y; rect.bottom = y + ry - 1;

	uint8_t* workbuf = (uint8_t*)jd->workbuf;

	if (!JD_USE_SCALE || jd->scale != 3) {	/* Not for 1/8 scaling */

		uint_fast8_t ixshift = (mx == 16);
		uint_fast8_t iyshift = (my == 16);

		/* Build an RGB MCU from discrete comopnents */
		rgb24 = workbuf;
		iy = 0;
		do {
#if JD_BAYER
			const int_fast8_t* btbl = &Bayer[(iy & 3) << 2];
#endif
			py = &jd->mcubuf[((iy & 8) + iy) << 3];
			pc = &jd->mcubuf[((mx << iyshift) + (iy >> iyshift)) << 3];
			ix = 0;
			do {
				do {
					cb = (pc[ 0] - 128); 	/* Get Cb/Cr component and restore right level */
					cr = (pc[64] - 128);
					++pc;

				/* Convert CbCr to RGB */
					uint_fast16_t rr = ((int32_t)(1.402   * (1<<FP_SHIFT)) * cr) >> FP_SHIFT;
					uint_fast16_t gg = ((int32_t)(0.34414 * (1<<FP_SHIFT)) * cb
									  + (int32_t)(0.71414 * (1<<FP_SHIFT)) * cr) >> FP_SHIFT;
					uint_fast16_t bb = ((int32_t)(1.772   * (1<<FP_SHIFT)) * cb) >> FP_SHIFT;
					do {
#if JD_BAYER
						yy = *py + btbl[ix & 3];		/* Get Y component */
#else
						yy = *py;					/* Get Y component */
#endif
						++py;
					/* Convert YCbCr to RGB */
						rgb24[0] = BYTECLIP(yy + rr);
						rgb24[1] = BYTECLIP(yy - gg);
						rgb24[2] = BYTECLIP(yy + bb);
						rgb24 += 3;
					} while (++ix & ixshift);
				} while (ix & 7);
				py += 64 - 8;	/* Jump to next block if double block heigt */
			} while (ix != mx);
		} while (++iy < my);

		/* Descale the MCU rectangular if needed */
		if (JD_USE_SCALE && jd->scale) {
			uint32_t x, y, r, g, b, s, w;
			uint8_t *op;

			/* Get averaged RGB value of each square correcponds to a pixel */
			s = jd->scale * 2;	/* Bumber of shifts for averaging */
			w = 1 << jd->scale;	/* Width of square */
			op = workbuf;
			iy = 0;
			do {
				ix = 0;
				do {
					rgb24 = &workbuf[(iy * mx + ix) * 3];
					r = g = b = 0;
					y = 0;
					do {	/* Accumulate RGB value in the square */
						x = 0;
						do {
							r += rgb24[x*3  ];
							g += rgb24[x*3+1];
							b += rgb24[x*3+2];
						} while (++x < w);
						rgb24 += mx * 3;
					} while (++y < w);
					/* Put the averaged RGB value as a pixel */
					op[0] = r >> s;
					op[1] = g >> s;
					op[2] = b >> s;
					op += 3;
				} while ((ix += w) < mx);
			} while ((iy += w) < my);
		}

	} else {	/* For only 1/8 scaling (left-top pixel in each block are the DC value of the block) */

		/* Build a 1/8 descaled RGB MCU from discrete comopnents */
		rgb24 = workbuf;
		pc = jd->mcubuf + mx * my;
		cb = pc[0] - 128;		/* Get Cb/Cr component and restore right level */
		cr = pc[64] - 128;
		iy = 0;
		do {
			py = jd->mcubuf;
			if (iy == 8) py += 64 * 2;
			ix = 0;
			do {
				yy = *py;	/* Get Y component */
				py += 64;

				/* Convert YCbCr to RGB */
				rgb24[0] = BYTECLIP(yy + (((int32_t)(1.402   * (1<<FP_SHIFT)) * cr) >> FP_SHIFT));
				rgb24[1] = BYTECLIP(yy - (((int32_t)(0.34414 * (1<<FP_SHIFT)) * cb
										 + (int32_t)(0.71414 * (1<<FP_SHIFT)) * cr) >> FP_SHIFT));
				rgb24[2] = BYTECLIP(yy + (((int32_t)(1.772   * (1<<FP_SHIFT)) * cb) >> FP_SHIFT));
				rgb24 += 3;
			} while ((ix += 8) < mx);
		} while ((iy += 8) < my);
	}

	/* Squeeze up pixel table if a part of MCU is to be truncated */
	mx >>= jd->scale;
	if (rx < mx) {
		uint8_t *s, *d;
		s = d = workbuf;
		for (size_t y = 1; y < ry; ++y) {
			memcpy(d += rx * 3, s += mx * 3, rx * 3);	/* Copy effective pixels */
		}
	}

	/* Convert RGB888 to RGB565 if needed */
	if (JD_FORMAT == 1) {
		uint8_t *s = workbuf;
		uint16_t *d = (uint16_t*)s;
		uint_fast16_t w;
		uint_fast16_t n = rx * ry;

		do {
			w = (*s++ & 0xF8) << 8;		/* RRRRR----------- */
			w |= (*s++ & 0xFC) << 3;	/* -----GGGGGG----- */
			w |= *s++ >> 3;				/* -----------BBBBB */
			*d++ = w;
		} while (--n);
	}

	/* Output the RGB rectangular */
	return outfunc(jd, workbuf, &rect) ? JDR_OK : JDR_INTR; 
}




/*-----------------------------------------------------------------------*/
/* Process restart interval                                              */
/*-----------------------------------------------------------------------*/

static JRESULT restart (
	lgfxJdec* jd,		/* Pointer to the decompressor object */
	uint16_t rstn	/* Expected restert sequense number */
)
{
	uint16_t d;
	uint8_t *dp = jd->dptr, *dpend = jd->dpend;

	/* Discard padding bits and get two bytes from the input stream */
	d = 0;
	for (int i = 0; i < 2; ++i) {
		if (++dp == dpend) {	/* No input data is available, re-fill input buffer */
			dp = jd->inbuf;
			jd->dpend = dpend = dp + jd->infunc(jd, dp, JD_SZBUF);
			if (dp == dpend) return JDR_INP;
		}
		d = (d << 8) | *dp;	/* Get a byte */
	}
	jd->dptr = dp; jd->dmsk = 0;

	/* Check the marker */
	if ((d & 0xFFD8) != 0xFFD0 || (d & 7) != (rstn & 7)) {
		return JDR_FMT1;	/* Err: expected RSTn marker is not detected (may be collapted data) */
	}

	/* Reset DC offset */
	jd->dcv[2] = jd->dcv[1] = jd->dcv[0] = 0;

	return JDR_OK;
}




/*-----------------------------------------------------------------------*/
/* Analyze the JPEG image and Initialize decompressor object             */
/*-----------------------------------------------------------------------*/

//#define	LDB_WORD(ptr) (uint16_t)(((uint16_t)*((uint8_t*)(ptr))<<8)|(uint16_t)*(uint8_t*)((ptr)+1))

static inline uint16_t LDB_WORD(uint8_t* ptr) {
  return ptr[0]<<8 | ptr[1];
}


JRESULT lgfx_jd_prepare (
	lgfxJdec* jd,			/* Blank decompressor object */
	uint32_t (*infunc)(lgfxJdec*, uint8_t*, uint32_t),	/* JPEG strem input function */
	void* pool,			/* Working buffer for the decompression session */
	uint_fast16_t sz_pool,	/* Size of working buffer */
	void* dev			/* I/O device identifier for the session */
)
{
	uint8_t *seg;
	uint32_t ofs;
	size_t n;
	int32_t rc;


	if (!pool) return JDR_PAR;

	jd->pool = (uint8_t*)pool;		/* Work memroy */
	jd->sz_pool = sz_pool;	/* Size of given work memory */
	jd->infunc = infunc;	/* Stream input function */
	jd->device = dev;		/* I/O device identifier */
	jd->nrst = 0;			/* No restart interval (default) */

//	memset(jd->huffbits, 0, sizeof(uint8_t*) * 4);	/* Nulls pointers */
//	memset(jd->huffcode, 0, sizeof(uint16_t*) * 4);
//	memset(jd->huffdata, 0, sizeof(uint8_t*) * 4);
//	memset(jd->qttbl, 0, sizeof(uint32_t*) * 4);

	jd->inbuf = seg = alloc_pool(jd, JD_SZBUF);		/* Allocate stream input buffer */
	if (!seg) return JDR_MEM1;

	if (infunc(jd, seg, 2) != 2) return JDR_INP;/* Check SOI marker */
	if (LDB_WORD(seg) != 0xFFD8) return JDR_FMT1;	/* Err: SOI is not detected */
	ofs = 2;

	for (;;) {
		if (infunc(jd, seg, 4) != 4) return JDR_INP;
		uint_fast16_t len = LDB_WORD(seg + 2) - 2;	/* Length field */
		if (seg[0] != 0xFF) return JDR_FMT1;	/* Check a JPEG marker */
		ofs += 4 + len;	/* Number of bytes loaded */

		switch (seg[1]) {	/* Marker */
		case 0xC0:	/* SOF0 (baseline JPEG) */
			/* Load segment data */
			if (len > JD_SZBUF) return JDR_MEM2;
			if (infunc(jd, seg, len) != len) return JDR_INP;

			jd->width = LDB_WORD(seg+3);		/* Image width in unit of pixel */
			jd->height = LDB_WORD(seg+1);		/* Image height in unit of pixel */
			jd->comps_in_frame = seg[5];
			if (seg[5] != 1 && seg[5] != 3)
				return JDR_FMT3;	/* Err: Supports only Y/Cb/Cr or Y(Grayscale) format */

			/* Check three image components */
			for (size_t i = 0; i < seg[5]; ++i) {
				uint_fast8_t b = seg[7 + 3 * i];							/* Get sampling factor */
				if (!i) {	/* Y component */
					if (b != 0x11 && b != 0x22 && b != 0x21) {	/* Check sampling factor */
						return JDR_FMT3;					/* Err: Supports only 4:4:4, 4:2:0 or 4:2:2 */
					}
					jd->msx = b >> 4; jd->msy = b & 15;		/* Size of MCU [blocks] */
				} else {	/* Cb/Cr component */
					if (b != 0x11) return JDR_FMT3;			/* Err: Sampling factor of Cr/Cb must be 1 */
				}
				b = seg[8 + 3 * i];							/* Get dequantizer table ID for this component */
				if (b > 3) return JDR_FMT3;					/* Err: Invalid ID */
				jd->qtid[i] = b;
			}
			break;

		case 0xDD:	/* DRI */
			/* Load segment data */
			if (len > JD_SZBUF) return JDR_MEM2;
			if (infunc(jd, seg, len) != len) return JDR_INP;

			/* Get restart interval (MCUs) */
			jd->nrst = LDB_WORD(seg);
			break;

		case 0xC4:	/* DHT */
			/* Load segment data */
			if (len > JD_SZBUF) return JDR_MEM2;
			if (infunc(jd, seg, len) != len) return JDR_INP;

			/* Create huffman tables */
			rc = create_huffman_tbl(jd, seg, len);
			if (rc) return (JRESULT)rc;
			break;

		case 0xDB:	/* DQT */
			/* Load segment data */
			if (len > JD_SZBUF) return JDR_MEM2;
			if (infunc(jd, seg, len) != len) return JDR_INP;

			/* Create de-quantizer tables */
			rc = create_qt_tbl(jd, seg, len);
			if (rc) return (JRESULT)rc;
			break;

		case 0xDA:	/* SOS */
			/* Load segment data */
			if (len > JD_SZBUF) return JDR_MEM2;
			if (infunc(jd, seg, len) != len) return JDR_INP;

			if (!jd->width || !jd->height) return JDR_FMT1;	/* Err: Invalid image size */

			if (seg[0] != jd->comps_in_frame) return JDR_FMT3;	/* Err: Supports only three color or grayscale components format */

			/* Check if all tables corresponding to each components have been loaded */
			for (size_t i = 0; i < jd->comps_in_frame; ++i) {
				uint_fast8_t b = seg[2 + 2 * i];	/* Get huffman table ID */
				if (b != 0x00 && b != 0x11)	return JDR_FMT3;	/* Err: Different table number for DC/AC element */
				b = i ? 1 : 0;
				if (!jd->huffbits[b][0] || !jd->huffbits[b][1]) {	/* Check dc/ac huffman table for this component */
					return JDR_FMT1;					/* Err: Nnot loaded */
				}
				if (!jd->qttbl[jd->qtid[i]]) {			/* Check dequantizer table for this component */
					return JDR_FMT1;					/* Err: Not loaded */
				}
			}

			/* Allocate working buffer for MCU and RGB */
			n = jd->msy * jd->msx;						/* Number of Y blocks in the MCU */
			if (!n) return JDR_FMT1;					/* Err: SOF0 has not been loaded */
			len = n * 64 * 2 + 64;						/* Allocate buffer for IDCT and RGB output */
			if (len < 256) len = 256;					/* but at least 256 byte is required for IDCT */
			jd->workbuf = alloc_pool(jd, len);			/* and it may occupy a part of following MCU working buffer for RGB output */
			if (!jd->workbuf) return JDR_MEM1;			/* Err: not enough memory */
			jd->mcubuf = (uint8_t*)alloc_pool(jd, (n + 2) * 64);	/* Allocate MCU working buffer */
			if (!jd->mcubuf) return JDR_MEM1;			/* Err: not enough memory */
			if (jd->comps_in_frame == 1) {
				memset(&jd->mcubuf[64], 128, 128);		/* Cb/Cr clear ( for grayscale )*/
			}

			/* Pre-load the JPEG data to extract it from the bit stream */
			ofs %= JD_SZBUF;						/* Align read offset to JD_SZBUF */
			int32_t dc = infunc(jd, seg + ofs, JD_SZBUF - ofs);
			jd->dptr = seg + ofs - 1;
			jd->dpend = seg + ofs + dc;
			jd->dmsk = 0;	/* Prepare to read bit stream */

			return JDR_OK;		/* Initialization succeeded. Ready to decompress the JPEG image. */

		case 0xC1:	/* SOF1 */
		case 0xC2:	/* SOF2 */
		case 0xC3:	/* SOF3 */
		case 0xC5:	/* SOF5 */
		case 0xC6:	/* SOF6 */
		case 0xC7:	/* SOF7 */
		case 0xC9:	/* SOF9 */
		case 0xCA:	/* SOF10 */
		case 0xCB:	/* SOF11 */
		case 0xCD:	/* SOF13 */
		case 0xCE:	/* SOF14 */
		case 0xCF:	/* SOF15 */
		case 0xD9:	/* EOI */
			return JDR_FMT3;	/* Unsuppoted JPEG standard (may be progressive JPEG) */

		default:	/* Unknown segment (comment, exif or etc..) */
			/* Skip segment data */
			if (infunc(jd, 0, len) != len) {	/* Null pointer specifies to skip bytes of stream */
				return JDR_INP;
			}
		}
	}
}




/*-----------------------------------------------------------------------*/
/* Start to decompress the JPEG picture                                  */
/*-----------------------------------------------------------------------*/

JRESULT lgfx_jd_decomp (
	lgfxJdec* jd,								/* Initialized decompression object */
	uint32_t (*outfunc)(lgfxJdec*, void*, JRECT*),	/* RGB output function */
	uint_fast8_t scale							/* Output de-scaling factor (0 to 3) */
)
{
	uint32_t x, y, mx, my;
	uint32_t nrst, rst, rsc;
	JRESULT rc;


	if (scale > (JD_USE_SCALE ? 3 : 0)) return JDR_PAR;
	jd->scale = scale;

	nrst = jd->nrst;
	mx = jd->msx << 3; my = jd->msy << 3;			/* Size of the MCU (pixel) */

	jd->dcv[2] = jd->dcv[1] = jd->dcv[0] = 0;	/* Initialize DC values */
	rst = rsc = 0;

	rc = JDR_OK;

	for (y = 0; y < jd->height; y += my) {		/* Vertical loop of MCUs */
		x = 0;
		do {	/* Horizontal loop of MCUs */
			if (nrst && rst++ == nrst) {	/* Process restart interval if enabled */
				rc = restart(jd, rsc++);
				if (rc != JDR_OK) return rc;
				rst = 1;
			}
			rc = mcu_load(jd);					/* Load an MCU (decompress huffman coded stream and apply IDCT) */
			if (rc != JDR_OK) return rc;
			rc = mcu_output(jd, outfunc, x, y);	/* Output the MCU (color space conversion, scaling and output) */
			if (rc != JDR_OK) return rc;
		} while ( (x += mx) < jd->width);
	}

	return rc;
}



