#include "phi1612-gate.h"

#if defined(PHI1612_4WAY)

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "algo/skein/skein-hash-4way.h"
#include "algo/jh/jh-hash-4way.h"
#include "algo/cubehash/sse2/cubehash_sse2.h"
#include "algo/fugue/sph_fugue.h"
#include "algo/gost/sph_gost.h"
#include "algo/echo/aes_ni/hash_api.h"

typedef struct {
    skein512_4way_context   skein;
    jh512_4way_context      jh;
    cubehashParam           cube;
    sph_fugue512_context    fugue;
    sph_gost512_context     gost;
    hashState_echo          echo;
} phi1612_4way_ctx_holder;

phi1612_4way_ctx_holder phi1612_4way_ctx __attribute__ ((aligned (64)));

void init_phi1612_4way_ctx()
{
     skein512_4way_init( &phi1612_4way_ctx.skein );
     jh512_4way_init( &phi1612_4way_ctx.jh );
     cubehashInit( &phi1612_4way_ctx.cube, 512, 16, 32 );
     sph_fugue512_init( &phi1612_4way_ctx.fugue );
     sph_gost512_init( &phi1612_4way_ctx.gost );
     init_echo( &phi1612_4way_ctx.echo, 512 );
};

void phi1612_4way_hash( void *state, const void *input )
{
     uint64_t hash0[8] __attribute__ ((aligned (64)));
     uint64_t hash1[8] __attribute__ ((aligned (64)));
     uint64_t hash2[8] __attribute__ ((aligned (64)));
     uint64_t hash3[8] __attribute__ ((aligned (64)));
     uint64_t vhash[8*4] __attribute__ ((aligned (64)));
     phi1612_4way_ctx_holder ctx;
     memcpy( &ctx, &phi1612_4way_ctx, sizeof(phi1612_4way_ctx) );

     // Skein parallel 4way
     skein512_4way( &ctx.skein, input, 80 );
     skein512_4way_close( &ctx.skein, vhash );

     // JH
     jh512_4way( &ctx.jh, vhash, 64 );
     jh512_4way_close( &ctx.jh, vhash );

     // Serial to the end
     mm256_deinterleave_4x64( hash0, hash1, hash2, hash3, vhash, 512 );

     // Cubehash
     cubehashUpdateDigest( &ctx.cube, (byte*)hash0, (const byte*) hash0, 64 );
     memcpy( &ctx.cube, &phi1612_4way_ctx.cube, sizeof(cubehashParam) );
     cubehashUpdateDigest( &ctx.cube, (byte*)hash1, (const byte*) hash1, 64 );
     memcpy( &ctx.cube, &phi1612_4way_ctx.cube, sizeof(cubehashParam) );
     cubehashUpdateDigest( &ctx.cube, (byte*)hash2, (const byte*) hash2, 64 );
     memcpy( &ctx.cube, &phi1612_4way_ctx.cube, sizeof(cubehashParam) );
     cubehashUpdateDigest( &ctx.cube, (byte*)hash3, (const byte*) hash3, 64 );

     // Fugue
     sph_fugue512( &ctx.fugue, hash0, 64 );
     sph_fugue512_close( &ctx.fugue, hash0 );
     sph_fugue512_init( &ctx.fugue );
     sph_fugue512( &ctx.fugue, hash1, 64 );
     sph_fugue512_close( &ctx.fugue, hash1 );
     sph_fugue512_init( &ctx.fugue );
     sph_fugue512( &ctx.fugue, hash2, 64 );
     sph_fugue512_close( &ctx.fugue, hash2 );
     sph_fugue512_init( &ctx.fugue );
     sph_fugue512( &ctx.fugue, hash3, 64 );
     sph_fugue512_close( &ctx.fugue, hash3 );

     // Gost
     sph_gost512( &ctx.gost, hash0, 64 );
     sph_gost512_close( &ctx.gost, hash0 );
     sph_gost512_init( &ctx.gost );
     sph_gost512( &ctx.gost, hash1, 64 );
     sph_gost512_close( &ctx.gost, hash1 );
     sph_gost512_init( &ctx.gost );
     sph_gost512( &ctx.gost, hash2, 64 );
     sph_gost512_close( &ctx.gost, hash2 );
     sph_gost512_init( &ctx.gost );
     sph_gost512( &ctx.gost, hash3, 64 );
     sph_gost512_close( &ctx.gost, hash3 );

     // Echo
     update_final_echo( &ctx.echo, (BitSequence *)hash0,
                       (const BitSequence *) hash0, 512 );
     init_echo( &ctx.echo, 512 );
     update_final_echo( &ctx.echo, (BitSequence *)hash1,
                       (const BitSequence *) hash1, 512 );
     init_echo( &ctx.echo, 512 );
     update_final_echo( &ctx.echo, (BitSequence *)hash2,
                       (const BitSequence *) hash2, 512 );
     init_echo( &ctx.echo, 512 );
     update_final_echo( &ctx.echo, (BitSequence *)hash3,
                       (const BitSequence *) hash3, 512 );

     memcpy( state,    hash0, 32 );
     memcpy( state+32, hash1, 32 );
     memcpy( state+64, hash2, 32 );
     memcpy( state+96, hash3, 32 );
}

int scanhash_phi1612_4way( int thr_id, struct work *work, uint32_t max_nonce,
                  uint64_t *hashes_done )
{
     uint32_t hash[4*8] __attribute__ ((aligned (64)));
     uint32_t vdata[24*4] __attribute__ ((aligned (64)));
     uint32_t *pdata = work->data;
     uint32_t *ptarget = work->target;
     const uint32_t first_nonce = pdata[19];
     uint32_t _ALIGN(64) endiandata[20];
     uint32_t n = first_nonce;
     uint32_t *nonces = work->nonces;
     int num_found = 0;
     uint32_t *noncep = vdata + 73;   // 9*8 + 1
     const uint32_t Htarg = ptarget[7];

     if ( opt_benchmark )
          ( (uint32_t*)ptarget )[7] = 0x0cff;

     for ( int k = 0; k < 19; k++ )
        be32enc( &endiandata[k], pdata[k] );

     uint64_t *edata = (uint64_t*)endiandata;
     mm256_interleave_4x64( (uint64_t*)vdata, edata, edata, edata, edata, 640 );

     do {
        be32enc( noncep,   n   );
        be32enc( noncep+2, n+1 );
        be32enc( noncep+4, n+2 );
        be32enc( noncep+6, n+3 );

        phi1612_4way_hash( hash, vdata );
        pdata[19] = n;

        for ( int i = 0; i < 4; i++ )
        if ( (hash+(i<<3))[7] <= Htarg && fulltest( hash+(i<<3), ptarget ) )
        {
           pdata[19] = n+i;
           nonces[ num_found++ ] = n+i;
           work_set_target_ratio( work, hash+(i<<3) );
        }
        n += 4;
     } while ( ( num_found == 0 ) && ( n < max_nonce )
               && !work_restart[thr_id].restart );

     *hashes_done = n - first_nonce + 1;
     return num_found;
}

#endif
