/* Rijndael Cipher

   Written by Mike Scott 21st April 1999
   Copyright (c) 1999 Mike Scott
   See rijndael documentation

   Permission for free direct or derivative use is granted subject 
   to compliance with any conditions that the originators of the 
   algorithm place on its exploitation.  

   Inspiration from Brian Gladman's implementation is acknowledged.

   Written for clarity, rather than speed.
   Assumes long is 32 bit quantity.
   Full implementation. 
   Endian indifferent.
   
   adopted for didentd 000410 by drt@un.bewaff.net

   $Id: rijndael.c,v 1.2 2000/11/21 19:28:23 drt Exp $

   $Log: rijndael.c,v $
   Revision 1.2  2000/11/21 19:28:23  drt
   Changed Email Address from drt@ailis.de to drt@un.bewaff.net

   Revision 1.1  2000/04/30 15:59:26  drt
   cleand up usage of djb stuff

   Revision 1.1.1.1  2000/04/16 17:38:10  drt
   initial ddns version

   Revision 1.1.1.1  2000/04/12 16:07:17  drt
   initial revision

*/

#define NO_TEST
#define STATICTABLES

#ifdef TEST
#include <stdio.h>
#endif

#define BYTE unsigned char
#define WORD unsigned long

/* rotates x one bit to the left */

#define ROTL(x) (((x)>>7)|((x)<<1))

/* Rotates 32-bit word left by 1, 2 or 3 byte  */

#define ROTL8(x) (((x)<<8)|((x)>>24))
#define ROTL16(x) (((x)<<16)|((x)>>16))
#define ROTL24(x) (((x)<<24)|((x)>>8))

/* Fixed Data */

static BYTE InCo[4]={0xB,0xD,0x9,0xE};  /* Inverse Coefficients */
static WORD rco[30];

#ifndef STATICTABLES
static BYTE fbsub[256];
static BYTE rbsub[256];
static BYTE ptab[256],ltab[256];
static WORD ftable[256];
static WORD rtable[256];
#else
static BYTE ltab[]={0x0,
		    0xff,0x19,0x1,0x32,0x2,0x1a,0xc6,0x4b,0xc7,0x1b,0x68,0x33,0xee,0xdf,0x3,0x64,
		    0x4,0xe0,0xe,0x34,0x8d,0x81,0xef,0x4c,0x71,0x8,0xc8,0xf8,0x69,0x1c,0xc1,0x7d,
		    0xc2,0x1d,0xb5,0xf9,0xb9,0x27,0x6a,0x4d,0xe4,0xa6,0x72,0x9a,0xc9,0x9,0x78,0x65,
		    0x2f,0x8a,0x5,0x21,0xf,0xe1,0x24,0x12,0xf0,0x82,0x45,0x35,0x93,0xda,0x8e,0x96,
		    0x8f,0xdb,0xbd,0x36,0xd0,0xce,0x94,0x13,0x5c,0xd2,0xf1,0x40,0x46,0x83,0x38,0x66,
		    0xdd,0xfd,0x30,0xbf,0x6,0x8b,0x62,0xb3,0x25,0xe2,0x98,0x22,0x88,0x91,0x10,0x7e,
		    0x6e,0x48,0xc3,0xa3,0xb6,0x1e,0x42,0x3a,0x6b,0x28,0x54,0xfa,0x85,0x3d,0xba,0x2b,
		    0x79,0xa,0x15,0x9b,0x9f,0x5e,0xca,0x4e,0xd4,0xac,0xe5,0xf3,0x73,0xa7,0x57,0xaf,
		    0x58,0xa8,0x50,0xf4,0xea,0xd6,0x74,0x4f,0xae,0xe9,0xd5,0xe7,0xe6,0xad,0xe8,0x2c,
		    0xd7,0x75,0x7a,0xeb,0x16,0xb,0xf5,0x59,0xcb,0x5f,0xb0,0x9c,0xa9,0x51,0xa0,0x7f,
		    0xc,0xf6,0x6f,0x17,0xc4,0x49,0xec,0xd8,0x43,0x1f,0x2d,0xa4,0x76,0x7b,0xb7,0xcc,
		    0xbb,0x3e,0x5a,0xfb,0x60,0xb1,0x86,0x3b,0x52,0xa1,0x6c,0xaa,0x55,0x29,0x9d,0x97,
		    0xb2,0x87,0x90,0x61,0xbe,0xdc,0xfc,0xbc,0x95,0xcf,0xcd,0x37,0x3f,0x5b,0xd1,0x53,
		    0x39,0x84,0x3c,0x41,0xa2,0x6d,0x47,0x14,0x2a,0x9e,0x5d,0x56,0xf2,0xd3,0xab,0x44,
		    0x11,0x92,0xd9,0x23,0x20,0x2e,0x89,0xb4,0x7c,0xb8,0x26,0x77,0x99,0xe3,0xa5,0x67,
		    0x4a,0xed,0xde,0xc5,0x31,0xfe,0x18,0xd,0x63,0x8c,0x80,0xc0,0xf7,0x70,0x7};
