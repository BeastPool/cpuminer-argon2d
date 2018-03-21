#ifndef LYRA2Z_GATE_H__
#define LYRA2Z_GATE_H__ 1

#include "algo-gate-api.h"
#include <stdint.h>

#if defined(__AVX__)
  #define LYRA2Z_4WAY
#endif
#if defined(__AVX2__)
//  #define LYRA2Z_8WAY
#endif


#define LYRA2Z_MATRIX_SIZE  BLOCK_LEN_INT64 * 8 * 8 * 8

#if defined(LYRA2Z_8WAY)

void lyra2z_8way_hash( void *state, const void *input );

int scanhash_lyra2z_8way( int thr_id, struct work *work, uint32_t max_nonce,
                         uint64_t *hashes_done );

bool lyra2z_8way_thread_init();

#elif defined(LYRA2Z_4WAY)

void lyra2z_4way_hash( void *state, const void *input );

int scanhash_lyra2z_4way( int thr_id, struct work *work, uint32_t max_nonce,
                         uint64_t *hashes_done );

bool lyra2z_4way_thread_init();

#else

void lyra2z_hash( void *state, const void *input );

int scanhash_lyra2z( int thr_id, struct work *work, uint32_t max_nonce,
                    uint64_t *hashes_done );

bool lyra2z_thread_init();

#endif

#endif
