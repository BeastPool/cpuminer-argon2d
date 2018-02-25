#if !defined(LUFFA_HASH_2WAY_H__)
#define LUFFA_HASH_2WAY_H__ 1
/*
 * luffa_for_sse2.h
 * Version 2.0 (Sep 15th 2009)
 *
 * Copyright (C) 2008-2009 Hitachi, Ltd. All rights reserved.
 *
 * Hitachi, Ltd. is the owner of this software and hereby grant
 * the U.S. Government and any interested party the right to use
 * this software for the purposes of the SHA-3 evaluation process,
 * notwithstanding that this software is copyrighted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(__AVX2__)

#include <immintrin.h>
#include "algo/sha/sha3-defs.h"
#include "avxdefs.h"

/* The length of digests*/
#define DIGEST_BIT_LEN_224 224
#define DIGEST_BIT_LEN_256 256
#define DIGEST_BIT_LEN_384 384
#define DIGEST_BIT_LEN_512 512

/*********************************/
/* The parameters of Luffa       */
#define MSG_BLOCK_BIT_LEN 256  /*The bit length of a message block*/
#define MSG_BLOCK_BYTE_LEN (MSG_BLOCK_BIT_LEN >> 3) /* The byte length
                                                     * of a message block*/

/* The number of blocks in Luffa */
#define WIDTH_224 3
#define WIDTH_256 3
#define WIDTH_384 4
#define WIDTH_512 5

/* The limit of the length of message */
#define LIMIT_224 64
#define LIMIT_256 64
#define LIMIT_384 128
#define LIMIT_512 128
/*********************************/

typedef struct {
    uint32 buffer[8*2] __attribute((aligned(64)));
    __m256i chainv[10] __attribute((aligned(32)));   /* Chaining values */
    int hashbitlen;
    int rembytes;
} luffa_2way_context;

int luffa_2way_init( luffa_2way_context *state, int hashbitlen );
int luffa_2way_update( luffa_2way_context *state, const void *data,
                       size_t len );
int luffa_2way_close( luffa_2way_context *state, void *hashval );
int luffa_2way_update_close( luffa_2way_context *state, void *output,
                                   const void *data, size_t inlen );

#endif
#endif