static BYTE ptab[]={0x1,
		    0x3,0x5,0xf,0x11,0x33,0x55,0xff,0x1a,0x2e,0x72,0x96,0xa1,0xf8,0x13,0x35,0x5f,
		    0xe1,0x38,0x48,0xd8,0x73,0x95,0xa4,0xf7,0x2,0x6,0xa,0x1e,0x22,0x66,0xaa,0xe5,
		    0x34,0x5c,0xe4,0x37,0x59,0xeb,0x26,0x6a,0xbe,0xd9,0x70,0x90,0xab,0xe6,0x31,0x53,
		    0xf5,0x4,0xc,0x14,0x3c,0x44,0xcc,0x4f,0xd1,0x68,0xb8,0xd3,0x6e,0xb2,0xcd,0x4c,
		    0xd4,0x67,0xa9,0xe0,0x3b,0x4d,0xd7,0x62,0xa6,0xf1,0x8,0x18,0x28,0x78,0x88,0x83,
		    0x9e,0xb9,0xd0,0x6b,0xbd,0xdc,0x7f,0x81,0x98,0xb3,0xce,0x49,0xdb,0x76,0x9a,0xb5,
		    0xc4,0x57,0xf9,0x10,0x30,0x50,0xf0,0xb,0x1d,0x27,0x69,0xbb,0xd6,0x61,0xa3,0xfe,
		    0x19,0x2b,0x7d,0x87,0x92,0xad,0xec,0x2f,0x71,0x93,0xae,0xe9,0x20,0x60,0xa0,0xfb,
		    0x16,0x3a,0x4e,0xd2,0x6d,0xb7,0xc2,0x5d,0xe7,0x32,0x56,0xfa,0x15,0x3f,0x41,0xc3,
		    0x5e,0xe2,0x3d,0x47,0xc9,0x40,0xc0,0x5b,0xed,0x2c,0x74,0x9c,0xbf,0xda,0x75,0x9f,
		    0xba,0xd5,0x64,0xac,0xef,0x2a,0x7e,0x82,0x9d,0xbc,0xdf,0x7a,0x8e,0x89,0x80,0x9b,
		    0xb6,0xc1,0x58,0xe8,0x23,0x65,0xaf,0xea,0x25,0x6f,0xb1,0xc8,0x43,0xc5,0x54,0xfc,
		    0x1f,0x21,0x63,0xa5,0xf4,0x7,0x9,0x1b,0x2d,0x77,0x99,0xb0,0xcb,0x46,0xca,0x45,
		    0xcf,0x4a,0xde,0x79,0x8b,0x86,0x91,0xa8,0xe3,0x3e,0x42,0xc6,0x51,0xf3,0xe,0x12,
		    0x36,0x5a,0xee,0x29,0x7b,0x8d,0x8c,0x8f,0x8a,0x85,0x94,0xa7,0xf2,0xd,0x17,0x39,
		    0x4b,0xdd,0x7c,0x84,0x97,0xa2,0xfd,0x1c,0x24,0x6c,0xb4,0xc7,0x52,0xf6,0x1};
