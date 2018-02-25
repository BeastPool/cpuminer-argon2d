#include "qubit-gate.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "algo/luffa/luffa_for_sse2.h" 
#include "algo/cubehash/sse2/cubehash_sse2.h" 
#include "algo/simd/nist.h"
#include "algo/shavite/sph_shavite.h"
#ifndef NO_AES_NI
#include "algo/echo/aes_ni/hash_api.h"
#else
#include "algo/echo/sph_echo.h"
#endif

typedef struct
{
        hashState_luffa         luffa;
        cubehashParam           cubehash;
        sph_shavite512_context  shavite;
        hashState_sd            simd;
#ifdef NO_AES_NI
        sph_echo512_context echo;
#else
        hashState_echo          echo;
#endif
} qubit_ctx_holder;

qubit_ctx_holder qubit_ctx;
static __thread hashState_luffa qubit_luffa_mid;

void init_qubit_ctx()
{
        init_luffa(&qubit_ctx.luffa,512);
        cubehashInit(&qubit_ctx.cubehash,512,16,32);
        sph_shavite512_init(&qubit_ctx.shavite);
        init_sd(&qubit_ctx.simd,512);
#ifdef NO_AES_NI
        sph_echo512_init(&qubit_ctx.echo);
#else
        init_echo(&qubit_ctx.echo, 512);
#endif
};

void qubit_luffa_midstate( const void* input )
{
    memcpy( &qubit_luffa_mid, &qubit_ctx.luffa, sizeof qubit_luffa_mid );
    update_luffa( &qubit_luffa_mid, input, 64 );
}

void qubit_hash(void *output, const void *input)
{
        unsigned char hash[128] __attribute((aligned(64)));
        #define hashB hash+64

        qubit_ctx_holder ctx;
        memcpy( &ctx, &qubit_ctx, sizeof(qubit_ctx) );

        const int midlen = 64;            // bytes
        const int tail   = 80 - midlen;   // 16
        memcpy( &ctx.luffa, &qubit_luffa_mid, sizeof qubit_luffa_mid );
        update_and_final_luffa( &ctx.luffa, (BitSequence*)hash,
                                (const BitSequence*)input + midlen, tail );

        cubehashUpdateDigest( &ctx.cubehash, (byte*)hash,
                              (const byte*) hash, 64 );

        sph_shavite512( &ctx.shavite, hash, 64);
        sph_shavite512_close( &ctx.shavite, hash);

        update_final_sd( &ctx.simd, (BitSequence *)hash,
                         (const BitSequence*)hash,  512 );

#ifdef NO_AES_NI
        sph_echo512 (&ctx.echo, (const void*) hash, 64);
        sph_echo512_close(&ctx.echo, (void*) hash);
#else
        update_final_echo( &ctx.echo, (BitSequence *) hash,
                     (const BitSequence *) hash, 512 );
#endif

        asm volatile ("emms");
        memcpy(output, hash, 32);
}

int scanhash_qubit(int thr_id, struct work *work,
		uint32_t max_nonce, uint64_t *hashes_done)
{
        uint32_t endiandata[20] __attribute__((aligned(64)));
        uint32_t hash64[8] __attribute__((aligned(64)));
        uint32_t *pdata = work->data;
        uint32_t *ptarget = work->target;
	uint32_t n = pdata[19] - 1;
	const uint32_t first_nonce = pdata[19];
	const uint32_t Htarg = ptarget[7];

	uint64_t htmax[] = { 0, 0xF, 0xFF,  0xFFF, 0xFFFF, 0x10000000 };
	uint32_t masks[] =
          { 0xFFFFFFFF, 0xFFFFFFF0, 0xFFFFFF00, 0xFFFFF000, 0xFFFF0000, 0 };

	// we need bigendian data...
        swab32_array( endiandata, pdata, 20 );

        qubit_luffa_midstate( endiandata );

#ifdef DEBUG_ALGO
	printf("[%d] Htarg=%X\n", thr_id, Htarg);
#endif
	for ( int m=0; m < 6; m++ )
        {
	    if ( Htarg <= htmax[m] )
            {
	        uint32_t mask = masks[m];
	        do
                {
	            pdata[19] = ++n;
		    be32enc(&endiandata[19], n);
		    qubit_hash(hash64, endiandata);
#ifndef DEBUG_ALGO
		    if (!(hash64[7] & mask))
                    {
                       if ( fulltest(hash64, ptarget) )
                       {
		          *hashes_done = n - first_nonce + 1;
		          return true;
                       }
//                       else
//                       {
//                          applog(LOG_INFO, "Result does not validate on CPU!");
//                       }
                     }
#else
                    if (!(n % 0x1000) && !thr_id) printf(".");
	        	if (!(hash64[7] & mask)) {
		            printf("[%d]",thr_id);
			    if (fulltest(hash64, ptarget)) {
                             work_set_target_ratio( work, hash64 );
                             *hashes_done = n - first_nonce + 1;
				return true;
	                    }
 	                }
#endif
                } while ( n < max_nonce && !work_restart[thr_id].restart );
                // see blake.c if else to understand the loop on htmax => mask
            break;
          } 
        }

	*hashes_done = n - first_nonce + 1;
	pdata[19] = n;
	return 0;
}

