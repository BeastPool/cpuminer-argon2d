#include "pentablake-gate.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "sph_blake.h"

//#define DEBUG_ALGO

extern void pentablakehash(void *output, const void *input)
{
	unsigned char _ALIGN(32) hash[128];
	// same as uint32_t hashA[16], hashB[16];
	#define hashB hash+64

	sph_blake512_context     ctx_blake;

	sph_blake512_init(&ctx_blake);
	sph_blake512(&ctx_blake, input, 80);
	sph_blake512_close(&ctx_blake, hash);

        sph_blake512_init(&ctx_blake);
	sph_blake512(&ctx_blake, hash, 64);
	sph_blake512_close(&ctx_blake, hashB);

        sph_blake512_init(&ctx_blake);
	sph_blake512(&ctx_blake, hashB, 64);
	sph_blake512_close(&ctx_blake, hash);

        sph_blake512_init(&ctx_blake);
	sph_blake512(&ctx_blake, hash, 64);
	sph_blake512_close(&ctx_blake, hashB);

        sph_blake512_init(&ctx_blake);
	sph_blake512(&ctx_blake, hashB, 64);
	sph_blake512_close(&ctx_blake, hash);

	memcpy(output, hash, 32);

}

int scanhash_pentablake(int thr_id, struct work *work, uint32_t max_nonce,
      uint64_t *hashes_done)
{
        uint32_t *pdata = work->data;
        uint32_t *ptarget = work->target;

	uint32_t n = pdata[19] - 1;
	const uint32_t first_nonce = pdata[19];
	const uint32_t Htarg = ptarget[7];

	uint32_t _ALIGN(32) hash64[8];
	uint32_t _ALIGN(32) endiandata[32];

	uint64_t htmax[] = {
		0,
		0xF,
		0xFF,
		0xFFF,
		0xFFFF,
		0x10000000
	};
	uint32_t masks[] = {
		0xFFFFFFFF,
		0xFFFFFFF0,
		0xFFFFFF00,
		0xFFFFF000,
		0xFFFF0000,
		0
	};

	// we need bigendian data...
        swab32_array( endiandata, pdata, 20 );

#ifdef DEBUG_ALGO
	if (Htarg != 0)
		printf("[%d] Htarg=%X\n", thr_id, Htarg);
#endif
	for (int m=0; m < 6; m++) {
		if (Htarg <= htmax[m]) {
			uint32_t mask = masks[m];
			do {
				pdata[19] = ++n;
				be32enc(&endiandata[19], n);
				pentablakehash(hash64, endiandata);
#ifndef DEBUG_ALGO
				if ((!(hash64[7] & mask)) && fulltest(hash64, ptarget)) {
					*hashes_done = n - first_nonce + 1;
					return true;
				}
#else
				if (!(n % 0x1000) && !thr_id) printf(".");
				if (!(hash64[7] & mask)) {
					printf("[%d]",thr_id);
					if (fulltest(hash64, ptarget)) {
						*hashes_done = n - first_nonce + 1;
						return true;
					}
				}
#endif
			} while (n < max_nonce && !work_restart[thr_id].restart);
			// see blake.c if else to understand the loop on htmax => mask
			break;
		}
	}

	*hashes_done = n - first_nonce + 1;
	pdata[19] = n;
	return 0;
} 