static BYTE fbsub[]={0x63,
		     0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x1,0x67,0x2b,0xfe,0xd7,0xab,0x76,0xca,
		     0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,0xb7,
		     0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,0x4,
		     0xc7,0x23,0xc3,0x18,0x96,0x5,0x9a,0x7,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,0x9,
		     0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,0x53,
		     0xd1,0x0,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,0xd0,
		     0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x2,0x7f,0x50,0x3c,0x9f,0xa8,0x51,
		     0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,0xcd,
		     0xc,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,0x60,
		     0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0xb,0xdb,0xe0,
		     0x32,0x3a,0xa,0x49,0x6,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,0xe7,
		     0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x8,0xba,
		     0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,0x70,
		     0x3e,0xb5,0x66,0x48,0x3,0xf6,0xe,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,0xe1,
		     0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,0x8c,
		     0xa1,0x89,0xd,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0xf,0xb0,0x54,0xbb,0x16};
static BYTE rbsub[]={0x52,
		     0x9,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,0x7c,
		     0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,0x54,
		     0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0xb,0x42,0xfa,0xc3,0x4e,0x8,
		     0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,0x72,
		     0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,0x6c,
		     0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,0x90,
		     0xd8,0xab,0x0,0x8c,0xbc,0xd3,0xa,0xf7,0xe4,0x58,0x5,0xb8,0xb3,0x45,0x6,0xd0,
		     0x2c,0x1e,0x8f,0xca,0x3f,0xf,0x2,0xc1,0xaf,0xbd,0x3,0x1,0x13,0x8a,0x6b,0x3a,
		     0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,0x96,
		     0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,0x47,
		     0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0xe,0xaa,0x18,0xbe,0x1b,0xfc,
		     0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,0x1f,
		     0xdd,0xa8,0x33,0x88,0x7,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,0x60,
		     0x51,0x7f,0xa9,0x19,0xb5,0x4a,0xd,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,0xa0,
		     0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,0x17,
		     0x2b,0x4,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0xc,0x7d};
static WORD ftable[]={0xa56363c6,
		      0x847c7cf8,0x997777ee,0x8d7b7bf6,0xdf2f2ff,0xbd6b6bd6,0xb16f6fde,0x54c5c591,0x50303060,
		      0x3010102,0xa96767ce,0x7d2b2b56,0x19fefee7,0x62d7d7b5,0xe6abab4d,0x9a7676ec,0x45caca8f,
		      0x9d82821f,0x40c9c989,0x877d7dfa,0x15fafaef,0xeb5959b2,0xc947478e,0xbf0f0fb,0xecadad41,
		      0x67d4d4b3,0xfda2a25f,0xeaafaf45,0xbf9c9c23,0xf7a4a453,0x967272e4,0x5bc0c09b,0xc2b7b775,
		      0x1cfdfde1,0xae93933d,0x6a26264c,0x5a36366c,0x413f3f7e,0x2f7f7f5,0x4fcccc83,0x5c343468,
		      0xf4a5a551,0x34e5e5d1,0x8f1f1f9,0x937171e2,0x73d8d8ab,0x53313162,0x3f15152a,0xc040408,
		      0x52c7c795,0x65232346,0x5ec3c39d,0x28181830,0xa1969637,0xf05050a,0xb59a9a2f,0x907070e,
		      0x36121224,0x9b80801b,0x3de2e2df,0x26ebebcd,0x6927274e,0xcdb2b27f,0x9f7575ea,0x1b090912,
		      0x9e83831d,0x742c2c58,0x2e1a1a34,0x2d1b1b36,0xb26e6edc,0xee5a5ab4,0xfba0a05b,0xf65252a4,
		      0x4d3b3b76,0x61d6d6b7,0xceb3b37d,0x7b292952,0x3ee3e3dd,0x712f2f5e,0x97848413,0xf55353a6,
		      0x68d1d1b9,0x0,0x2cededc1,0x60202040,0x1ffcfce3,0xc8b1b179,0xed5b5bb6,0xbe6a6ad4,
		      0x46cbcb8d,0xd9bebe67,0x4b393972,0xde4a4a94,0xd44c4c98,0xe85858b0,0x4acfcf85,0x6bd0d0bb,
		      0x2aefefc5,0xe5aaaa4f,0x16fbfbed,0xc5434386,0xd74d4d9a,0x55333366,0x94858511,0xcf45458a,
		      0x10f9f9e9,0x6020204,0x817f7ffe,0xf05050a0,0x443c3c78,0xba9f9f25,0xe3a8a84b,0xf35151a2,
		      0xfea3a35d,0xc0404080,0x8a8f8f05,0xad92923f,0xbc9d9d21,0x48383870,0x4f5f5f1,0xdfbcbc63,
		      0xc1b6b677,0x75dadaaf,0x63212142,0x30101020,0x1affffe5,0xef3f3fd,0x6dd2d2bf,0x4ccdcd81,
		      0x140c0c18,0x35131326,0x2fececc3,0xe15f5fbe,0xa2979735,0xcc444488,0x3917172e,0x57c4c493,
		      0xf2a7a755,0x827e7efc,0x473d3d7a,0xac6464c8,0xe75d5dba,0x2b191932,0x957373e6,0xa06060c0,
		      0x98818119,0xd14f4f9e,0x7fdcdca3,0x66222244,0x7e2a2a54,0xab90903b,0x8388880b,0xca46468c,
		      0x29eeeec7,0xd3b8b86b,0x3c141428,0x79dedea7,0xe25e5ebc,0x1d0b0b16,0x76dbdbad,0x3be0e0db,
		      0x56323264,0x4e3a3a74,0x1e0a0a14,0xdb494992,0xa06060c,0x6c242448,0xe45c5cb8,0x5dc2c29f,
		      0x6ed3d3bd,0xefacac43,0xa66262c4,0xa8919139,0xa4959531,0x37e4e4d3,0x8b7979f2,0x32e7e7d5,
		      0x43c8c88b,0x5937376e,0xb76d6dda,0x8c8d8d01,0x64d5d5b1,0xd24e4e9c,0xe0a9a949,0xb46c6cd8,
		      0xfa5656ac,0x7f4f4f3,0x25eaeacf,0xaf6565ca,0x8e7a7af4,0xe9aeae47,0x18080810,0xd5baba6f,
		      0x887878f0,0x6f25254a,0x722e2e5c,0x241c1c38,0xf1a6a657,0xc7b4b473,0x51c6c697,0x23e8e8cb,
		      0x7cdddda1,0x9c7474e8,0x211f1f3e,0xdd4b4b96,0xdcbdbd61,0x868b8b0d,0x858a8a0f,0x907070e0,
		      0x423e3e7c,0xc4b5b571,0xaa6666cc,0xd8484890,0x5030306,0x1f6f6f7,0x120e0e1c,0xa36161c2,
		      0x5f35356a,0xf95757ae,0xd0b9b969,0x91868617,0x58c1c199,0x271d1d3a,0xb99e9e27,0x38e1e1d9,
		      0x13f8f8eb,0xb398982b,0x33111122,0xbb6969d2,0x70d9d9a9,0x898e8e07,0xa7949433,0xb69b9b2d,
		      0x221e1e3c,0x92878715,0x20e9e9c9,0x49cece87,0xff5555aa,0x78282850,0x7adfdfa5,0x8f8c8c03,
		      0xf8a1a159,0x80898909,0x170d0d1a,0xdabfbf65,0x31e6e6d7,0xc6424284,0xb86868d0,0xc3414182,
		      0xb0999929,0x772d2d5a,0x110f0f1e,0xcbb0b07b,0xfc5454a8,0xd6bbbb6d,0x3a16162c};
static WORD rtable[]={0x50a7f451,
		      0x5365417e,0xc3a4171a,0x965e273a,0xcb6bab3b,0xf1459d1f,0xab58faac,0x9303e34b,0x55fa3020,
		      0xf66d76ad,0x9176cc88,0x254c02f5,0xfcd7e54f,0xd7cb2ac5,0x80443526,0x8fa362b5,0x495ab1de,
		      0x671bba25,0x980eea45,0xe1c0fe5d,0x2752fc3,0x12f04c81,0xa397468d,0xc6f9d36b,0xe75f8f03,
		      0x959c9215,0xeb7a6dbf,0xda595295,0x2d83bed4,0xd3217458,0x2969e049,0x44c8c98e,0x6a89c275,
		      0x78798ef4,0x6b3e5899,0xdd71b927,0xb64fe1be,0x17ad88f0,0x66ac20c9,0xb43ace7d,0x184adf63,
		      0x82311ae5,0x60335197,0x457f5362,0xe07764b1,0x84ae6bbb,0x1ca081fe,0x942b08f9,0x58684870,
		      0x19fd458f,0x876cde94,0xb7f87b52,0x23d373ab,0xe2024b72,0x578f1fe3,0x2aab5566,0x728ebb2,
		      0x3c2b52f,0x9a7bc586,0xa50837d3,0xf2872830,0xb2a5bf23,0xba6a0302,0x5c8216ed,0x2b1ccf8a,
		      0x92b479a7,0xf0f207f3,0xa1e2694e,0xcdf4da65,0xd5be0506,0x1f6234d1,0x8afea6c4,0x9d532e34,
		      0xa055f3a2,0x32e18a05,0x75ebf6a4,0x39ec830b,0xaaef6040,0x69f715e,0x51106ebd,0xf98a213e,
		      0x3d06dd96,0xae053edd,0x46bde64d,0xb58d5491,0x55dc471,0x6fd40604,0xff155060,0x24fb9819,
		      0x97e9bdd6,0xcc434089,0x779ed967,0xbd42e8b0,0x888b8907,0x385b19e7,0xdbeec879,0x470a7ca1,
		      0xe90f427c,0xc91e84f8,0x0,0x83868009,0x48ed2b32,0xac70111e,0x4e725a6c,0xfbff0efd,
		      0x5638850f,0x1ed5ae3d,0x27392d36,0x64d90f0a,0x21a65c68,0xd1545b9b,0x3a2e3624,0xb1670a0c,
		      0xfe75793,0xd296eeb4,0x9e919b1b,0x4fc5c080,0xa220dc61,0x694b775a,0x161a121c,0xaba93e2,
		      0xe52aa0c0,0x43e0223c,0x1d171b12,0xb0d090e,0xadc78bf2,0xb9a8b62d,0xc8a91e14,0x8519f157,
		      0x4c0775af,0xbbdd99ee,0xfd607fa3,0x9f2601f7,0xbcf5725c,0xc53b6644,0x347efb5b,0x7629438b,
		      0xdcc623cb,0x68fcedb6,0x63f1e4b8,0xcadc31d7,0x10856342,0x40229713,0x2011c684,0x7d244a85,
		      0xf83dbbd2,0x1132f9ae,0x6da129c7,0x4b2f9e1d,0xf330b2dc,0xec52860d,0xd0e3c177,0x6c16b32b,
		      0x99b970a9,0xfa489411,0x2264e947,0xc48cfca8,0x1a3ff0a0,0xd82c7d56,0xef903322,0xc74e4987,
		      0xc1d138d9,0xfea2ca8c,0x360bd498,0xcf81f5a6,0x28de7aa5,0x268eb7da,0xa4bfad3f,0xe49d3a2c,
		      0xd927850,0x9bcc5f6a,0x62467e54,0xc2138df6,0xe8b8d890,0x5ef7392e,0xf5afc382,0xbe805d9f,
		      0x7c93d069,0xa92dd56f,0xb31225cf,0x3b99acc8,0xa77d1810,0x6e639ce8,0x7bbb3bdb,0x97826cd,
		      0xf418596e,0x1b79aec,0xa89a4f83,0x656e95e6,0x7ee6ffaa,0x8cfbc21,0xe6e815ef,0xd99be7ba,
		      0xce366f4a,0xd4099fea,0xd67cb029,0xafb2a431,0x31233f2a,0x3094a5c6,0xc066a235,0x37bc4e74,
		      0xa6ca82fc,0xb0d090e0,0x15d8a733,0x4a9804f1,0xf7daec41,0xe50cd7f,0x2ff69117,0x8dd64d76,
		      0x4db0ef43,0x544daacc,0xdf0496e4,0xe3b5d19e,0x1b886a4c,0xb81f2cc1,0x7f516546,0x4ea5e9d,
		      0x5d358c01,0x737487fa,0x2e410bfb,0x5a1d67b3,0x52d2db92,0x335610e9,0x1347d66d,0x8c61d79a,
		      0x7a0ca137,0x8e14f859,0x893c13eb,0xee27a9ce,0x35c961b7,0xede51ce1,0x3cb1477a,0x59dfd29c,
		      0x3f73f255,0x79ce1418,0xbf37c773,0xeacdf753,0x5baafd5f,0x146f3ddf,0x86db4478,0x81f3afca,
		      0x3ec468b9,0x2c342438,0x5f40a3c2,0x72c31d16,0xc25e2bc,0x8b493c28,0x41950dff,0x7101a839,
		      0xdeb30c08,0x9ce4b4d8,0x90c15664,0x6184cb7b,0x70b632d5,0x745c6c48,0x4257b8d0};
#endif

/* Parameter-dependent data */

int Nk,Nb,Nr;
BYTE fi[24],ri[24];
WORD fkey[120];
WORD rkey[120];

static WORD pack(BYTE *b)
{ /* pack bytes into a 32-bit Word */
    return ((WORD)b[3]<<24)|((WORD)b[2]<<16)|((WORD)b[1]<<8)|(WORD)b[0];
}

static void unpack(WORD a,BYTE *b)
{ /* unpack bytes from a word */
    b[0]=(BYTE)a;
    b[1]=(BYTE)(a>>8);
    b[2]=(BYTE)(a>>16);
    b[3]=(BYTE)(a>>24);
}

static BYTE bmul(BYTE x,BYTE y)
{ /* x.y= AntiLog(Log(x) + Log(y)) */
    if (x && y) return ptab[(ltab[x]+ltab[y])%255];
    else return 0;
}

static WORD SubByte(WORD a)
{
    BYTE b[4];
    unpack(a,b);
    b[0]=fbsub[b[0]];
    b[1]=fbsub[b[1]];
    b[2]=fbsub[b[2]];
    b[3]=fbsub[b[3]];
    return pack(b);    
}

static BYTE product(WORD x,WORD y)
{ /* dot product of two 4-byte arrays */
    BYTE xb[4],yb[4];
    unpack(x,xb);
    unpack(y,yb); 
    return bmul(xb[0],yb[0])^bmul(xb[1],yb[1])^bmul(xb[2],yb[2])^bmul(xb[3],yb[3]);
}

static WORD InvMixCol(WORD x)
{ /* matrix Multiplication */
    WORD y,m;
    BYTE b[4];

    m=pack(InCo);
    b[3]=product(m,x);
    m=ROTL24(m);
    b[2]=product(m,x);
    m=ROTL24(m);
    b[1]=product(m,x);
    m=ROTL24(m);
    b[0]=product(m,x);
    y=pack(b);
    return y;
}

BYTE ByteSub(BYTE x)
{
    BYTE y=ptab[255-ltab[x]];  /* multiplicative inverse */
    x=y;  x=ROTL(x);
    y^=x; x=ROTL(x);
    y^=x; x=ROTL(x);
    y^=x; x=ROTL(x);
    y^=x; y^=0x63;
    return y;
}

#ifndef STATICTABLES
static BYTE xtime(BYTE a)
{
    BYTE b;
    if (a&0x80) b=0x1B;
    else        b=0;
    a<<=1;
    a^=b;
    return a;
}

void rijndaelGentables(void)
{ /* generate tables */
    int i;
    BYTE x,y,b[4];

    /* use 3 as primitive root to generate power and log tables */
    
    ltab[0]=0;
    ptab[0]=1;  ltab[1]=0;
    ptab[1]=3;  ltab[3]=1; 
    for (i=2;i<256;i++)
    {
        ptab[i]=ptab[i-1]^xtime(ptab[i-1]);
        ltab[ptab[i]]=i;
    }
    
    /* affine transformation:- each bit is xored with itself shifted one bit */
    
    fbsub[0]=0x63;
    rbsub[0x63]=0;
    for (i=1;i<256;i++)
      {
        y=ByteSub((BYTE)i);
        fbsub[i]=y; rbsub[y]=i;
      }
    
    for (i=0,y=1;i<30;i++)
    {
        rco[i]=y;
        y=xtime(y);
    }
    
    /* calculate forward and reverse tables */
    for (i=0;i<256;i++)
      {
        y=fbsub[i];
        b[3]=y^xtime(y); b[2]=y;
        b[1]=y;          b[0]=xtime(y);
        ftable[i]=pack(b);
	
        y=rbsub[i];
        b[3]=bmul(InCo[0],y); b[2]=bmul(InCo[1],y);
        b[1]=bmul(InCo[2],y); b[0]=bmul(InCo[3],y);
        rtable[i]=pack(b);
    }
}
#endif

void rijndaelKeySched(int nb, int nk, char *key)
{ /* blocksize=32*nb bits. Key=32*nk bits */
  /* currently nb,bk = 4, 6 or 8          */
  /* key comes as 4*Nk bytes              */
  /* Key Scheduler. Create expanded encryption key */
    int i,j,k,m,N;
    int C1,C2,C3;
    WORD CipherKey[8];
    
    Nb=nb; Nk=nk;

  /* Nr is number of rounds */
    if (Nb>=Nk) Nr=6+Nb;
    else        Nr=6+Nk;

    C1=1;
    if (Nb<8) { C2=2; C3=3; }
    else      { C2=3; C3=4; }

  /* pre-calculate forward and reverse increments */
    for (m=j=0;j<nb;j++,m+=3)
    {
        fi[m]=(j+C1)%nb;
        fi[m+1]=(j+C2)%nb;
        fi[m+2]=(j+C3)%nb;
        ri[m]=(nb+j-C1)%nb;
        ri[m+1]=(nb+j-C2)%nb;
        ri[m+2]=(nb+j-C3)%nb;
    }

    N=Nb*(Nr+1);
    
    for (i=j=0;i<Nk;i++,j+=4)
    {
        CipherKey[i]=pack(&key[j]);
    }
    for (i=0;i<Nk;i++) fkey[i]=CipherKey[i];
    for (j=Nk,k=0;j<N;j+=Nk,k++)
    {
        fkey[j]=fkey[j-Nk]^SubByte(ROTL24(fkey[j-1]))^rco[k];
        if (Nk<=6)
        {
            for (i=1;i<Nk && (i+j)<N;i++)
                fkey[i+j]=fkey[i+j-Nk]^fkey[i+j-1];
        }
        else
        {
            for (i=1;i<4 &&(i+j)<N;i++)
                fkey[i+j]=fkey[i+j-Nk]^fkey[i+j-1];
            if ((j+4)<N) fkey[j+4]=fkey[j+4-Nk]^SubByte(fkey[j+3]);
            for (i=5;i<Nk && (i+j)<N;i++)
                fkey[i+j]=fkey[i+j-Nk]^fkey[i+j-1];
        }

    }

 /* now for the expanded decrypt key in reverse order */

    for (j=0;j<Nb;j++) rkey[j+N-Nb]=fkey[j]; 
    for (i=Nb;i<N-Nb;i+=Nb)
    {
        k=N-Nb-i;
        for (j=0;j<Nb;j++) rkey[k+j]=InvMixCol(fkey[i+j]);
    }
    for (j=N-Nb;j<N;j++) rkey[j-N+Nb]=fkey[j];
}


/* There is an obvious time/space trade-off possible here.     *
 * Instead of just one ftable[], I could have 4, the other     *
 * 3 pre-rotated to save the ROTL8, ROTL16 and ROTL24 overhead */ 

void rijndaelEncrypt(char *buff)
{
    int i,j,k,m;
    WORD a[8],b[8],*x,*y,*t;

    for (i=j=0;i<Nb;i++,j+=4)
    {
        a[i]=pack(&buff[j]);
        a[i]^=fkey[i];
    }
    k=Nb;
    x=a; y=b;

/* State alternates between a and b */
    for (i=1;i<Nr;i++)
    { /* Nr is number of rounds. May be odd. */

/* if Nb is fixed - unroll this next 
   loop and hard-code in the values of fi[]  */

        for (m=j=0;j<Nb;j++,m+=3)
        { /* deal with each 32-bit element of the State */
          /* This is the time-critical bit */
            y[j]=fkey[k++]^ftable[(BYTE)x[j]]^
                 ROTL8(ftable[(BYTE)(x[fi[m]]>>8)])^
                 ROTL16(ftable[(BYTE)(x[fi[m+1]]>>16)])^
                 ROTL24(ftable[x[fi[m+2]]>>24]);
        }
        t=x; x=y; y=t;      /* swap pointers */
    }

/* Last Round - unroll if possible */ 
    for (m=j=0;j<Nb;j++,m+=3)
    {
        y[j]=fkey[k++]^(WORD)fbsub[(BYTE)x[j]]^
             ROTL8((WORD)fbsub[(BYTE)(x[fi[m]]>>8)])^
             ROTL16((WORD)fbsub[(BYTE)(x[fi[m+1]]>>16)])^
             ROTL24((WORD)fbsub[x[fi[m+2]]>>24]);
    }   
    for (i=j=0;i<Nb;i++,j+=4)
    {
        unpack(y[i],&buff[j]);
        x[i]=y[i]=0;   /* clean up stack */
    }
    return;
}

void rijndaelDecrypt(char *buff)
{
    int i,j,k,m;
    WORD a[8],b[8],*x,*y,*t;

    for (i=j=0;i<Nb;i++,j+=4)
    {
        a[i]=pack(&buff[j]);
        a[i]^=rkey[i];
    }
    k=Nb;
    x=a; y=b;

/* State alternates between a and b */
    for (i=1;i<Nr;i++)
    { /* Nr is number of rounds. May be odd. */

/* if Nb is fixed - unroll this next 
   loop and hard-code in the values of ri[]  */

        for (m=j=0;j<Nb;j++,m+=3)
        { /* This is the time-critical bit */
            y[j]=rkey[k++]^rtable[(BYTE)x[j]]^
                 ROTL8(rtable[(BYTE)(x[ri[m]]>>8)])^
                 ROTL16(rtable[(BYTE)(x[ri[m+1]]>>16)])^
                 ROTL24(rtable[x[ri[m+2]]>>24]);
        }
        t=x; x=y; y=t;      /* swap pointers */
    }

/* Last Round - unroll if possible */ 
    for (m=j=0;j<Nb;j++,m+=3)
    {
        y[j]=rkey[k++]^(WORD)rbsub[(BYTE)x[j]]^
             ROTL8((WORD)rbsub[(BYTE)(x[ri[m]]>>8)])^
             ROTL16((WORD)rbsub[(BYTE)(x[ri[m+1]]>>16)])^
             ROTL24((WORD)rbsub[x[ri[m+2]]>>24]);
    }        
    for (i=j=0;i<Nb;i++,j+=4)
    {
        unpack(y[i],&buff[j]);
        x[i]=y[i]=0;   /* clean up stack */
    }
    return;
}

#ifdef TEST
int main()
{ /* test driver */
    int i,j,n,nb,nk;
    BYTE y,x,m;
    char key[32];
    char block[32];

#ifndef STATICTABLES
    rijndaelGentables();
#endif

    for (i=0;i<32;i++) key[i]=0;
    key[0]=1;
    for (i=0;i<32;i++) block[i]=i;

    for (nb=4;nb<=8;nb+=2)
        for (nk=4;nk<=8;nk+=2)
    {  
        printf("\nBlock Size= %d bits, Key Size= %d bits\n",nb*32,nk*32);
        rijndaelKeySched(nb,nk,key);
        printf("Plain=   ");
        for (i=0;i<nb*4;i++) printf("%2x",block[i]);
        printf("\n");
        rijndaelEncrypt(block);
        printf("Encrypt= ");
        for (i=0;i<nb*4;i++) printf("%2x",(unsigned char)block[i]);
        printf("\n");
        rijndaelDecrypt(block);
        printf("Decrypt= ");
        for (i=0;i<nb*4;i++) printf("%2x",block[i]);
        printf("\n");
    }
    return 0;
}
#endif TEST



